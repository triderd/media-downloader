#!/bin/bash
set -e
cd "$(dirname "$0")"

export JAVA_HOME="${JAVA_HOME:-/usr/lib/jvm/java-17-openjdk}"
export PATH="$JAVA_HOME/bin:$PATH"

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

echo "Building..."
./gradlew build
echo ""
echo "Done. Run: java -jar app/build/libs/app.jar"
echo "Or with args: java -jar app/build/libs/app.jar <url>"
