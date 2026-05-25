# media-downloader

CLI media downloader supporting Pixiv, YouTube, Telegram, Danbooru, Gelbooru, and 1000+ sites via yt-dlp.

## Dependencies

- CMake 3.16+, C++20 compiler (gcc 10+ / clang 12+)
- libcurl (development headers)
- nlohmann/json (header-only, >= 3.10)
- yt-dlp (external command, `pip install yt-dlp`)
- ffmpeg, unzip (only needed for Pixiv ugoira animations)

### Linux (Debian/Ubuntu)

```bash
sudo apt install cmake g++ libcurl4-openssl-dev nlohmann-json3-dev ffmpeg unzip
pip install yt-dlp
```

### Linux (Arch)

```bash
sudo pacman -S cmake gcc curl nlohmann-json ffmpeg unzip
pip install yt-dlp
```

### macOS

```bash
brew install cmake curl nlohmann-json ffmpeg unzip
pip install yt-dlp
```

## Build & Run

```bash
cmake -B build
cmake --build build
./build/media_downloader
```

## System-wide Install (`mdw`)

Installs the binary as `mdw` so it can be run from any directory:

```bash
bash scripts/install.sh
```

Config files go to `~/.config/media-downloader/`. Make sure `~/.local/bin` is in your `PATH`:

```bash
echo 'export PATH="$HOME/.local/bin:$PATH"' >> ~/.bashrc
source ~/.bashrc
```

Uninstall:

```bash
bash scripts/install.sh --uninstall
```

Alternatively, install via CMake:

```bash
cmake --install build --prefix /usr/local   # → /usr/local/bin/mdw
```

## Usage

```bash
mdw

mdw "https://t.me/channel/123"

mdw "url1" "url2" "url3"

# From file (one URL per line, # for comments)
mdw -f urls.txt

# Show help
mdw -h
```

## Android APK

Download the latest APK from [Releases](https://github.com/triderd/media-downloader/releases/latest) or build from source:

```bash
cd android
bash build.sh
# → app/build/outputs/apk/debug/app-debug.apk
```

```bash
# Install via adb
adb install android/app/build/outputs/apk/debug/app-debug.apk

# Or just open the APK on your phone
```

**Requirements for building:** JDK 17, Android SDK (platform 35, build-tools 35).

The Android app bundles Python 3.10 + yt-dlp inside the APK. No Termux or external dependencies needed.

## Supported Sites

**Pixiv**(cookies required), **Youtube**, **Telegram**, **Danbooru**(cookies required), **Gelbooru**, **Pornhub**, **1000+ yt-dlp sites**

## Authentication Cookies

Create a `<site>.txt` file in the `cookies/` directory or in `~/.config/media-downloader/cookies/`.

Resolution order: current directory first, then `~/.config/media-downloader/cookies/`.

```bash
# Danbooru — session cookie (F12 → Application → Cookies → _danbooru_session)
echo "_danbooru_session=YOUR_SESSION" > cookies/danbooru.txt

# Pixiv — PHPSESSID (F12 → Application → Cookies → PHPSESSID)
echo "PHPSESSID=YOUR_SESSION" > cookies/pixiv.txt
```

## Configuration

`config.json` (current directory or `~/.config/media-downloader/config.json`):

```json
{
    "download_dir": ".",
    "default_format": "bestvideo+bestaudio/best",
    "concurrent_downloads": 3
}
```

| Field | Description | Default |
|-------|-------------|---------|
| `download_dir` | Download directory (`.` = current) | `.` |
| `default_format` | Default format for yt-dlp | `bestvideo+bestaudio/best` |
| `concurrent_downloads` | Parallel download threads (1 = sequential) | `3` |

## Custom Rules (patterns.json)

Add your own regex-based extractors in `patterns.json`:

```json
[
    {
        "name": "Imgur",
        "url_match": "imgur\\.com/(gallery|a)/",
        "media_patterns": [
            "<meta[^>]+property=\"og:image\"[^>]+content=\"([^\"]+)\"",
            "<img[^>]+src=\"([^\"]+\\.(jpg|png|gif|mp4|webm))\""
        ]
    }
]
```

Rule fields:

| Field | Description |
|-------|-------------|
| `name` | Rule name (for logs) |
| `url_match` | Regex to match the page URL |
| `media_patterns` | List of regexes with a capture group for the media URL |
| `headers` | (optional) Extra request headers |
| `cookies_from` | (optional) Cookie file name for auth |

## License

MIT
