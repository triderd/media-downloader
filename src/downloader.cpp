#include "downloader.hpp"

#include <curl/curl.h>

#include <filesystem>
#include <fstream>
#include <chrono>
#include <iostream>
#include <iomanip>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <thread>
#include <algorithm>


std::chrono::steady_clock::time_point
Downloader::last_progress_update =
    std::chrono::steady_clock::now();

std::chrono::steady_clock::time_point
Downloader::download_start_time =
    std::chrono::steady_clock::now();

bool Downloader::progress_enabled =
    true;



size_t Downloader::write_data(
    void* ptr,
    size_t size,
    size_t nmemb,
    void* userdata
)
{
    FILE* stream = static_cast<FILE*>(userdata);

    return fwrite(ptr, size, nmemb, stream);
}



int Downloader::progress_callback(
    void* clientp,
    curl_off_t dltotal,
    curl_off_t dlnow,
    curl_off_t ultotal,
    curl_off_t ulnow
)
{
    if (!progress_enabled)
    {
        return 0;
    }

    if (dltotal <= 0)
    {
        return 0;
    }

    auto now =
        std::chrono::steady_clock::now();

    auto elapsed =
        std::chrono::duration_cast<
            std::chrono::milliseconds
        >(
            now - last_progress_update
        );

    if (elapsed.count() < 250)
    {
        return 0;
    }

    last_progress_update = now;

    double percent =
        (static_cast<double>(dlnow)
        / static_cast<double>(dltotal))
        * 100.0;

    double downloaded_mb =
        static_cast<double>(dlnow)
        / 1024.0 / 1024.0;

    double total_mb =
        static_cast<double>(dltotal)
        / 1024.0 / 1024.0;

    std::cout
        << "\r["
        << std::fixed
        << std::setprecision(1)
        << percent
        << "%] "
        << downloaded_mb
        << " MB / "
        << total_mb
        << " MB"
        << std::flush;

    return 0;
}


size_t Downloader::header_callback(
    char* buffer,
    size_t size,
    size_t nitems,
    void* userdata
)
{
    size_t total_size =
        size * nitems;

    std::string header(
        buffer,
        total_size
    );

    DownloadContext* context =
        static_cast<DownloadContext*>(userdata);

    std::string prefix =
        "Content-Disposition:";

    if (header.find(prefix) != std::string::npos)
    {
        size_t filename_pos =
            header.find("filename=");

        if (filename_pos != std::string::npos)
        {
            std::string filename =
                header.substr(
                    filename_pos + 9
                );

            if (!filename.empty())
            {
                if (filename.front() == '"')
                {
                    filename.erase(0, 1);
                }

                if (!filename.empty() &&
                    filename.back() == '\n')
                {
                    filename.pop_back();
                }

                if (!filename.empty() &&
                    filename.back() == '\r')
                {
                    filename.pop_back();
                }

                if (!filename.empty() &&
                    filename.back() == '"')
                {
                    filename.pop_back();
                }

                context->filename =
                    filename;
            }
        }
    }

    return total_size;
}



std::string Downloader::get_filename_from_url(
    const std::string& url
)
{
    size_t last_slash =
        url.find_last_of('/');

    if (last_slash == std::string::npos)
    {
        return "downloaded_file";
    }

    std::string filename =
        url.substr(last_slash + 1);

    if (filename.empty())
    {
        return "downloaded_file";
    }

    return filename;
}




bool Downloader::download(
    const std::string& url,
    const std::string& output_file,
    const std::vector<std::string>& headers,
    const std::string& cookies,
    const std::vector<int>& frame_delays,
    int max_retries,
    bool show_progress
)
{
    progress_enabled = show_progress;

    std::string final_name =
        output_file;

    if (final_name.empty())
    {
        final_name =
            get_filename_from_url(url);
    }

    std::string final_filename =
        output_file;

    if (final_filename.empty())
    {
        final_filename =
            "downloaded_file";
    }

    bool http_success = false;

    for (
        int attempt = 1;
        attempt <= max_retries;
        ++attempt
    )
    {
        if (attempt > 1)
        {
            std::cerr
                << "Retry "
                << attempt
                << "/"
                << max_retries
                << " for "
                << url
                << "\n";

            std::this_thread::sleep_for(
                std::chrono::milliseconds(
                    attempt * 1000
                )
            );

            std::remove(
                final_filename.c_str()
            );
        }

        CURL* curl = curl_easy_init();

        download_start_time =
            std::chrono::steady_clock::now();

        if (!curl)
        {
            std::cerr << "Failed to init curl\n";
            continue;
        }

        FILE* file = fopen(
            final_name.c_str(),
            "wb"
        );

        if (!file)
        {
            std::cerr << "Failed to open file\n";
            curl_easy_cleanup(curl);
            continue;
        }

        DownloadContext context;

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_PATH_AS_IS, 1L);

        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 0L);
        curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, 1L);
        curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, 60L);
        curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "");
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 20L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, file);
        curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);
        curl_easy_setopt(curl, CURLOPT_HEADERDATA, &context);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/136.0.0.0 Safari/537.36");
        curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progress_callback);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);

        curl_slist* header_list = nullptr;

        for (const auto& header : headers)
        {
            header_list =
                curl_slist_append(
                    header_list,
                    header.c_str()
                );
        }

        header_list = curl_slist_append(header_list, "Accept: image/avif,image/webp,image/apng,image/*,*/*;q=0.8");
        header_list = curl_slist_append(header_list, "Accept-Language: en-US,en;q=0.9");
        header_list = curl_slist_append(header_list, "Connection: keep-alive");
        header_list = curl_slist_append(header_list, "Sec-Fetch-Dest: image");

        if (header_list)
        {
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header_list);

            if (!cookies.empty())
            {
                curl_easy_setopt(curl, CURLOPT_COOKIE, cookies.c_str());
            }
        }

        CURLcode result = curl_easy_perform(curl);

        long response_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

        if (response_code >= 400)
        {
            std::cerr
                << "HTTP error: "
                << response_code
                << "\n";

            fclose(file);
            curl_slist_free_all(header_list);
            curl_easy_cleanup(curl);
            continue;
        }

        char* content_type = nullptr;
        curl_easy_getinfo(curl, CURLINFO_CONTENT_TYPE, &content_type);

        if (content_type && !is_media_content_type(content_type))
        {
            std::cerr
                << "Invalid content type: "
                << content_type
                << "\n";

            fclose(file);
            std::remove(final_filename.c_str());
            curl_slist_free_all(header_list);
            curl_easy_cleanup(curl);
            continue;
        }

        fclose(file);
        curl_slist_free_all(header_list);
        curl_easy_cleanup(curl);

        if (result != CURLE_OK)
        {
            std::cerr
                << "Download failed\n"
                << "CURL error: "
                << curl_easy_strerror(result)
                << "\n";

            continue;
        }

        http_success = true;
        break;
    }

    if (!http_success)
    {
        std::remove(
            final_filename.c_str()
        );

        return false;
    }


    if (
        final_filename.ends_with(
            ".zip"
        )
    )
    {
        if (
            !is_path_safe_for_shell(
                final_filename
            )
        )
        {
            std::cerr
                << "Unsafe filename, skipping ZIP extraction\n";

            return false;
        }

        std::string folder_name =
            final_filename
            + "_frames";

        std::string unzip_cmd =
            "unzip -o \""
            + final_filename
            + "\" -d \""
            + folder_name
            + "\"";

        std::cout
            << "Extracting frames...\n";

        int result =
            std::system(
                (
                    "mkdir -p \""
                    + folder_name
                    + "\" && "
                    + unzip_cmd
                ).c_str()
            );

        if (result != 0)
        {
            std::cerr
                << "Failed to extract ZIP\n";

            return false;
        }

        std::string mp4_filename =
            final_filename.substr(
                0,
                final_filename.length() - 4
            )
            + ".mp4";

        if (!frame_delays.empty())
        {
            std::string concat_path =
                folder_name
                + "/concat.txt";

            std::ofstream concat_file(
                concat_path
            );

            if (!concat_file.is_open())
            {
                std::cerr
                    << "Failed to create concat file\n";

                return false;
            }

            concat_file
                << "ffconcat version 1.0\n";

            for (
                size_t i = 0;
                i < frame_delays.size();
                ++i
            )
            {
                char frame_name[32];

                snprintf(
                    frame_name,
                    sizeof(frame_name),
                    "%06zu.jpg",
                    i
                );

                double duration =
                    static_cast<double>(
                        frame_delays[i]
                    )
                    / 1000.0;

                concat_file
                    << "file '"
                    << folder_name
                    << "/"
                    << frame_name
                    << "'\n";

                concat_file
                    << "duration "
                    << std::fixed
                    << std::setprecision(3)
                    << duration
                    << "\n";
            }

            concat_file.close();

            std::string ffmpeg_cmd =
                "ffmpeg -y "
                "-f concat "
                "-safe 0 "
                "-i \""
                + concat_path
                + "\" "
                "-c:v libx264 "
                "-pix_fmt yuv420p "
                "-vf \"fps=60,setpts=PTS-STARTPTS\" "
                "\""
                + mp4_filename
                + "\"";

            std::cout
                << "Converting to MP4 "
                << "("
                << frame_delays.size()
                << " frames)...\n";

            int ff_result =
                std::system(
                    ffmpeg_cmd.c_str()
                );

            if (ff_result != 0)
            {
                std::cerr
                    << "FFmpeg conversion failed\n";
            }
        }
        else
        {
            std::string ffmpeg_cmd =
                "ffmpeg -y "
                "-framerate 30 "
                "-i \""
                + folder_name
                + "/%06d.jpg\" "
                "-c:v libx264 "
                "-pix_fmt yuv420p "
                "\""
                + mp4_filename
                + "\"";

            std::cout
                << "Converting to MP4...\n";

            int ff_result =
                std::system(
                    ffmpeg_cmd.c_str()
                );

            if (ff_result != 0)
            {
                std::cerr
                    << "FFmpeg conversion failed\n";
            }
        }
    }


    return true;
}


std::string Downloader::extract_filename(
    const std::string& url
)
{
    size_t last_slash =
        url.find_last_of('/');

    if (last_slash == std::string::npos)
    {
        return "downloaded_file";
    }

    std::string filename =
        url.substr(last_slash + 1);

    size_t query_pos =
        filename.find('?');

    if (query_pos != std::string::npos)
    {
        filename =
            filename.substr(
                0,
                query_pos
            );
    }

    if (filename.empty())
    {
        return "downloaded_file";
    }

    return filename;
}


std::string Downloader::sanitize_filename(
    const std::string& filename
)
{
    std::string safe = filename;

    const std::string invalid_chars =
        "\\/:*?\"<>|$`;&|(){}!'#\n\r\t";

    for (char& c : safe)
    {
        if (
            static_cast<unsigned char>(c) < 0x20
            ||
            invalid_chars.find(c)
            != std::string::npos
        )
        {
            c = '_';
        }
    }

    if (
        safe.empty()
        ||
        safe == "."
        ||
        safe == ".."
    )
    {
        return "downloaded_file";
    }

    return safe;
}


bool Downloader::is_path_safe_for_shell(
    const std::string& path
)
{
    for (char c : path)
    {
        if (
            static_cast<unsigned char>(c) < 0x20
            ||
            c == ';'
            ||
            c == '`'
            ||
            c == '$'
            ||
            c == '|'
            ||
            c == '&'
        )
        {
            return false;
        }
    }

    return true;
}

bool Downloader::is_media_content_type(
    const std::string& content_type
)
{
    return
        content_type.starts_with("image/")
        ||
        content_type.starts_with("video/")
        ||
        content_type.starts_with("audio/")
        ||
        content_type == "application/octet-stream"
        ||
        content_type == "application/zip";
}
