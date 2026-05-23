#pragma once

#include "../base_extractor.hpp"

#include <regex>
#include <string>
#include <vector>
#include <map>

struct PatternRule
{
    std::string name;

    std::regex url_match;

    std::vector<std::regex> media_patterns;

    std::map<std::string, std::string> headers;

    std::string cookies_from;
};

class PatternExtractor
    : public BaseExtractor
{
public:
    PatternExtractor();

    bool matches(
        const std::string& url
    ) override;

    std::vector<DownloadTask>
    extract(
        const std::string& url
    ) override;

private:
    std::vector<PatternRule> rules;

    void load_rules(
        const std::string& path
    );

    std::string fetch_page(
        const std::string& url,
        const PatternRule& rule
    );

    std::string resolve_url(
        const std::string& page_url,
        const std::string& media_url
    );
};
