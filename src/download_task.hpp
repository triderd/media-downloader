#pragma once

#include <string>
#include <vector>

struct DownloadTask
{

    int playlist_index = 0;
    
    int playlist_total = 0;
    
    std::string title;

    std::string playlist_title;

    std::string url;

    std::string filename;

    std::string directory;

    std::vector<std::string> headers;

    std::string cookies;

    bool use_ytdlp = false;

    std::string format_id;

    std::vector<int> frame_delays;
};
