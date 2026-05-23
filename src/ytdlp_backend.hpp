#pragma once

#include <string>
#include <vector>

struct YtDlpFormat
{
    std::string id;

    std::string description;
};

class YtDlpBackend
{
public:
    static std::vector<YtDlpFormat>
    get_formats(
        const std::string& url
    );

    static std::string
    get_playlist_title(
        const std::string& url
    );

    static bool
    download(
        const std::string& url,
        const std::string& format_id,
        const std::string& output_template = "",
        const std::string& playlist_dir = ""
    );
};
