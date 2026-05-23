# media-downloader

CLI-загрузчик медиа с Pixiv, YouTube, Telegram, Danbooru, Gelbooru и 1000+ сайтов через yt-dlp.

## Зависимости

- CMake 3.16+, компилятор C++20 (gcc 10+ / clang 12+)
- libcurl (dev-пакет, нужен `curl/curl.h`)
- nlohmann/json (header-only, >= 3.10)
- yt-dlp (внешняя команда, `pip install yt-dlp`)
- ffmpeg, unzip (только для ugoira-анимаций с Pixiv)

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

## Сборка и запуск

```bash
cmake -B build
cmake --build build
./build/media_downloader
```

## Установка как системная команда (`mdw`)

После установки команда `mdw <url>` работает из любого каталога.

```bash
bash scripts/install.sh
```

Бинарник ставится в `~/.local/bin/mdw`, конфиги — в `~/.config/media-downloader/`.

Добавь `~/.local/bin` в PATH если ещё нет:

```bash
echo 'export PATH="$HOME/.local/bin:$PATH"' >> ~/.bashrc
source ~/.bashrc
```

Удаление:

```bash
bash scripts/install.sh --uninstall
```

Также можно установить через CMake:

```bash
cmake --install build --prefix /usr/local   # → /usr/local/bin/mdw
```

## Использование

```bash
# Интерактивный режим (ввод URL с клавиатуры)
mdw

# Один URL аргументом
mdw "https://t.me/channel/123"

# Несколько URL подряд
mdw "url1" "url2" "url3"

# Из файла (один URL на строку, # — комментарий)
mdw -f urls.txt

# Справка
mdw -h
```

## Поддерживаемые сайты

| Сайт | Как работает | Cookie-файл |
|------|-------------|-------------|
| **Pixiv** | AJAX API, иллюстрации + ugoira с правильным FPS | `cookies/pixiv.txt` |
| **YouTube** | yt-dlp для форматов и загрузки, плейлисты | — |
| **Telegram** | t.me/s/ парсинг, фото + альбомы | — |
| **Danbooru** | JSON API, обход Cloudflare через descriptive URL | `cookies/danbooru.txt` |
| **Gelbooru** | JSON API | `cookies/gelbooru.txt` |
| **Pattern** | Пользовательские regex-правила (patterns.json) | по желанию |
| **Всё остальное** | yt-dlp backend (1000+ сайтов) | — |

## Куки для авторизации

Создай файл `<сайт>.txt` в папке `cookies/` или в `~/.config/media-downloader/cookies/`.

Поиск: сначала текущая папка, потом `~/.config/media-downloader/cookies/`.

```bash
# Danbooru — сессионная кука (F12 → Application → Cookies → _danbooru_session)
echo "_danbooru_session=ТВОЯ_СЕССИЯ" > cookies/danbooru.txt

# Pixiv — PHPSESSID (F12 → Application → Cookies → PHPSESSID)
echo "PHPSESSID=ТВОЯ_СЕССИЯ" > cookies/pixiv.txt
```

## Конфигурация

Файл `config.json` (в текущей папке или `~/.config/media-downloader/config.json`):

```json
{
    "download_dir": ".",
    "default_format": "bestvideo+bestaudio/best",
    "concurrent_downloads": 3
}
```

| Поле | Описание | По умолчанию |
|------|---------|-------------|
| `download_dir` | Папка для загрузок (`.` = текущая) | `.` |
| `default_format` | Формат по умолчанию для yt-dlp | `bestvideo+bestaudio/best` |
| `concurrent_downloads` | Число одновременных загрузок | `3` |

## Кастомные правила (patterns.json)

Можно добавить свои regex-правила для любых сайтов (в `patterns.json`):

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

Поля правила:

| Поле | Описание |
|------|---------|
| `name` | Название (для логов) |
| `url_match` | Regex — совпадение с URL |
| `media_patterns` | Список regex с capture-группой для URL медиа |
| `headers` | (опционально) Доп. заголовки запроса |
| `cookies_from` | (опционально) Имя cookie-файла для авторизации |

## Лицензия

MIT
