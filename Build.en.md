
# How to build QSVEnc

- [Windows](./Build.en.md#windows)
- Linux
  - [Linux (Ubuntu 22.04)](./Build.en.md#linux-ubuntu-2204)
  - [Linux (Ubuntu 20.04)](./Build.en.md#linux-ubuntu-2004)
  - [Linux (Ubuntu 18.04)](./Build.en.md#linux-ubuntu-1804)
  - [Linux (Fedora 32)](./Build.en.md#linux-fedora-32)
  - [Intel Drivers for Linux](/Build.en.md#Intel-Drivers-for-Linux)

## Windows

### 0. Requirements
To build QSVEnc, components below are required.

- Visual Studio 2019
- [Avisynth](https://github.com/AviSynth/AviSynthPlus) SDK
- [VapourSynth](http://www.vapoursynth.com/) SDK

Install Avisynth+ and VapourSynth, with the SDKs.

Then, "avisynth_c.h" of the Avisynth+ SDK and "VapourSynth.h" of the VapourSynth SDK should be added to the include path of Visual Studio.

These include path can be passed by environment variables "AVISYNTH_SDK" and "VAPOURSYNTH_SDK".

With default installation, environment variables could be set as below.
```Batchfile
setx AVISYNTH_SDK "C:\Program Files (x86)\AviSynth+\FilterSDK"
setx VAPOURSYNTH_SDK "C:\Program Files (x86)\VapourSynth\sdk"
```

[OpenCL header files](https://github.com/KhronosGroup/OpenCL-Headers.git) will also be needed,
and the include path shall be passed by environment variables "OPENCL_HEADERS".

```Batchfile
git clone https://github.com/KhronosGroup/OpenCL-Headers.git <path-to-clone>
setx OPENCL_HEADERS <path-to-clone>
```

You will also need source code of [Caption2Ass_PCR](https://github.com/maki-rxrz/Caption2Ass_PCR).

```Batchfile
git clone https://github.com/maki-rxrz/Caption2Ass_PCR <path-to-clone>
setx CAPTION2ASS_SRC <path-to-clone>/src
```


### 1. Download source code

```Batchfile
git clone https://github.com/rigaya/QSVEnc --recursive
cd QSVEnc
git clone https://github.com/rigaya/ffmpeg5_dlls_for_hwenc.git ffmpeg_lgpl
```

### 2. Build QSVEncC.exe / QSVEnc.auo

Finally, open QSVEnc.sln, and start build of QSVEnc by Visual Studio.

|  |For Debug build|For Release build|
|:--------------|:--------------|:--------|
|QSVEnc.auo (win32 only) | Debug | Release |
|QSVEncC(64).exe | DebugStatic | RelStatic |


## Linux (Ubuntu 22.04)

### 0. Requirements

- C++17 Compiler
- Intel Driver
- git
- cmake
- libraries
  - libva, libdrm 
  - ffmpeg 4.x libs (libavcodec58, libavformat58, libavfilter7, libavutil56, libswresample3, libavdevice58)
  - libass9
  - [Optional] VapourSynth

### 1. Install build tools

```Shell
sudo apt install build-essential libtool pkg-config git cmake
```

### 2. Install Intel driver
OpenCL driver can be installed following instruction on [this link](https://dgpu-docs.intel.com/installation-guides/ubuntu/ubuntu-focal.html), but QSVEnc will require only part of it.

```Shell
sudo apt-get install -y gpg-agent wget
wget -qO - https://repositories.intel.com/graphics/intel-graphics.key | \
  sudo gpg --dearmor --output /usr/share/keyrings/intel-graphics.gpg
echo 'deb [arch=amd64 signed-by=/usr/share/keyrings/intel-graphics.gpg] https://repositories.intel.com/graphics/ubuntu jammy main' | \
  sudo tee  /etc/apt/sources.list.d/intel.gpu.focal.list
sudo apt update
sudo apt install intel-media-va-driver-non-free intel-opencl-icd opencl-headers
```

### 3. Install required libraries

```Shell
sudo apt install \
  libva-drm2 \
  libva-x11-2 \
  libva-glx2 \
  libx11-dev \
  libigfxcmrt7 \
  libva-dev \
  libdrm-dev

sudo apt install ffmpeg \
  libavcodec-extra libavcodec-dev libavutil-dev libavformat-dev libswresample-dev libavfilter-dev libavdevice-dev \
  libass9 libass-dev
```

### 4. [Optional] Install VapourSynth
VapourSynth is required only if you need VapourSynth(vpy) reader support.  

Please go on to [5. Build QSVEncC] if you don't need vpy reader.

<details><summary>How to build VapourSynth</summary>

#### 4.1 Install build tools for VapourSynth
```Shell
sudo apt install python3-pip autoconf automake libtool meson
```

#### 4.2 Install zimg
```Shell
git clone https://github.com/sekrit-twc/zimg.git --recursive
cd zimg
./autogen.sh
./configure
make && sudo make install
cd ..
```

#### 4.3 Install cython
```Shell
sudo pip3 install Cython
```

#### 4.4 Install VapourSynth
```Shell
git clone https://github.com/vapoursynth/vapoursynth.git
cd vapoursynth
./autogen.sh
./configure
make && sudo make install

# Make sure vapoursynth could be imported from python
# Change "python3.x" depending on your encironment
sudo ln -s /usr/local/lib/python3.x/site-packages/vapoursynth.so /usr/lib/python3.x/lib-dynload/vapoursynth.so
sudo ldconfig
```

#### 4.5 Check if VapourSynth has been installed properly
Make sure you get version number without errors.
```Shell
vspipe --version
```

#### 4.6 [Option] Build vslsmashsource
```Shell
# Install lsmash
git clone https://github.com/l-smash/l-smash.git
cd l-smash
./configure --enable-shared
make && sudo make install
cd ..
 
# Install vslsmashsource
git clone https://github.com/Mr-Ojii/L-SMASH-Works.git
cd L-SMASH-Works/Avisynth
meson build
cd build
ninja && sudo ninja install
cd ../../../
```

</details>


### 5. [Optional] Install AvisynthPlus
AvisynthPlus is required only if you need AvisynthPlus(avs) reader support.  

Please go on to [7. Build QSVEncC] if you don't need avs reader.

<details><summary>How to build AvisynthPlus</summary>

#### 5.1 Install build tools for AvisynthPlus
```Shell
sudo apt install cmake
```

#### 5.2 Install AvisynthPlus
```Shell
git clone https://github.com/AviSynth/AviSynthPlus.git
cd AviSynthPlus
mkdir avisynth-build && cd avisynth-build 
cmake ../
make && sudo make install
cd ../..
```

#### 5.3 [Option] Build lsmashsource
```Shell
# Install lsmash
git clone https://github.com/l-smash/l-smash.git
cd l-smash
./configure --enable-shared
make && sudo make install
cd ..
 
# Install vslsmashsource
git clone https://github.com/Mr-Ojii/L-SMASH-Works.git
cd L-SMASH-Works/VapourSynth
meson build
cd build
ninja && sudo ninja install
cd ../../../
```
</details>


### 6. Add user to proper group
```Shell
# QSV
sudo gpasswd -a ${USER} video
# OpenCL
sudo gpasswd -a ${USER} render
```

### 7. Build QSVEncC
```Shell
git clone https://github.com/rigaya/QSVEnc --recursive
cd QSVEnc
./configure
make
```
Check if it works properly.
```Shell
./qsvencc --check-hw
```

You shall get results below if Quick Sync Video works properly.
```
Success: QuickSyncVideo (hw encoding) available
```


## Linux (Ubuntu 20.04)

### 0. Requirements

- C++17 Compiler
- Intel Driver
- git
- cmake
- libraries
  - libva, libdrm 
  - ffmpeg 4.x libs (libavcodec58, libavformat58, libavfilter7, libavutil56, libswresample3, libavdevice58)
  - libass9
  - [Optional] VapourSynth

### 1. Install build tools

```Shell
sudo apt install build-essential libtool pkg-config git cmake
```

### 2. Install Intel driver
OpenCL driver can be installed following instruction on [this link](https://dgpu-docs.intel.com/installation-guides/ubuntu/ubuntu-focal.html), but QSVEnc will require only part of it.

```Shell
sudo apt-get install -y gpg-agent wget
wget -qO - https://repositories.intel.com/graphics/intel-graphics.key | \
  sudo gpg --dearmor --output /usr/share/keyrings/intel-graphics.gpg
echo 'deb [arch=amd64 signed-by=/usr/share/keyrings/intel-graphics.gpg] https://repositories.intel.com/graphics/ubuntu focal-devel main' | \
  sudo tee  /etc/apt/sources.list.d/intel.gpu.focal.list
sudo apt update
sudo apt install intel-media-va-driver-non-free intel-opencl-icd opencl-headers
```

### 3. Install required libraries

```Shell
sudo apt install \
  libva-drm2 \
  libva-x11-2 \
  libva-glx2 \
  libx11-dev \
  libigfxcmrt7 \
  libva-dev \
  libdrm-dev

sudo apt install ffmpeg \
  libavcodec-extra libavcodec-dev libavutil-dev libavformat-dev libswresample-dev libavfilter-dev libavdevice-dev \
  libass9 libass-dev
```

### 4. [Optional] Install VapourSynth
VapourSynth is required only if you need VapourSynth(vpy) reader support.  

Please go on to [5. Build QSVEncC] if you don't need vpy reader.

<details><summary>How to build VapourSynth</summary>

#### 4.1 Install build tools for VapourSynth
```Shell
sudo apt install python3-pip autoconf automake libtool meson
```

#### 4.2 Install zimg
```Shell
git clone https://github.com/sekrit-twc/zimg.git --recursive
cd zimg
./autogen.sh
./configure
make && sudo make install
cd ..
```

#### 4.3 Install cython
```Shell
sudo pip3 install Cython
```

#### 4.4 Install VapourSynth
```Shell
git clone https://github.com/vapoursynth/vapoursynth.git
cd vapoursynth
./autogen.sh
./configure
make && sudo make install

# Make sure vapoursynth could be imported from python
# Change "python3.x" depending on your encironment
sudo ln -s /usr/local/lib/python3.x/site-packages/vapoursynth.so /usr/lib/python3.x/lib-dynload/vapoursynth.so
sudo ldconfig
```

#### 4.5 Check if VapourSynth has been installed properly
Make sure you get version number without errors.
```Shell
vspipe --version
```

#### 4.6 [Option] Build vslsmashsource
```Shell
# Install lsmash
git clone https://github.com/l-smash/l-smash.git
cd l-smash
./configure --enable-shared
make && sudo make install
cd ..
 
# Install vslsmashsource
git clone https://github.com/HolyWu/L-SMASH-Works.git
# As the latest version requires more recent ffmpeg libs, checkout the older version
cd L-SMASH-Works
git checkout -b 20200531 refs/tags/20200531
cd Avisynth
meson build
cd build
ninja && sudo ninja install
cd ../../../
```

</details>


### 5. [Optional] Install AvisynthPlus
AvisynthPlus is required only if you need AvisynthPlus(avs) reader support.  

Please go on to [7. Build QSVEncC] if you don't need avs reader.

<details><summary>How to build AvisynthPlus</summary>

#### 5.1 Install build tools for AvisynthPlus
```Shell
sudo apt install cmake
```

#### 5.2 Install AvisynthPlus
```Shell
git clone https://github.com/AviSynth/AviSynthPlus.git
cd AviSynthPlus
mkdir avisynth-build && cd avisynth-build 
cmake ../
make && sudo make install
cd ../..
```

#### 5.3 [Option] Build lsmashsource
```Shell
# Install lsmash
git clone https://github.com/l-smash/l-smash.git
cd l-smash
./configure --enable-shared
make && sudo make install
cd ..
 
# Install vslsmashsource
git clone https://github.com/HolyWu/L-SMASH-Works.git
cd L-SMASH-Works
# Use older version to meet libavcodec lib version requirements
git checkout -b 20200531 refs/tags/20200531
cd VapourSynth
meson build
cd build
ninja && sudo ninja install
cd ../../../
```
</details>


### 6. Add user to proper group
```Shell
# QSV
sudo gpasswd -a ${USER} video
# OpenCL
sudo gpasswd -a ${USER} render
```

### 7. Build QSVEncC
```Shell
git clone https://github.com/rigaya/QSVEnc --recursive
cd QSVEnc
./configure
make
```
Check if it works properly.
```Shell
./qsvencc --check-hw
```

You shall get results below if Quick Sync Video works properly.
```
Success: QuickSyncVideo (hw encoding) available
```

## Linux (Ubuntu 18.04)

In Ubuntu 18.04, you may additionally need to build libva, and media-driver yourself.

### 0. Requirements

- C++17 Compiler
- git
- cmake
- libraries
  - ffmpeg 4.x libs (libavcodec58, libavformat58, libavfilter7, libavutil56, libswresample3, libavdevice58)
  - libass9
  - [Optional] VapourSynth

### 1. Install build tools

```Shell
sudo apt install build-essential meson automake libtool cmake pkg-config git cmake
```

### 2. Install libva

#### 2.1 Install libva dependencies

```Shell
sudo apt-get install  libdrm-dev libx11-dev libxext-dev libxfixes-dev
```

#### 2.2 Build libva

```Shell
git clone https://github.com/intel/libva.git
cd libva
./autogen.sh
./configure
```

You shall get output as below.
```
---------------------------------------------------------------
libva - 2.9.0 (VA-API 1.9.0)

Installation prefix .............. : /usr/local
Default driver path .............. : ${exec_prefix}/lib/dri
Extra window systems ............. : drm x11
Build documentation .............. : no
Build with messaging ............. : yes
---------------------------------------------------------------
```

Then, build and install.
```Shell
make && sudo make install
cd ..
```

### 4. Install media driver

#### 4.1 Build gmmlib
```Shell
git clone https://github.com/intel/gmmlib.git
cd gmmlib
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make && sudo make install
cd ../..
```

<details><summary>Files below will be installed.</summary>

```
-- Install configuration: ""
-- Installing: /usr/local/lib/dri/iHD_drv_video.so
-- Installing: /usr/local/lib/libigfxcmrt.so.7.2.0
-- Installing: /usr/local/lib/libigfxcmrt.so.7
-- Installing: /usr/local/lib/libigfxcmrt.so
-- Installing: /usr/local/include/igfxcmrt/cm_rt.h
-- Installing: /usr/local/include/igfxcmrt/cm_rt_g8.h
-- Installing: /usr/local/include/igfxcmrt/cm_rt_g9.h
-- Installing: /usr/local/include/igfxcmrt/cm_rt_g10.h
-- Installing: /usr/local/include/igfxcmrt/cm_rt_g11.h
-- Installing: /usr/local/include/igfxcmrt/cm_rt_g12_tgl.h
-- Installing: /usr/local/include/igfxcmrt/cm_hw_vebox_cmd_g10.h
-- Installing: /usr/local/include/igfxcmrt/cm_rt_def_os.h
-- Installing: /usr/local/include/igfxcmrt/cm_rt_api_os.h
-- Installing: /usr/local/include/igfxcmrt/cm_rt_extension.h
-- Installing: /usr/local/lib/pkgconfig/igfxcmrt.pc
```

</details>

#### 4.2 Build media driver
```
sudo apt install libdrm-dev xorg xorg-dev openbox libx11-dev libgl1-mesa-glx libgl1-mesa-dev
git clone https://github.com/intel/media-driver.git
mkdir build_media && cd build_media
cmake ../media-driver
make && sudo make install
cd ..
```

### 5. Install Intel OpenCL runtime libraries
OpenCL driver can be innstalled following instruction on [this link](https://dgpu-docs.intel.com/installation-guides/ubuntu/ubuntu-bionic.html).
```Shell
sudo apt-get install -y gpg-agent wget
wget -qO - https://repositories.intel.com/graphics/intel-graphics.key |
  sudo apt-key add -
sudo apt-add-repository \
  'deb [arch=amd64] https://repositories.intel.com/graphics/ubuntu bionic main'
sudo apt-get update
sudo apt-get install \
  intel-opencl \
  intel-level-zero-gpu level-zero
sudo apt install opencl-headers
```

### 6. Install ffmpeg 4.x libraries
```Shell
sudo add-apt-repository ppa:jonathonf/ffmpeg-4
sudo apt update
sudo apt install ffmpeg \
  libavcodec-extra58 libavcodec-dev libavutil56 libavutil-dev libavformat58 libavformat-dev libavdevice58 libavdevice-dev \
  libswresample3 libswresample-dev libavfilter-extra7 libavfilter-dev libass9 libass-dev
```

### 7. [Optional] Install VapourSynth
VapourSynth is required only if you need VapourSynth(vpy) reader support.  

Please go on to [5. Build QSVEncC] if you don't need vpy reader.

<details><summary>How to build VapourSynth</summary>

#### 7.1 Install build tools for VapourSynth
```Shell
sudo apt install python3-pip autoconf automake libtool meson
```

#### 7.2 Install zimg
```Shell
git clone https://github.com/sekrit-twc/zimg.git --recursive
cd zimg
./autogen.sh
./configure
make && sudo make install
cd ..
```

#### 7.3 Install cython
```Shell
sudo pip3 install Cython
```

#### 7.4 Install VapourSynth
```Shell
git clone https://github.com/vapoursynth/vapoursynth.git
cd vapoursynth
./autogen.sh
./configure
make && sudo make install

# Make sure vapoursynth could be imported from python
# Change "python3.x" depending on your encironment
sudo ln -s /usr/local/lib/python3.x/site-packages/vapoursynth.so /usr/lib/python3.x/lib-dynload/vapoursynth.so
sudo ldconfig
```

#### 7.5 Check if VapourSynth has been installed properly
Make sure you get version number without errors.
```Shell
vspipe --version
```

#### 7.6 [Option] Build vslsmashsource
```Shell
# Install lsmash
git clone https://github.com/l-smash/l-smash.git
cd l-smash
./configure --enable-shared
make && sudo make install
cd ..
 
# Install vslsmashsource
git clone https://github.com/HolyWu/L-SMASH-Works.git
# As the latest version requires more recent ffmpeg libs, checkout the older version
cd L-SMASH-Works
git checkout -b 20200531 refs/tags/20200531
cd VapourSynth
meson build
cd build
ninja && sudo ninja install
cd ../../../
```

</details>

### 8. Add user to proper group
```Shell
# QSV
sudo gpasswd -a ${USER} video
# OpenCL
sudo gpasswd -a ${USER} render
```

### 9. Build QSVEncC
```Shell
git clone https://github.com/rigaya/QSVEnc --recursive
cd QSVEnc
./configure --extra-cxxflags="-I/opt/intel/mediasdk/include" --extra-ldflags="-L/opt/intel/mediasdk/lib"
make
```
Check if it works properly.
```Shell
LD_LIBRARY_PATH=/opt/intel/mediasdk/lib ./qsvencc --check-hw
```

You shall get results below if Quick Sync Video works properly.
```
Success: QuickSyncVideo (hw encoding) available
```


## Linux (Fedora 32)

### 0. Requirements

- C++17 Compiler
- Intel Driver
- git
- cmake
- libraries
  - libva, libdrm 
  - ffmpeg 4.x libs (libavcodec58, libavformat58, libavfilter7, libavutil56, libswresample3, libavdevice58)
  - libass9
  - [Optional] VapourSynth

### 1. Install build tools

```Shell
sudo dnf install @development-tools cmake
```

### 2. Install required libraries

```Shell
sudo dnf install https://download1.rpmfusion.org/nonfree/fedora/rpmfusion-nonfree-release-$(rpm -E %fedora).noarch.rpm
sudo dnf install https://download1.rpmfusion.org/free/fedora/rpmfusion-free-release-$(rpm -E %fedora).noarch.rpm

sudo dnf install libva-devel libX11-devel libdrm-devel
sudo dnf install ffmpeg ffmpeg-devel
```

### 3. Install Intel Media driver and OpenCL runtime
OpenCL driver can be innstalled following instruction on [this link](https://dgpu-docs.intel.com/installation-guides/redhat/redhat-8.3.html).

```Shell
#Media
sudo dnf install intel-media-driver
#OpenCL
sudo dnf install -y 'dnf-command(config-manager)'
sudo dnf config-manager --add-repo https://repositories.intel.com/graphics/rhel/8.3/intel-graphics.repo
sudo dnf update --refresh
sudo dnf install intel-opencl intel-media intel-mediasdk level-zero intel-level-zero-gpu
sudo dnf install opencl-headers
```


### 4. [Optional] Install VapourSynth
VapourSynth is required only if you need VapourSynth(vpy) reader support.  

Please go on to [5. Build QSVEncC] if you don't need vpy reader.

<details><summary>How to build VapourSynth</summary>

#### 4.1 Install build tools for VapourSynth
```Shell
sudo apt install python3-pip autoconf automake libtool meson
```

#### 4.2 Install zimg
```Shell
git clone https://github.com/sekrit-twc/zimg.git --recursive
cd zimg
./autogen.sh
./configure
make && sudo make install
cd ..
```

#### 4.3 Install cython
```Shell
sudo pip3 install Cython
```

#### 4.4 Install VapourSynth
```Shell
git clone https://github.com/vapoursynth/vapoursynth.git
cd vapoursynth
./autogen.sh
./configure
make && sudo make install

# Make sure vapoursynth could be imported from python
# Change "python3.x" depending on your encironment
sudo ln -s /usr/local/lib/python3.x/site-packages/vapoursynth.so /usr/lib/python3.x/lib-dynload/vapoursynth.so
sudo ldconfig
```

#### 4.5 Check if VapourSynth has been installed properly
Make sure you get version number without errors.
```Shell
vspipe --version
```

#### 4.6 [Option] Build vslsmashsource
```Shell
# Install lsmash
git clone https://github.com/l-smash/l-smash.git
cd l-smash
./configure --enable-shared
make && sudo make install
cd ..
 
# Install vslsmashsource
git clone https://github.com/HolyWu/L-SMASH-Works.git
# As the latest version requires more recent ffmpeg libs, checkout the older version
cd L-SMASH-Works
git checkout -b 20200531 refs/tags/20200531
cd VapourSynth
meson build
cd build
ninja && sudo ninja install
cd ../../../
```

</details>

### 5. Add user to proper group
```Shell
# QSV
sudo gpasswd -a ${USER} video
# OpenCL
sudo gpasswd -a ${USER} render
```

### 6. Build QSVEncC
```Shell
git clone https://github.com/rigaya/QSVEnc --recursive
cd QSVEnc
./configure
make
```
Check if it works properly.
```Shell
./qsvencc --check-hw
```

You shall get results below if Quick Sync Video works properly.
```
Success: QuickSyncVideo (hw encoding) available
```


## Intel Drivers for Linux
Please refer to Intel Media SDK Wiki for [driver packages for Linux distributions](https://github.com/Intel-Media-SDK/MediaSDK/wiki/Media-SDK-in-Linux-Distributions) and [further infomation about Intel media stack on Ubuntu](https://github.com/Intel-Media-SDK/MediaSDK/wiki/Intel-media-stack-on-Ubuntu).