#pragma once

#include "../download_task.hpp"
#include "base_extractor.hpp"

#include <regex>

class GenericExtractor
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
    std::string download_page(
        const std::string& url
    );

    std::vector<std::string>
    find_media(
        const std::string& html
    );

    std::string resolve_url(
        const std::string& page_url,
        const std::string& media_url
    );
};
