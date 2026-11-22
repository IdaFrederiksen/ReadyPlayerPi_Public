#!/bin/bash
set -e

echo "=== Installer biblioteker ==="
sudo apt install -y cmake nlohmann-json3-dev libasound2-dev qt6-base-dev

echo "=== CMake konfiguration ==="
cd ~/ReadyPlayerPi
mkdir -p build
cd build
cmake .. -DBUILD_HARDWARE_CLIENT=ON

echo "=== Byg projekt ==="
make -j4

echo "=== Starter client ==="
cd ~/ReadyPlayerPi/build
./client-hw $1
