#include "ytdlp_backend.hpp"

#include <cstdio>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cstdlib>

std::vector<YtDlpFormat>
YtDlpBackend::get_formats(
    const std::string& url
)
{
    std::vector<YtDlpFormat> formats;

    std::string command =
        "yt-dlp -F \""
        + url
        + "\"";

    FILE* pipe =
        popen(command.c_str(), "r");

    if (!pipe)
    {
        return formats;
    }

    char buffer[4096];

    while (
        fgets(
            buffer,
            sizeof(buffer),
            pipe
        )
    )
    {
        std::string line =
            buffer;

        if (
            line.starts_with("ID")
            ||
            line.starts_with("[")
            ||
            line.size() < 5
        )
        {
            continue;
        }

        std::stringstream ss(
            line
        );

        std::string id;

        ss >> id;

        if (id.empty())
        {
            continue;
        }

        YtDlpFormat format;

        format.id =
            id;

        format.description =
            line;

        formats.push_back(
            format
        );
    }

    pclose(pipe);

    return formats;
}

std::string
YtDlpBackend::get_playlist_title(
    const std::string& url
)
{
    std::string command =
        "yt-dlp --print playlist_title \""
        + url
        + "\"";

    FILE* pipe =
        popen(command.c_str(), "r");

    if (!pipe)
    {
        return "playlist";
    }

    char buffer[256];
    std::string result;

    if (
        fgets(
            buffer,
            sizeof(buffer),
            pipe
        )
    )
    {
        result = buffer;
    }

    pclose(pipe);

    if (result.empty())
    {
        return "playlist";
    }

    result.erase(
        std::remove(
            result.begin(),
            result.end(),
            '\n'
        ),
        result.end()
    );

    return result;
}

bool
YtDlpBackend::download(
    const std::string& url,
    const std::string& format_id,
    const std::string& output_template,
    const std::string& playlist_dir
)
{
    std::string command;

    if (!playlist_dir.empty())
    {
        command =
            "yt-dlp "
            "--yes-playlist "
            "--js-runtimes node "
            "-f "
            + format_id
            + " "
            "-o \""
            + playlist_dir
            + "/%(playlist_index)02d - %(title)s.%(ext)s\" "
            "\""
            + url
            + "\"";
    }
    else
    {
        std::string output =
            output_template.empty()
            ? "%(title)s.%(ext)s"
            : output_template;

        command =
            "yt-dlp -f "
            + format_id
            + " -o \""
            + output
            + "\" \""
            + url
            + "\"";
    }

    int result =
        system(command.c_str());

    return
        result == 0;
}
