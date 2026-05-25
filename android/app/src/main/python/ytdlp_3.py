import yt_dlp, json, os, traceback

_ARGS_FILE = "/data/data/com.mediadownloader/files/ytdlp_args.json"
_RESULT_FILE = _ARGS_FILE + ".result"
_PROGRESS_FILE = _ARGS_FILE + ".progress"

if os.path.exists(_ARGS_FILE):
    with open(_ARGS_FILE) as f:
        _args = json.load(f)
    os.remove(_ARGS_FILE)

    if os.path.exists(_PROGRESS_FILE):
        os.remove(_PROGRESS_FILE)

    def progress_hook(d):
        try:
            if d['status'] == 'downloading':
                total = d.get('total_bytes') or d.get('total_bytes_estimate') or 0
                downloaded = d.get('downloaded_bytes', 0)
                speed = d.get('speed') or 0
                eta = d.get('eta') or 0
                if total > 0:
                    pct = downloaded / total
                    with open(_PROGRESS_FILE, 'w') as pf:
                        json.dump({
                            'pct': pct,
                            'downloaded': downloaded,
                            'total': total,
                            'speed': speed,
                            'eta': eta
                        }, pf)
            elif d['status'] == 'finished':
                with open(_PROGRESS_FILE, 'w') as pf:
                    json.dump(
                        {'pct': 1.0, 'downloaded': d.get('total_bytes', 0),
                         'total': d.get('total_bytes', 0), 'speed': 0, 'eta': 0},
                        pf)
        except:
            pass

    code, err, title, thumbnail = 0, "", "", ""
    try:
        ydl = yt_dlp.YoutubeDL({
            'format': _args.get("fmt", "bestvideo+bestaudio/best"),
            'outtmpl': {'default': _args.get("tmpl", "%(title)s.%(ext)s")},
            'quiet': True,
            'noprogress': True,
            'progress_hooks': [progress_hook],
            'socket_timeout': 30,
            'http_headers': {
                'User-Agent': 'Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/136.0.0.0 Safari/537.36',
                'Accept': 'text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8',
                'Accept-Language': 'en-US,en;q=0.9',
            },
        })

        try:
            info = ydl.extract_info(_args["url"], download=False)
            title = info.get('title', '') if info else ''
            thumbnail = info.get('thumbnail', '') if info else ''
        except:
            pass

        with open(_PROGRESS_FILE, 'w') as pf:
            json.dump({'pct': 0, 'title': title, 'thumbnail': thumbnail,
                       'downloaded': 0, 'total': 0, 'speed': 0, 'eta': 0}, pf)

        ydl.download([_args["url"]])
    except SystemExit as e:
        code = e.code if e.code is not None else 0
    except Exception as e:
        code = 1
        err = type(e).__name__ + ": " + str(e) + "\n" + traceback.format_exc()

    with open(_RESULT_FILE, "w") as f:
        json.dump({"code": code, "err": err, "title": title, "thumbnail": thumbnail}, f)
