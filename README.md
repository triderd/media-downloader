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

### Termux (Android)

```bash
pkg install cmake clang curl libcurl nlohmann-json python ffmpeg unzip
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
# Interactive mode (type URL when prompted)
mdw

# Single URL as argument
mdw "https://t.me/channel/123"

# Multiple URLs
mdw "url1" "url2" "url3"

# From file (one URL per line, # for comments)
mdw -f urls.txt

# Show help
mdw -h
```

## Supported Sites

| Site | Method | Auth file |
|------|--------|-----------|
| **Pixiv** | AJAX API, illustrations + ugoira with correct FPS | `cookies/pixiv.txt` |
| **YouTube** | yt-dlp for format listing & download, playlists | — |
| **Telegram** | t.me/s/ page scraping, photos + albums | — |
| **Danbooru** | JSON API, Cloudflare bypass via descriptive URL | `cookies/danbooru.txt` |
| **Gelbooru** | Page scraping (API blocked), CDN sample download | not required |
| **Pattern** | User-defined regex rules (patterns.json) | optional |
| **Everything else** | yt-dlp backend (1000+ sites) | — |

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

## Architecture

Extractors are tried in priority order — the first one that matches the URL wins:

```
Pixiv → YouTube → Telegram → Danbooru → Pattern → YtDlp (catch-all)
```

- **Pixiv** — AJAX API calls, parses JSON for illustration pages and ugoira metadata. Ugoira frames are extracted with correct per-frame delays via ffmpeg concat demuxer.
- **YouTube** — delegates format listing and download to yt-dlp. Playlist video IDs are scraped from the page HTML.
- **Telegram** — fetches t.me/s/ static HTML, filters photo elements by post ID in the href attribute.
- **Danbooru** — JSON API. For files behind Cloudflare (original CDN), constructs a descriptive URL from tags.
- **Gelbooru** — page HTML scraping (API blocked behind auth). Extracts `<img id="image">` from the post page.
- **Pattern** — user-defined regex rules from `patterns.json`. Only matches URLs with matching rules.
- **YtDlp** — delegates to yt-dlp for everything else. Covers 1000+ sites.

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
