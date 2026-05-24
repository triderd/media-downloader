#pragma once

#include "../base_extractor.hpp"

#include <regex>
#include <string>
#include <vector>
#include <nlohmann/json_fwd.hpp>

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

    std::string fetch_page(
        const std::string& url,
        const std::string& referer,
        const std::string& site_name
    );

    std::string extract_post_id(
        const std::string& url
    );

    std::vector<DownloadTask>
    extract_search(
        const std::string& url
    );

    std::string
    get_file_url_from_post(
        const nlohmann::json& post
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
