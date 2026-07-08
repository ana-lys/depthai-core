#!/bin/bash
set -e

echo "===================================================="
echo "    DepthAI Core (C++) - Jetson Auto-Installer      "
echo "===================================================="

# 1. Update and install core APT dependencies
echo "[1/4] Installing dependencies via apt-get..."
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    cmake \
    git \
    pkg-config \
    libusb-1.0-0-dev \
    libopencv-dev

# 2. Clone the repository (if not already in a depthai-core folder)
if [ ! -d "depthai-core" ]; then
    echo "[2/4] Cloning depthai-core repository..."
    git clone https://github.com/luxonis/depthai-core.git --branch main --recursive
    cd depthai-core
else
    echo "[2/4] depthai-core directory already exists. Skipping clone."
    # If we are already inside the folder, or it's in the current dir
    if [ -f "CMakeLists.txt" ]; then
        echo "Already inside depthai-core folder."
    else
        cd depthai-core
    fi
fi

# 3. Configure the build with CMake
echo "[3/4] Configuring CMake..."
# Enable OpenCV, and build the examples
cmake -S . -B build \
    -D DEPTHAI_BUILD_EXAMPLES=ON \
    -D DEPTHAI_OPENCV_USE_SYSTEM=ON \
    -D CMAKE_BUILD_TYPE=Release

# 4. Compile the SDK and examples
# Note: Since the AGX Orin has 32GB of RAM, -j6 is the sweet spot. 
# It will use about ~18GB-24GB of RAM during peak template compilation, keeping 
# a safe buffer so the OS doesn't OOM crash, while drastically speeding up the build!
echo "[4/4] Compiling depthai-core with 6 threads (-j6)..."
echo "Grab a coffee, this will take about 10-15 minutes on an AGX Orin!"
cmake --build build -j6

echo "===================================================="
echo "    Done! DepthAI is fully installed and built!     "
echo "===================================================="
