# ORB-SLAM3 on NVIDIA Jetson Xavier NX

This repository documents the complete setup and installation process for running **ORB-SLAM3** with an **Intel RealSense camera** on the **NVIDIA Jetson Xavier NX** platform.

The implementation in this repository is based on the official **ORB-SLAM3** codebase provided by the **UZ-SLAMLab**.  
Minor modifications and bug fixes were applied to improve stability and compatibility with the NVIDIA Jetson platform.
<br>**Official ORB-SLAM3 repository:**  
https://github.com/UZ-SLAMLab/ORB_SLAM3

The Jetson system setup and dependency installation were guided by the following reference:
<br>**Jetson setup and ORB-SLAM Installation reference:**  
https://nemo.cool/1160.html

---
## System Overview

- Platform: NVIDIA Jetson Xavier NX  
- JetPack Version: 5.0.2  
- Visual SLAM: ORB-SLAM3  
- Depth Camera: Intel RealSense  
- SDK: Intel librealsense (built from source)  
- Language: C++ (C++14)

---

## Jetson Setup

```bash
# Update system
sudo apt update
sudo apt upgrade

# Install JetPack components
sudo apt install nvidia-jetpack-dev

# Verify installation
sudo jetson_clocks --show
sudo tegrastats

# Install useful tools
sudo apt install -y htop git build-essential
sudo apt install -y curl wget cmake pkg-config

# Set maximum performance mode
sudo nvpmodel -m 0
sudo jetson_clocks
```

#### Optional : Jetson Optimization Script 
```bash
cat > ~/optimize_jetson.sh << 'EOF'
#!/bin/bash
echo "Optimizing Jetson Xavier NX..."
sudo nvpmodel -m 0
sudo jetson_clocks
echo 3 | sudo tee /proc/sys/vm/drop_caches
echo "Optimization complete!"
EOF

chmod +x ~/optimize_jetson.sh
```
Run using `~/optimize_jetson.sh`

---

## Intel RealSense SDK (librealsense) Installation 
The RealSense is **built from source** for compatibility and performance on Jetson ARM architecture.
```bash
# Install dependencies
sudo apt-get install -y git libssl-dev libusb-1.0-0-dev pkg-config libgtk-3-dev
sudo apt-get install -y libglfw3-dev libgl1-mesa-dev libglu1-mesa-dev

# Clone librealsense
cd ~
git clone https://github.com/IntelRealSense/librealsense.git
cd librealsense

# Copy udev rules and reload rules
sudo cp config/99-realsense-libusb.rules /etc/udev/rules.d/
sudo udevadm control --reload-rules
sudo udevadm trigger

# Build and Install SDK 
mkdir build && cd build
cmake ../ -DBUILD_EXAMPLES=true -DCMAKE_BUILD_TYPE=Release
make -j$(nproc) # compile using all the available cores
sudo make install
sudo ldconfig   # Refresh shared library cache
```
You can verify the installation by running `realsense-viewer`.


---

## ORB-SLAM3 Installation on Jetson 
#### Install System Dependencies 
```bash
sudo apt install -y \
  libgtk2.0-dev \
  libavcodec-dev libavformat-dev libswscale-dev \
  libtbb2 libtbb-dev \
  libjpeg-dev libpng-dev libtiff-dev \
  libglew-dev libboost-all-dev libssl-dev \
  libeigen3-dev \
  libgl1-mesa-dev libegl1-mesa-dev libwayland-dev \
  libopencv-dev \
  python3-dev python3-numpy python3-pip
```

#### Pangolin Installation (C++14 Compatible)
```bash
cd ~
git clone https://github.com/stevenlovegrove/Pangolin.git
cd Pangolin
mkdir build && cd build

cmake .. \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_TOOLS=OFF \
  -DBUILD_EXAMPLES=OFF \
  -DBUILD_TESTS=OFF \
  -DBUILD_PANGOLIN_PYTHON=OFF \
  -DCMAKE_CXX_FLAGS="-w -Wno-error" \
  -DCMAKE_C_FLAGS="-w -Wno-error"

make -j2
sudo make install
sudo ldconfig
```

#### ORB-SLAM3 Clone Installation 
```bash
# clone and prepare ORB-SLAM3 
cd ~
git clone https://github.com/S-Bhargavii/ORB_SLAM3.git
cd ORB_SLAM3

# Fix Eigen paths for ARM architecture
chmod +x build.sh
sed -i 's/Eigen\//eigen3\/Eigen\//g' $(find . -type f -name "*.h" -o -name "*.cc" -o -name "*.cpp")

# Fix incorrect unsupported Eigen path
find . -type f \( -name "*.cc" -o -name "*.cpp" -o -name "*.h" \) -exec sed -i 's|unsupported/eigen3/Eigen|unsupported/Eigen|g' {} \;
```

#### Install Third-party Libraries 
```bash
# Build DBoW2
cd Thirdparty/DBoW2
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j2
cd ../../..

# Build g2o
cd Thirdparty/g2o
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j2
cd ../../..

# Build Sophus (optional, skip if errors)
if [ -d "Thirdparty/Sophus" ]; then
    cd Thirdparty/Sophus
    mkdir build && cd build
    cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=OFF
    make -j2 || echo "Sophus build failed, continuing without it"
    cd ../../..
fi

# Extract vocabulary
cd Vocabulary
tar -xf ORBvoc.txt.tar.gz
cd ..
```
#### ORB-SLAM3 Build Installation 
```bash
# Build ORB_SLAM3 with C++14 support
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_STANDARD=14
make -j2
```
