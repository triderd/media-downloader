#!/bin/bash
set -e
cd "$(dirname "$0")"

export JAVA_HOME="${JAVA_HOME:-/usr/lib/jvm/java-17-openjdk}"
export PATH="$JAVA_HOME/bin:$PATH"
export ANDROID_SDK_ROOT="${ANDROID_SDK_ROOT:-$HOME/Android/Sdk}"
export ANDROID_HOME="$ANDROID_SDK_ROOT"

if [ ! -f "gradlew" ]; then
    GRADLE_ZIP="gradle-8.13-bin.zip"
    TMP_DIR="/tmp/gradle-bootstrap"
    mkdir -p "$TMP_DIR"
    if [ ! -f "$TMP_DIR/$GRADLE_ZIP" ]; then
        echo "Downloading Gradle (128MB)..."
        curl -L -o "$TMP_DIR/$GRADLE_ZIP" "https://services.gradle.org/distributions/$GRADLE_ZIP"
    fi
    unzip -qo "$TMP_DIR/$GRADLE_ZIP" -d "$TMP_DIR"
    "$TMP_DIR/gradle-8.13/bin/gradle" wrapper --gradle-version 8.13
    echo "Wrapper created."
fi

if [ ! -f "local.properties" ]; then
    echo "sdk.dir=$ANDROID_SDK_ROOT" > local.properties
    echo "local.properties created."
fi

echo "Building Android APK..."
./gradlew assembleDebug
echo ""
echo "Done. APK: app/build/outputs/apk/debug/app-debug.apk"
echo "Install: adb install app/build/outputs/apk/debug/app-debug.apk"
