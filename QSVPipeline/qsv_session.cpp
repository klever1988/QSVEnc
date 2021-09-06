﻿// -----------------------------------------------------------------------------------------
// QSVEnc by rigaya
// -----------------------------------------------------------------------------------------
// The MIT License
//
// Copyright (c) 2011-2016 rigaya
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
// --------------------------------------------------------------------------------------------

#include "qsv_util.h"
#include "rgy_err.h"
#include "rgy_osdep.h"
#include "rgy_frame.h"

#include "qsv_session.h"
#include <mutex>
#include "rgy_env.h"
#if defined(_WIN32) || defined(_WIN64)
#include "api_hook.h"
#endif //#if defined(_WIN32) || defined(_WIN64)
#include "qsv_query.h"
#include "qsv_hw_device.h"
#include "qsv_allocator.h"
#include "qsv_allocator_sys.h"
#include "mfxdispatcher.h"

#if D3D_SURFACES_SUPPORT
#include "qsv_hw_d3d9.h"
#include "qsv_hw_d3d11.h"

#include "qsv_allocator_d3d9.h"
#include "qsv_allocator_d3d11.h"
#endif

#if LIBVA_SUPPORT
#include "qsv_hw_va.h"
#include "qsv_allocator_va.h"
#endif

RGY_ERR MFXVideoSession2::InitSessionInitParam() {
    INIT_MFX_EXT_BUFFER(m_ThreadsParam, MFX_EXTBUFF_THREADS_PARAM);
    //m_ThreadsParam.NumThread = (mfxU16)clamp_param_int(m_prm.threads, 0, QSV_SESSION_THREAD_MAX, _T("session-threads"));
    //m_ThreadsParam.Priority = (mfxU16)clamp_param_int(m_prm.priority, MFX_PRIORITY_LOW, MFX_PRIORITY_HIGH, _T("priority"));
    //m_pInitParamExtBuf[0] = &m_ThreadsParam.Header;

    RGY_MEMSET_ZERO(m_InitParam);
    //m_InitParam.ExtParam = m_pInitParamExtBuf;
    //m_InitParam.NumExtParam = 1;
    return RGY_ERR_NONE;
}

#if defined(_WIN32) || defined(_WIN64)
typedef decltype(GetSystemInfo)* funcGetSystemInfo;
static int nGetSystemInfoHookThreads = -1;
static std::mutex mtxGetSystemInfoHook;
static funcGetSystemInfo origGetSystemInfoFunc = nullptr;
void __stdcall GetSystemInfoHook(LPSYSTEM_INFO lpSystemInfo) {
    origGetSystemInfoFunc(lpSystemInfo);
    if (lpSystemInfo && nGetSystemInfoHookThreads > 0) {
        decltype(lpSystemInfo->dwActiveProcessorMask) mask = 0;
        const int nThreads = std::max(1, std::min(nGetSystemInfoHookThreads, (int)sizeof(lpSystemInfo->dwActiveProcessorMask) * 8));
        for (int i = 0; i < nThreads; i++) {
            mask |= ((size_t)1 << i);
        }
        lpSystemInfo->dwActiveProcessorMask = mask;
        lpSystemInfo->dwNumberOfProcessors = nThreads;
    }
}
#endif

MFXVideoSession2Params::MFXVideoSession2Params() : threads(0), priority(0) {};

void MFXVideoSession2::PrintMes(RGYLogLevel log_level, const TCHAR *format, ...) {
    if (m_log == nullptr) {
        if (log_level <= RGY_LOG_INFO) {
            return;
        }
    } else if (log_level < m_log->getLogLevel(RGY_LOGT_CORE)) {
        return;
    }

    va_list args;
    va_start(args, format);

    int len = _vsctprintf(format, args) + 1; // _vscprintf doesn't count terminating '\0'
    vector<TCHAR> buffer(len, 0);
    _vstprintf_s(buffer.data(), len, format, args);
    va_end(args);

    if (m_log.get() != nullptr) {
        m_log->write(log_level, RGY_LOGT_CORE, buffer.data());
    } else {
        _ftprintf(stderr, _T("%s"), buffer.data());
    }
}

MFXVideoSession2::MFXVideoSession2() :
    MFXVideoSession(),
    m_log(),
    m_prm(),
    m_InitParam(),
    m_pInitParamExtBuf(),
    m_ThreadsParam() {
    RGY_MEMSET_ZERO(m_InitParam);
    INIT_MFX_EXT_BUFFER(m_ThreadsParam, MFX_EXTBUFF_THREADS_PARAM);
    for (size_t i = 0; i < _countof(m_pInitParamExtBuf); i++) {
        m_pInitParamExtBuf[i] = nullptr;
    }
}

void MFXVideoSession2::setParams(std::shared_ptr<RGYLog>& log, const MFXVideoSession2Params& params) {
    m_log = log;
    m_prm = params;
}

mfxStatus MFXVideoSession2::initImpl(mfxIMPL& impl) {
    mfxVersion verRequired = MFX_LIB_VERSION_1_1;
#if defined(_WIN32) || defined(_WIN64)
    std::lock_guard<std::mutex> lock(mtxGetSystemInfoHook);
    nGetSystemInfoHookThreads = m_prm.threads;
    apihook api_hook;
    api_hook.hook(_T("kernel32.dll"), "GetSystemInfo", GetSystemInfoHook, (void **)&origGetSystemInfoFunc);
#endif
#if USE_ONEVPL
    auto loader = MFXLoad();
    auto cfg = MFXCreateConfig(loader);
    //hwの使用を要求
    mfxVariant ImplValueHW;
    ImplValueHW.Type = MFX_VARIANT_TYPE_U32;
    ImplValueHW.Data.U32 = MFX_IMPL_TYPE_HARDWARE;
    auto sts = MFXSetConfigFilterProperty(cfg, (const mfxU8 *)"mfxImplDescription.Impl", ImplValueHW);
    if (sts != MFX_ERR_NONE) {
        m_log->write(RGY_LOG_ERROR, RGY_LOGT_CORE, _T("MFXVideoSession2::init: Failed to set mfxImplDescription.Impl %d: %s.\n"), ImplValueHW.Data.U32, get_err_mes(err_to_rgy(sts)));
        return sts;
    }

    mfxVariant accMode;
    accMode.Type = MFX_VARIANT_TYPE_U32;
#if D3D_SURFACES_SUPPORT
#if MFX_D3D11_SUPPORT
    if ((impl & MFX_IMPL_VIA_D3D11) == MFX_IMPL_VIA_D3D11) {
        accMode.Data.U32 = MFX_ACCEL_MODE_VIA_D3D11;
    } else
#endif
    if ((impl & MFX_IMPL_VIA_D3D9) == MFX_IMPL_VIA_D3D9) {
        accMode.Data.U32 = MFX_ACCEL_MODE_VIA_D3D9;
    }
#elif LIBVA_SUPPORT
    if ((impl & MFX_IMPL_VIA_VAAPI) == MFX_IMPL_VIA_VAAPI) {
        accMode.Data.U32 = MFX_ACCEL_MODE_VIA_VAAPI;
    }
#endif
    sts = MFXSetConfigFilterProperty(cfg, (const mfxU8 *)"mfxImplDescription.AccelerationMode", accMode);
    if (sts != MFX_ERR_NONE) {
        m_log->write(RGY_LOG_ERROR, RGY_LOGT_CORE, _T("MFXVideoSession2::init: Failed to set mfxImplDescription.AccelerationMode %d: %s.\n"), ImplValueHW.Data.U32, get_err_mes(err_to_rgy(sts)));
        return sts;
    }

    m_log->write(RGY_LOG_DEBUG, RGY_LOGT_CORE, _T("MFXVideoSession2::init: try init by MFXCreateSession.\n"));
    sts = MFXCreateSession(loader, 0, (mfxSession *)&m_session);
    if (sts == MFX_ERR_NONE) return sts;
    m_log->write(RGY_LOG_DEBUG, RGY_LOGT_CORE, _T("MFXVideoSession2::init: Failed to init by MFXCreateSession.\n"));
#endif
    m_log->write(RGY_LOG_DEBUG, RGY_LOGT_CORE, _T("MFXVideoSession2::init: try init by MFXInit.\n"));
    return Init(impl, &verRequired);
}

mfxStatus MFXVideoSession2::initHW(mfxIMPL& impl) {
    auto err = initImpl(impl);
    if (err != MFX_ERR_NONE) {
        if (impl & MFX_IMPL_HARDWARE_ANY) {  //MFX_IMPL_HARDWARE_ANYがサポートされない場合もあり得るので、失敗したらこれをオフにしてもう一回試す
            impl &= (~MFX_IMPL_HARDWARE_ANY);
            impl |= MFX_IMPL_HARDWARE;
        } else if (impl & MFX_IMPL_HARDWARE) {  //MFX_IMPL_HARDWAREで失敗したら、MFX_IMPL_HARDWARE_ANYでもう一回試す
            impl &= (~MFX_IMPL_HARDWARE);
            impl |= MFX_IMPL_HARDWARE_ANY;
        }
        err = initImpl(impl);
    }
    return err;
}

mfxStatus MFXVideoSession2::initD3D9() {
    mfxIMPL impl = MFX_IMPL_HARDWARE_ANY | MFX_IMPL_VIA_D3D9;
    auto err = initHW(impl);
    PrintMes((err) ? RGY_LOG_ERROR : RGY_LOG_DEBUG, _T("InitSession(d3d9): %s.\n"), get_err_mes(err));
    return err;
}

mfxStatus MFXVideoSession2::initD3D11() {
    mfxIMPL impl = MFX_IMPL_HARDWARE_ANY | MFX_IMPL_VIA_D3D11;
    auto err = initHW(impl);
    PrintMes((err) ? RGY_LOG_ERROR : RGY_LOG_DEBUG, _T("InitSession(d3d11): %s.\n"), get_err_mes(err));
    return err;
}

mfxStatus MFXVideoSession2::initVA() {
    mfxIMPL impl = MFX_IMPL_HARDWARE_ANY | MFX_IMPL_VIA_VAAPI;
    auto err = initHW(impl);
    PrintMes((err) ? RGY_LOG_ERROR : RGY_LOG_DEBUG, _T("InitSession(va): %s.\n"), get_err_mes(err));
    return err;
}

mfxStatus MFXVideoSession2::initSW() {
    mfxIMPL impl = MFX_IMPL_SOFTWARE;
    auto err = initImpl(impl);
    PrintMes((err) ? RGY_LOG_ERROR : RGY_LOG_DEBUG, _T("InitSession(sys): %s.\n"), get_err_mes(err));
    return err;
}

RGY_ERR InitSession(MFXVideoSession2& mfxSession, const MFXVideoSession2Params& params, const mfxIMPL impl, std::shared_ptr<RGYLog>& log) {
    mfxSession.setParams(log, params);
	auto err = RGY_ERR_NOT_INITIALIZED;
#if D3D_SURFACES_SUPPORT
#if MFX_D3D11_SUPPORT
	if ((impl & MFX_IMPL_VIA_D3D11) == MFX_IMPL_VIA_D3D11) {
		err = err_to_rgy(mfxSession.initD3D11());
		if (err != RGY_ERR_NONE) err = RGY_ERR_NOT_INITIALIZED;
	}
#endif
	if ((impl & MFX_IMPL_VIA_D3D9) == MFX_IMPL_VIA_D3D9 && err == RGY_ERR_NOT_INITIALIZED) {
		err = err_to_rgy(mfxSession.initD3D9());
	}
#elif LIBVA_SUPPORT
	err = err_to_rgy(mfxSession.initVA());
#endif
	return err;
}

mfxIMPL GetDefaultMFXImpl(MemType memType) {
	mfxIMPL impl = 0;
#if D3D_SURFACES_SUPPORT
    if ((memType & D3D11_MEMORY) || memType == SYSTEM_MEMORY) {
        impl |= MFX_IMPL_VIA_D3D11;
    }
#if MFX_D3D11_SUPPORT
    if ((memType & D3D9_MEMORY) || memType == SYSTEM_MEMORY) {
        impl |= MFX_IMPL_VIA_D3D9;
    }
#endif
#elif LIBVA_SUPPORT
	impl |= MFX_IMPL_VIA_VAAPI;
#endif
	return impl;
}

RGY_ERR InitSessionAndDevice(std::unique_ptr<CQSVHWDevice>& hwdev, MFXVideoSession2& mfxSession, MemType& memType, const MFXVideoSession2Params& params, std::shared_ptr<RGYLog>& log) {
    auto sts = RGY_ERR_NONE;
    {
        auto targetImpl = GetDefaultMFXImpl(memType);
#if D3D_SURFACES_SUPPORT
        //Win7でD3D11のチェックをやると、
        //デスクトップコンポジションが切られてしまう問題が発生すると報告を頂いたので、
        //D3D11をWin8以降に限定
        if (!check_OS_Win8orLater() || MFX_D3D11_SUPPORT == 0) {
            memType &= (MemType)(~D3D11_MEMORY);
            targetImpl &= (~MFX_IMPL_VIA_D3D11);
            log->write(RGY_LOG_DEBUG, RGY_LOGT_CORE, _T("InitSession: OS is Win7, do not check for d3d11 mode.\n"));
        }
#endif //#if D3D_SURFACES_SUPPORT

        if ((sts = InitSession(mfxSession, params, targetImpl, log)) != RGY_ERR_NONE) {
            log->write(RGY_LOG_ERROR, RGY_LOGT_CORE, _T("Failed to init session: %s.\n"), get_err_mes(sts));
            return sts;
        }
    }

    mfxVersion mfxVer;
    mfxSession.QueryVersion(&mfxVer);
    mfxIMPL impl;
    mfxSession.QueryIMPL(&impl);
    log->write(RGY_LOG_DEBUG, RGY_LOGT_CORE, _T("InitSession: mfx lib version: %d.%02d, impl %s\n"), mfxVer.Major, mfxVer.Minor, MFXImplToStr(impl).c_str());

    hwdev.reset();
    if (memType != SYSTEM_MEMORY) {
#if D3D_SURFACES_SUPPORT
#if MFX_D3D11_SUPPORT
        if ((impl & MFX_IMPL_VIA_D3D11) == MFX_IMPL_VIA_D3D11
            && (hwdev = std::make_unique<CQSVD3D11Device>(log))) {
            memType = D3D11_MEMORY;
            log->write(RGY_LOG_DEBUG, RGY_LOGT_CORE, _T("HWDevice: d3d11 - init...\n"));

            sts = err_to_rgy(hwdev->Init(NULL, 0, GetAdapterID(mfxSession.get())));
            if (sts != RGY_ERR_NONE) {
                hwdev.reset();
                log->write(RGY_LOG_DEBUG, RGY_LOGT_CORE, _T("HWDevice: d3d11 - init failed.\n"));
            }
        }
#endif // #if MFX_D3D11_SUPPORT
        if (!hwdev
			&& (hwdev = std::make_unique<CQSVD3D9Device>(log))) {
            //もし、d3d11要求で失敗したら自動的にd3d9に切り替える
            //sessionごと切り替える必要がある
            if ((impl & MFX_IMPL_VIA_D3D9) != MFX_IMPL_VIA_D3D9) {
                log->write(RGY_LOG_DEBUG, RGY_LOGT_CORE, _T("Retry openning device, changing to d3d9 mode, re-init session.\n"));

				if ((sts = InitSession(mfxSession, params, MFX_IMPL_VIA_D3D9, log)) != RGY_ERR_NONE) {
					log->write(RGY_LOG_ERROR, RGY_LOGT_CORE, _T("Failed to init session: %s.\n"), get_err_mes(sts));
					return sts;
				}
            }

            log->write(RGY_LOG_DEBUG, RGY_LOGT_CORE, _T("HWDevice: d3d9 - init...\n"));
			POINT point = {0, 0};
			HWND window = WindowFromPoint(point);
            sts = err_to_rgy(hwdev->Init(window, 0, GetAdapterID(mfxSession.get())));
        }
#elif LIBVA_SUPPORT
        if ((impl & MFX_IMPL_VIA_VAAPI) == MFX_IMPL_VIA_VAAPI) {
            memType = VA_MEMORY;
			hwdev.reset(CreateVAAPIDevice("", MFX_LIBVA_DRM, log));
			if (!hwdev) {
				return RGY_ERR_MEMORY_ALLOC;
			}
			sts = err_to_rgy(hwdev->Init(NULL, 0, GetAdapterID(mfxSession.get())));
		}
#endif
		if (!hwdev
			|| sts != RGY_ERR_NONE) {
            log->write(RGY_LOG_ERROR, RGY_LOGT_CORE, _T("Failed to initialize HW Device.: %s.\n"), get_err_mes(sts));
			return sts;
		}
		log->write(RGY_LOG_DEBUG, RGY_LOGT_CORE, _T("HWDevice: init device success.\n"));
    }
    return RGY_ERR_NONE;
}

RGY_ERR CreateAllocator(
    std::unique_ptr<QSVAllocator>& allocator, bool& externalAlloc,
    const MemType memType, CQSVHWDevice *hwdev, MFXVideoSession2& session, std::shared_ptr<RGYLog>& log) {
    auto sts = RGY_ERR_NONE;
    if (log) log->write(RGY_LOG_DEBUG, RGY_LOGT_CORE, _T("CreateAllocator: MemType: %s\n"), MemTypeToStr(memType));

#define CA_ERR(ret, MES)    {if (RGY_ERR_NONE > (ret)) { if (log) log->write(RGY_LOG_ERROR, RGY_LOGT_CORE, _T("%s : %s\n"), MES, get_err_mes(ret)); return ret;}}

    mfxIMPL impl = 0;
    session.QueryIMPL(&impl);
    if (
#if LIBVA_SUPPORT
        MFX_IMPL_BASETYPE(impl) == MFX_IMPL_HARDWARE || //システムメモリ使用でも MFX_HANDLE_VA_DISPLAYをHW libraryに渡してやる必要がある
#endif //#if LIBVA_SUPPORT
        D3D9_MEMORY == memType || D3D11_MEMORY == memType || VA_MEMORY == memType || HW_MEMORY == memType) {

        const mfxHandleType hdl_t = mfxHandleTypeFromMemType(memType, false);
        mfxHDL hdl = NULL;
        sts = err_to_rgy(hwdev->GetHandle(hdl_t, &hdl));
        CA_ERR(sts, _T("Failed to get HW device handle."));
            if (log) log->write(RGY_LOG_DEBUG, RGY_LOGT_CORE, _T("CreateAllocator: HW device GetHandle success.\n"));

        if (impl != MFX_IMPL_SOFTWARE) {
            // hwエンコード時のみハンドルを渡す
            sts = err_to_rgy(session.SetHandle(hdl_t, hdl));
            CA_ERR(sts, _T("Failed to set HW device handle to main session."));
            if (log) log->write(RGY_LOG_DEBUG, RGY_LOGT_CORE, _T("CreateAllocator: set HW device handle to main session.\n"));
        }
    }

    std::unique_ptr<mfxAllocatorParams> allocParams;
    if (D3D9_MEMORY == memType || D3D11_MEMORY == memType || VA_MEMORY == memType || HW_MEMORY == memType) {
#if D3D_SURFACES_SUPPORT
        mfxHDL hdl = nullptr;
        const auto hdl_t = mfxHandleTypeFromMemType(memType, false);
        if (hdl_t) {
            sts = err_to_rgy(hwdev->GetHandle(hdl_t, &hdl));
            if (sts != RGY_ERR_NONE) {
                if (log) log->write(RGY_LOG_ERROR, RGY_LOGT_CORE, _T("Failed to get HW device handle: %s.\n"), get_err_mes(sts));
                return sts;
            }
        }
        //D3D allocatorを作成
#if MFX_D3D11_SUPPORT
        if (D3D11_MEMORY == memType) {
            if (log) log->write(RGY_LOG_DEBUG, RGY_LOGT_CORE, _T("CreateAllocator: Create d3d11 allocator.\n"));
            allocator.reset(new QSVAllocatorD3D11);
            if (!allocator) {
                if (log) log->write(RGY_LOG_ERROR, RGY_LOGT_CORE, _T("Failed to allcate memory for D3D11FrameAllocator.\n"));
                return RGY_ERR_MEMORY_ALLOC;
            }
            auto allocParamsD3D11 = std::make_unique<QSVAllocatorParamsD3D11>();
            allocParamsD3D11->pDevice = reinterpret_cast<ID3D11Device *>(hdl);
            if (log) log->write(RGY_LOG_DEBUG, RGY_LOGT_CORE, _T("CreateAllocator: d3d11...\n"));

            allocParams.reset(allocParamsD3D11.release());
        } else
#endif // #if MFX_D3D11_SUPPORT
        {
            if (log) log->write(RGY_LOG_DEBUG, RGY_LOGT_CORE, _T("CreateAllocator: Create d3d9 allocator.\n"));
            allocator.reset(new QSVAllocatorD3D9);
            if (!allocator) {
                if (log) log->write(RGY_LOG_ERROR, RGY_LOGT_CORE, _T("Failed to allcate memory for D3DFrameAllocator.\n"));
                return RGY_ERR_MEMORY_ALLOC;
            }

            auto allocParamsD3D9 = std::make_unique<QSVAllocatorParamsD3D9>();
            allocParamsD3D9->pManager = reinterpret_cast<IDirect3DDeviceManager9 *>(hdl);
            //通常、OpenCL-d3d9間のinteropでrelease/acquireで余計なオーバーヘッドが発生させないために、
            //shared_handleを取得する必要がある(qsv_opencl.hのgetOpenCLFrameInterop()参照)
            //shared_handleはd3d9でCreateSurfaceする際に取得する。
            //しかし、これを取得しようとするとWin7のSandybridge環境ではデコードが正常に行われなくなってしまう問題があるとの報告を受けた
            //そのため、shared_handleを取得するのは、SandyBridgeでない環境に限るようにする
            allocParamsD3D9->getSharedHandle = getCPUGen(&session) != CPU_GEN_SANDYBRIDGE;
            if (log) log->write(RGY_LOG_DEBUG, RGY_LOGT_CORE, _T("CreateAllocator: d3d9 (getSharedHandle = %s)...\n"), allocParamsD3D9->getSharedHandle ? _T("true") : _T("false"));

            allocParams.reset(allocParamsD3D9.release());
        }

        //GPUメモリ使用時には external allocatorを使用する必要がある
        //mfxSessionにallocatorを渡してやる必要がある
        sts = err_to_rgy(session.SetFrameAllocator(allocator.get()));
        CA_ERR(sts, _T("Failed to set frame allocator to encode session."));
        if (log) log->write(RGY_LOG_DEBUG, RGY_LOGT_CORE, _T("CreateAllocator: frame allocator set to session.\n"));

        externalAlloc = true;
#endif
#if LIBVA_SUPPORT
        mfxHDL hdl = NULL;
        sts = err_to_rgy(hwdev->GetHandle(MFX_HANDLE_VA_DISPLAY, &hdl));
        CA_ERR(sts, _T("Failed to get HW device handle."));
        if (log) log->write(RGY_LOG_DEBUG, RGY_LOGT_CORE, _T("CreateAllocator: HW device GetHandle success. : 0x%x\n"), (uint32_t)(size_t)hdl);

        //VAAPI allocatorを作成
        allocator.reset(new QSVAllocatorVA());
        if (!allocator) {
            if (log) log->write(RGY_LOG_ERROR, RGY_LOGT_CORE, _T("Failed to allcate memory for vaapiFrameAllocator.\n"));
            return RGY_ERR_MEMORY_ALLOC;
        }

        auto p_vaapiAllocParams = std::make_unique<QSVAllocatorParamsVA>();
        p_vaapiAllocParams->m_dpy = (VADisplay)hdl;
        allocParams.reset(p_vaapiAllocParams.release());

        //GPUメモリ使用時には external allocatorを使用する必要がある
        //mfxSessionにallocatorを渡してやる必要がある
        sts = err_to_rgy(session.SetFrameAllocator(allocator.get()));
        CA_ERR(sts, _T("Failed to set frame allocator to encode session."));
        if (log) log->write(RGY_LOG_DEBUG, RGY_LOGT_CORE, _T("CreateAllocator: frame allocator set to session.\n"));

        externalAlloc = true;
#endif
    } else {
        //system memory allocatorを作成
        allocator.reset(new QSVAllocatorSys);
        if (!allocator) {
            return RGY_ERR_MEMORY_ALLOC;
        }
        if (log) log->write(RGY_LOG_DEBUG, RGY_LOGT_CORE, _T("CreateAllocator: sys mem allocator...\n"));
    }

    //メモリallocatorの初期化
    if (RGY_ERR_NONE > (sts = err_to_rgy(allocator->Init(allocParams.get(), log)))) {
        if (log) log->write(RGY_LOG_ERROR, RGY_LOGT_CORE, _T("Failed to initialize %s memory allocator. : %s\n"), MemTypeToStr(memType), get_err_mes(sts));
        return sts;
    }
    if (log) log->write(RGY_LOG_DEBUG, RGY_LOGT_CORE, _T("CreateAllocator: frame allocator initialized.\n"));
#undef CA_ERR
    return RGY_ERR_NONE;
}
