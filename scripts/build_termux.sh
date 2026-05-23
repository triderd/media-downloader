#!/data/data/com.termux/files/usr/bin/bash
# Termux native build for media-downloader (aarch64)
# Usage: bash scripts/build_termux.sh

set -e

echo "==> Installing dependencies..."
pkg update
pkg install -y cmake make clang curl libcurl nlohmann-json python

echo "==> Installing yt-dlp..."
pip install yt-dlp

echo "==> Configuring CMake..."
cmake -B build_termux \
    -DCMAKE_BUILD_TYPE=Release \
    .

echo "==> Building..."
cmake --build build_termux

echo "==> Binary: ./build_termux/media_downloader"
echo "==> Run: ./build_termux/media_downloader"
