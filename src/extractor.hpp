#pragma once

#include <string>
#include <vector>


class Extractor
{
public:
    std::vector<std::string> extract_media_urls(
        const std::string& page_url
    );

private:
    std::string download_page(
        const std::string& url
    );

    std::vector<std::string> find_og_media(
        const std::string& html
    );

    std::vector<std::string> find_html_media(
        const std::string& html
    );

    std::string resolve_url(
        const std::string& page_url,
        const std::string& media_url
    );

};
