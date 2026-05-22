#pragma once

#include <string>
#include <vector>

struct DownloadTask
{
    std::string url;

    std::string filename;

    std::vector<std::string> headers;

    std::string cookies;

    bool use_ytdlp = false;
};
