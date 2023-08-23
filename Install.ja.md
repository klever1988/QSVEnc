
# QSVEncCのインストール方法

- [Windows](./Install.ja.md#windows)
- Linux
  - [Linux (Ubuntu 22.04)](./Install.ja.md#linux-ubuntu-2204)
  - [Linux (Ubuntu 20.04)](./Install.ja.md#linux-ubuntu-2004)
  - [Linux (Fedora 32)](./Install.ja.md#linux-fedora-32)
  - その他のLinux OS  
    その他のLinux OS向けには、ソースコードからビルドする必要があります。ビルド方法については、[こちら](./Build.ja.md)を参照してください。


## Windows 

### 1. Intelグラフィックスドライバをインストールします。
### 2. Windows用実行ファイルをダウンロードして展開します。  
実行ファイルは[こちら](https://github.com/rigaya/QSVEnc/releases)からダウンロードできます。QSVEncC_x.xx_Win32.7z が 32bit版、QSVEncC_x.xx_x64.7z が 64bit版です。通常は、64bit版を使用します。

実行時は展開したフォルダからそのまま実行できます。
  
## Linux (Ubuntu 22.04)

### 1. Intel Media ドライバのインストール  
OpenCL関連は[こちらのリンク](https://dgpu-docs.intel.com/installation-guides/ubuntu/ubuntu-jammy-arc.html)に従ってインストールする。

```Shell
sudo apt-get install -y gpg-agent wget
wget -qO - https://repositories.intel.com/graphics/intel-graphics.key | \
  sudo gpg --dearmor --output /usr/share/keyrings/intel-graphics.gpg
echo 'deb [arch=amd64,i386 signed-by=/usr/share/keyrings/intel-graphics.gpg] https://repositories.intel.com/graphics/ubuntu jammy arc' | \
  sudo tee  /etc/apt/sources.list.d/intel.gpu.jammy.list
sudo apt update
sudo apt install intel-media-va-driver-non-free intel-opencl-icd
```

### 2. QSVとOpenCLの使用のため、ユーザーを下記グループに追加
```Shell
# QSV
sudo gpasswd -a ${USER} video
# OpenCL
sudo gpasswd -a ${USER} render
```

### 3. qsvenccのインストール
qsvenccのdebファイルを[こちら](https://github.com/rigaya/QSVEnc/releases)からダウンロードします。

その後、下記のようにインストールします。"x.xx"はインストールするバージョンに置き換えてください。

```Shell
sudo apt install ./qsvencc_x.xx_Ubuntu22.04_amd64.deb
```

### 4. 追加オプション
下記機能を使用するには、追加でインストールが必要です。

- avs読み込み  
  [AvisynthPlus](https://github.com/AviSynth/AviSynthPlus)のインストールが必要です。
  
- vpy読み込み  
  [VapourSynth](https://www.vapoursynth.com/)のインストールが必要です。

### 5. その他

- qsvencc実行時に、"Failed to load OpenCL." というエラーが出る場合  
  /lib/x86_64-linux-gnu/libOpenCL.so が存在することを確認してください。 libOpenCL.so.1 しかない場合は、下記のようにシンボリックリンクを作成してください。
  
  ```Shell
  sudo ln -s /lib/x86_64-linux-gnu/libOpenCL.so.1 /lib/x86_64-linux-gnu/libOpenCL.so
  ```

- qsvenccでFixedFunction(FF)モードが使用できない
- Arc GPU / JasperLake 等でエンコードできない

  原因として、HuCファームウェアがロードされていないことが考えられます。[詳細](https://01.org/linuxgraphics/downloads/firmware)

  FixedFunction(FF)モード(別名 Low Powerモード)を使用するには、HuCファームウェアがロードされている必要があります。
  
  Arc GPU / JasperLakeでは、FFモードしか対応していないため、HuCファームウェアがロードされていないとQSVエンコードできません。

  HuCがロードされているかは、下記で確認できます。
  ```
  sudo cat /sys/kernel/debug/dri/0/i915_huc_load_status
  ```

  HuCのモジュールが存在するかは、下記で確認できます。
  ```
  sudo modinfo i915 | grep -i "huc"
  ```

  ご使用のCPUの世代に該当するモジュールがあれば、HuCファームウェアのロードを有効にすれば
  FixedFunctionモードを利用可能です。

  HuCファームウェアのロードを有効にするには、ファイル```/etc/modprobe.d/i915.conf```にカーネルパラメータを追加し、システムを再起動します。
  ```
  options i915 enable_guc=2
  ```
  
  
## Linux (Ubuntu 20.04)

### 1. Intel Media ドライバのインストール  
OpenCL関連は[こちらのリンク](https://dgpu-docs.intel.com/installation-guides/ubuntu/ubuntu-focal-arc.html)に従ってインストールする。

```Shell
sudo apt-get install -y gpg-agent wget
wget -qO - https://repositories.intel.com/graphics/intel-graphics.key | \
  sudo gpg --dearmor --output /usr/share/keyrings/intel-graphics.gpg
echo 'deb [arch=amd64 signed-by=/usr/share/keyrings/intel-graphics.gpg] https://repositories.intel.com/graphics/ubuntu focal-devel main' | \
  sudo tee  /etc/apt/sources.list.d/intel.gpu.focal.list
sudo apt-get update
sudo apt install intel-media-va-driver-non-free intel-opencl-icd
```

### 2. QSVとOpenCLの使用のため、ユーザーを下記グループに追加
```Shell
# QSV
sudo gpasswd -a ${USER} video
# OpenCL
sudo gpasswd -a ${USER} render
```

### 3. qsvenccのインストール
qsvenccのdebファイルを[こちら](https://github.com/rigaya/QSVEnc/releases)からダウンロードします。

その後、下記のようにインストールします。"x.xx"はインストールするバージョンに置き換えてください。

```Shell
sudo apt install ./qsvencc_x.xx_Ubuntu20.04_amd64.deb
```

### 4. 追加オプション
下記機能を使用するには、追加でインストールが必要です。

- avs読み込み  
  [AvisynthPlus](https://github.com/AviSynth/AviSynthPlus)のインストールが必要です。
  
- vpy読み込み  
  [VapourSynth](https://www.vapoursynth.com/)のインストールが必要です。

### 5. その他

- qsvencc実行時に、"Failed to load OpenCL." というエラーが出る場合  
  /lib/x86_64-linux-gnu/libOpenCL.so が存在することを確認してください。 libOpenCL.so.1 しかない場合は、下記のようにシンボリックリンクを作成してください。
  
  ```Shell
  sudo ln -s /lib/x86_64-linux-gnu/libOpenCL.so.1 /lib/x86_64-linux-gnu/libOpenCL.so
  ```

- qsvenccでFixedFunction(FF)モードが使用できない
- Arc GPU / JasperLake 等でエンコードできない

  原因として、HuCファームウェアがロードされていないことが考えられます。[詳細](https://01.org/linuxgraphics/downloads/firmware)

  FixedFunction(FF)モード(別名 Low Powerモード)を使用するには、HuCファームウェアがロードされている必要があります。
  
  Arc GPU / JasperLakeでは、FFモードしか対応していないため、HuCファームウェアがロードされていないとQSVエンコードできません。

  HuCがロードされているかは、下記で確認できます。
  ```
  sudo cat /sys/kernel/debug/dri/0/i915_huc_load_status
  ```

  HuCのモジュールが存在するかは、下記で確認できます。
  ```
  sudo modinfo i915 | grep -i "huc"
  ```

  ご使用のCPUの世代に該当するモジュールがあれば、HuCファームウェアのロードを有効にすれば
  FixedFunctionモードを利用可能です。

  HuCファームウェアのロードを有効にするには、ファイル```/etc/modprobe.d/i915.conf```にカーネルパラメータを追加し、システムを再起動します。
  ```
  options i915 enable_guc=2
  ```
  

## Linux (Fedora 32)

### 1. Intel Media ドライバとOpenCLランタイムのインストール  

```Shell
#Media
sudo dnf install intel-media-driver
#OpenCL
sudo dnf install -y 'dnf-command(config-manager)'
sudo dnf config-manager --add-repo https://repositories.intel.com/graphics/rhel/8.3/intel-graphics.repo
sudo dnf update --refresh
sudo dnf install intel-opencl intel-media intel-mediasdk level-zero intel-level-zero-gpu
```

### 2. QSVとOpenCLの使用のため、ユーザーを下記グループに追加
```Shell
# QSV
sudo gpasswd -a ${USER} video
# OpenCL
sudo gpasswd -a ${USER} render
```

### 3. qsvenccのインストール
qsvenccのrpmファイルを[こちら](https://github.com/rigaya/QSVEnc/releases)からダウンロードします。

その後、下記のようにインストールします。"x.xx"はインストールするバージョンに置き換えてください。

```Shell
sudo dnf install ./qsvencc_x.xx_1.x86_64.rpm
```

### 4. 追加オプション
下記機能を使用するには、追加でインストールが必要です。

- avs読み込み  
  [AvisynthPlus](https://github.com/AviSynth/AviSynthPlus)のインストールが必要です。
  
- vpy読み込み  
  [VapourSynth](https://www.vapoursynth.com/)のインストールが必要です。

### 5. その他

- qsvencc実行時に、"Failed to load OpenCL." というエラーが出る場合  
  /lib/x86_64-linux-gnu/libOpenCL.so が存在することを確認してください。 libOpenCL.so.1 しかない場合は、下記のようにシンボリックリンクを作成してください。
  
  ```Shell
  sudo ln -s /lib/x86_64-linux-gnu/libOpenCL.so.1 /lib/x86_64-linux-gnu/libOpenCL.so
  ```