#pragma once

#include "../base_extractor.hpp"

#include <regex>
#include <string>
#include <vector>

class TelegramExtractor
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
    std::string fetch_embed(
        const std::string& channel,
        const std::string& post_id
    );

    std::vector<std::string>
    extract_media_urls(
        const std::string& html,
        const std::string& post_id
    );

    std::string clean_telegram_url(
        const std::string& url
    );
};
