#!/bin/bash
set -e

if [ -z "$ANDROID_NDK" ]; then
    echo "ERROR: Set ANDROID_NDK to your NDK path"
    echo "  export ANDROID_NDK=/path/to/android-ndk-r27"
    exit 1
fi

NDK="$ANDROID_NDK"
HOST_TAG="linux-x86_64"
TOOLCHAIN="$NDK/toolchains/llvm/prebuilt/$HOST_TAG"

if [ ! -d "$TOOLCHAIN" ]; then
    echo "ERROR: NDK toolchain not found at $TOOLCHAIN"
    exit 1
fi

API=24
ABI=arm64-v8a
TARGET="aarch64-linux-android"
SYSROOT="$TOOLCHAIN/sysroot"
PREFIX="$PWD/build_android_libs"
export CC="$TOOLCHAIN/bin/${TARGET}${API}-clang"
export CXX="$TOOLCHAIN/bin/${TARGET}${API}-clang++"
export AR="$TOOLCHAIN/bin/llvm-ar"
export RANLIB="$TOOLCHAIN/bin/llvm-ranlib"
export STRIP="$TOOLCHAIN/bin/llvm-strip"

export CFLAGS="-fPIC"
export CXXFLAGS="-fPIC"

echo "==> Building libcurl for Android..."

CURL_VER="8.12.1"
CURL_DIR="$PWD/build_curl"

if [ ! -f "$PREFIX/lib/libcurl.a" ]; then
    if [ ! -d "$CURL_DIR" ]; then
        curl -L "https://curl.se/download/curl-${CURL_VER}.tar.xz" -o "/tmp/curl-${CURL_VER}.tar.xz"
        tar -xf "/tmp/curl-${CURL_VER}.tar.xz" -C "$PWD"
        mv "$PWD/curl-${CURL_VER}" "$CURL_DIR"
    fi

    cd "$CURL_DIR"
    ./configure \
        --host="$TARGET" \
        --prefix="$PREFIX" \
        --disable-shared \
        --enable-static \
        --with-openssl \
        --disable-ldap \
        --disable-ldaps \
        --disable-rtsp \
        --disable-dict \
        --disable-tftp \
        --disable-pop3 \
        --disable-imap \
        --disable-smtp \
        --disable-gopher \
        --disable-mqtt \
        --disable-manual \
        --without-zstd \
        --without-brotli \
        --without-libpsl \
        --without-nghttp2 \
        --without-nghttp3 \
        --without-ngtcp2

    make -j$(nproc)
    make install
    cd "$OLDPWD"
    echo "==> libcurl built"
else
    echo "==> libcurl already built"
fi

echo "==> Configuring CMake for Android..."

cmake -B build_android \
    -DCMAKE_TOOLCHAIN_FILE="$NDK/build/cmake/android.toolchain.cmake" \
    -DANDROID_ABI="$ABI" \
    -DANDROID_PLATFORM="android-${API}" \
    -DANDROID_STL=c++_shared \
    -DCMAKE_BUILD_TYPE=Release \
    -DCURL_INCLUDE_DIR="$PREFIX/include" \
    -DCURL_LIBRARY="$PREFIX/lib/libcurl.a" \
    -DCMAKE_FIND_ROOT_PATH_MODE_PACKAGE=NEVER \
    .

echo "==> Building..."
cmake --build build_android

echo "==> Binary: build_android/media_downloader"
echo "==> Size: $(du -h build_android/media_downloader | cut -f1)"
echo "==> Push to device: adb push build_android/media_downloader /data/local/tmp/"
