#pragma once

#include "../base_extractor.hpp"

#include <regex>
#include <string>
#include <vector>

class DanbooruExtractor
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
    std::string fetch_json(
        const std::string& url,
        const std::string& referer,
        const std::string& site_name
    );

    std::string extract_post_id(
        const std::string& url
    );

    enum class Site
    {
        Danbooru,
        Gelbooru,
        Unknown
    };

    Site detect_site(
        const std::string& url
    );
};
