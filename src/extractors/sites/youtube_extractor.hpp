#pragma once

#include "../base_extractor.hpp"

class YouTubeExtractor
    : public BaseExtractor
{
public:
    bool matches(
        const std::string& url
    ) override;

    std::vector<DownloadTask>
    extract(
        const std::string& url
    ) override;

private:

    std::string url_decode(
        const std::string& value
    );
    
    std::string extract_url_from_cipher(
        const std::string& cipher
    );


    std::string extract_player_json(
        const std::string& html
    );

    std::string http_get(
        const std::string& url
    );

    std::string http_post(
        const std::string& url,
        const std::string& body
    );


    std::string extract_video_id(
        const std::string& url
    );
};
