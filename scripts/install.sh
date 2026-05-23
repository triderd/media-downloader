#!/bin/bash
# Install media-downloader as system-wide 'mdw' command
# Usage: bash scripts/install.sh [--uninstall]

set -e

CONFIG_DIR="${XDG_CONFIG_HOME:-$HOME/.config}/media-downloader"
BIN_DIR="${HOME}/.local/bin"
BINARY="$BIN_DIR/mdw"

if [ "${1}" = "--uninstall" ]; then
    echo "Removing $BINARY..."
    rm -f "$BINARY"
    echo "Config files left at $CONFIG_DIR (remove manually if desired)"
    echo "Uninstalled."
    exit 0
fi

echo "==> Building..."
cmake -B build -DCMAKE_BUILD_TYPE=Release .
cmake --build build

echo "==> Creating directories..."
mkdir -p "$BIN_DIR"
mkdir -p "$CONFIG_DIR/cookies"

echo "==> Installing binary to $BINARY..."
cp build/media_downloader "$BINARY"
chmod +x "$BINARY"

echo "==> Copying default configs to $CONFIG_DIR..."
[ -f config.json ] && cp -n config.json "$CONFIG_DIR/" || true
[ -f patterns.json ] && cp -n patterns.json "$CONFIG_DIR/" || true
[ -d cookies ] && cp -rn cookies/. "$CONFIG_DIR/cookies/" 2>/dev/null || true

if [[ ":$PATH:" != *":$BIN_DIR:"* ]]; then
    echo ""
    echo "NOTE: Add $BIN_DIR to your PATH if not already:"
    echo "  echo 'export PATH=\"\$HOME/.local/bin:\$PATH\"' >> ~/.bashrc"
    echo "  source ~/.bashrc"
fi

echo ""
echo "Installed! Run: mdw <url>"
echo "Config: $CONFIG_DIR/config.json"
echo "Cookies: $CONFIG_DIR/cookies/"
