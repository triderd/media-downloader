#include "downloader.hpp"
#include "extractors/extractor_manager.hpp"
#include "ui.hpp"
#include "ytdlp_backend.hpp"
#include "config.hpp"

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <filesystem>
#include <future>
#include <mutex>

static std::mutex cout_mutex;

static void safe_cout(
    const std::string& line
)
{
    std::lock_guard<std::mutex> lock(
        cout_mutex
    );

    std::cout << line << std::flush;
}

static std::pair<int, int>
process_url(
    const std::string& url
)
{
    ExtractorManager manager;

    Downloader downloader;

    if (
        url.ends_with(".jpg")
        ||
        url.ends_with(".png")
        ||
        url.ends_with(".jpeg")
        ||
        url.ends_with(".gif")
        ||
        url.ends_with(".webp")
        ||
        url.ends_with(".mp4")
        ||
        url.ends_with(".webm")
    )
    {
        DownloadTask task;

        task.url = url;

        size_t last_slash =
            url.find_last_of('/');

        if (last_slash != std::string::npos)
        {
            task.filename =
                url.substr(
                    last_slash + 1
                );
        }
        else
        {
            task.filename =
                "downloaded_file";
        }

        bool success =
            downloader.download(
                task.url,
                task.filename,
                task.headers,
                task.cookies
            );

        if (!success)
        {
            std::cout
                << "Download failed\n";
        }

        return {
            success ? 1 : 0,
            success ? 0 : 1
        };
    }

    UI::info(
         "Extracting media URLs..."
    );

    std::vector<DownloadTask> tasks =
        manager.extract(url);

    if (tasks.empty())
    {
        std::cout
            << "No media found\n";

        return { 0, 1 };
    }

    bool is_playlist = tasks.size() > 1;

    std::string base_dir =
        Config::get_download_dir();

    std::string download_folder;

    if (is_playlist)
    {
        download_folder =
            base_dir
            + "/"
            +
            downloader.sanitize_filename(
                tasks[0].playlist_title
            );

        std::filesystem::create_directories(
            download_folder
        );
    }

    UI::success(
        "Found "
        +
        std::to_string(
            tasks.size()
        )
        +
        " media files"
    );

    int concurrent =
        Config::get_concurrent_downloads();

    if (concurrent < 1)
    {
        concurrent = 1;
    }

    int completed = 0;
    int failed = 0;

    if (concurrent == 1 || tasks.size() == 1)
    {
        for (
            size_t i = 0;
            i < tasks.size();
            ++i
        )
        {
            const auto& task =
                tasks[i];

            UI::separator();

            std::cout
                << task.url
                << "\n";

            std::string filename;

            if (download_folder.empty())
            {
                filename =
                    downloader.sanitize_filename(
                        task.filename
                    );
            }
            else
            {
                filename =
                    download_folder
                    + "/"
                    +
                    downloader.sanitize_filename(
                        task.filename
                    );
            }

            bool success;

            if (task.use_ytdlp)
            {
                if (is_playlist)
                {
                    success =
                        YtDlpBackend::download(
                            url,
                            task.format_id,
                            "",
                            download_folder
                        );
                }
                else
                {
                    success =
                        YtDlpBackend::download(
                            task.url,
                            task.format_id
                        );
                }
            }
            else
            {
                success =
                    downloader.download(
                        task.url,
                        filename,
                        task.headers,
                        task.cookies,
                        task.frame_delays
                    );
            }

            if (success)
            {
                ++completed;
            }
            else
            {
                ++failed;

                UI::error(
                    "Failed: "
                    + task.url
                );
            }
        }
    }
    else
    {
        std::vector<std::future<bool>> futures;

        for (
            size_t i = 0;
            i < tasks.size();
            ++i
        )
        {
            const auto& task =
                tasks[i];

            std::string filename;

            if (download_folder.empty())
            {
                filename =
                    downloader.sanitize_filename(
                        task.filename
                    );
            }
            else
            {
                filename =
                    download_folder
                    + "/"
                    +
                    downloader.sanitize_filename(
                        task.filename
                    );
            }

            futures.push_back(
                std::async(
                    std::launch::async,
                    [
                        &task,
                        filename,
                        &url,
                        is_playlist,
                        &download_folder
                    ]() -> bool
                    {
                        Downloader local_downloader;

                        safe_cout(
                            "\033[90m"
                            + std::to_string(
                                task.playlist_index
                            )
                            + "/"
                            + std::to_string(
                                task.playlist_total
                            )
                            + " Downloading: "
                            + task.url
                            + "\033[0m\n"
                        );

                        bool success;

                        if (task.use_ytdlp)
                        {
                            if (is_playlist)
                            {
                                success =
                                    YtDlpBackend::download(
                                        url,
                                        task.format_id,
                                        "",
                                        download_folder
                                    );
                            }
                            else
                            {
                                success =
                                    YtDlpBackend::download(
                                        task.url,
                                        task.format_id
                                    );
                            }
                        }
                        else
                        {
                            success =
                                local_downloader.download(
                                    task.url,
                                    filename,
                                    task.headers,
                                    task.cookies,
                                    task.frame_delays,
                                    3,
                                    false
                                );
                        }

                        if (!success)
                        {
                            safe_cout(
                                "\033[1;31mFailed: "
                                + task.url
                                + "\033[0m\n"
                            );
                        }

                        return success;
                    }
                )
            );
        }

        for (auto& future : futures)
        {
            if (future.get())
            {
                ++completed;
            }
            else
            {
                ++failed;
            }
        }
    }

    return { completed, failed };
}

static void print_usage()
{
    std::cout
        << "Media Downloader\n\n"
        << "Usage:\n"
        << "  media_downloader                  Interactive mode (read URL from stdin)\n"
        << "  media_downloader <url>            Download a single URL\n"
        << "  media_downloader <url1> <url2>... Download multiple URLs\n"
        << "  media_downloader -f <file>        Read URLs from file (one per line)\n"
        << "  media_downloader -h, --help       Show this help\n\n"
        << "Supported sites:\n"
        << "  Pixiv, YouTube, Telegram, Danbooru, Gelbooru\n"
        << "  + 1000+ sites via yt-dlp backend\n"
        << "  + Custom sites via patterns.json\n";
}

static std::vector<std::string>
read_urls_from_file(
    const std::string& path
)
{
    std::vector<std::string> urls;

    std::ifstream file(path);

    if (!file.is_open())
    {
        std::cerr
            << "Cannot open file: "
            << path
            << "\n";

        return urls;
    }

    std::string line;

    while (
        std::getline(
            file,
            line
        )
    )
    {
        while (
            !line.empty()
            &&
            (
                line.front() == ' '
                ||
                line.front() == '\t'
            )
        )
        {
            line.erase(0, 1);
        }

        while (
            !line.empty()
            &&
            (
                line.back() == ' '
                ||
                line.back() == '\t'
                ||
                line.back() == '\r'
                ||
                line.back() == '\n'
            )
        )
        {
            line.pop_back();
        }

        if (
            line.empty()
            ||
            line.front() == '#'
        )
        {
            continue;
        }

        urls.push_back(line);
    }

    return urls;
}

int main(
    int argc,
    char* argv[]
)
{
    Config::load();

    UI::title(
        "Media Downloader"
    );

    std::vector<std::string> urls;
    bool from_stdin = false;

    for (
        int i = 1;
        i < argc;
        ++i
    )
    {
        std::string arg =
            argv[i];

        if (
            arg == "-h"
            ||
            arg == "--help"
        )
        {
            print_usage();

            return 0;
        }

        if (
            arg == "-f"
        )
        {
            if (i + 1 < argc)
            {
                ++i;

                auto file_urls =
                    read_urls_from_file(
                        argv[i]
                    );

                urls.insert(
                    urls.end(),
                    file_urls.begin(),
                    file_urls.end()
                );
            }
            else
            {
                std::cerr
                    << "Missing filename after -f\n";

                return 1;
            }

            continue;
        }

        if (
            arg.starts_with("http://")
            ||
            arg.starts_with("https://")
            ||
            arg.find(".")
            != std::string::npos
        )
        {
            urls.push_back(arg);
        }
    }

    if (urls.empty() && argc == 1)
    {
        std::string line;

        std::cout
            << "Page URL: ";

        std::getline(
            std::cin,
            line
        );

        if (line.empty())
        {
            return 0;
        }

        urls.push_back(line);
        from_stdin = true;
    }

    if (urls.empty())
    {
        std::cerr
            << "No URLs provided\n";

        print_usage();

        return 1;
    }

    int total_completed = 0;
    int total_failed = 0;

    for (
        size_t i = 0;
        i < urls.size();
        ++i
    )
    {
        if (
            urls.size() > 1
            &&
            !from_stdin
        )
        {
            UI::separator();

            std::cout
                << "URL "
                << i + 1
                << "/"
                << urls.size()
                << ": "
                << urls[i]
                << "\n";
        }

        auto [ok, fail] =
            process_url(
                urls[i]
            );

        total_completed += ok;
        total_failed += fail;
    }

    if (
        urls.size() > 1
        ||
        total_completed + total_failed > 1
    )
    {
        std::cout << "\n";

        UI::success(
            "Total: "
            + std::to_string(
                total_completed
            )
            + " completed, "
            + std::to_string(
                total_failed
            )
            + " failed"
        );
    }

    return
        total_failed > 0
        ? 1
        : 0;
}
