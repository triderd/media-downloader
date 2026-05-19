#pragma once

#include "../../download_task.hpp"
#include "../base_extractor.hpp"

class PixivExtractor
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
    std::string http_get(
        const std::string& url
    );
    
    std::string extract_artwork_id(
        const std::string& url
    );

};
