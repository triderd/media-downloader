#include "pattern_extractor.hpp"
#include "../../http/http_client.hpp"
#include "../../auth/cookie_manager.hpp"
#include "../../paths.hpp"

#include <fstream>
#include <iostream>
#include <set>
#include <nlohmann/json.hpp>

using json =
    nlohmann::json;

PatternExtractor::PatternExtractor()
{
    load_rules(
        "patterns.json"
    );
}

void PatternExtractor::load_rules(
    const std::string& path
)
{
    std::ifstream file(
        Paths::resolve(path)
    );

    if (!file.is_open())
    {
        return;
    }

    json config;

    try
    {
        config =
            json::parse(file);
    }
    catch (const std::exception& e)
    {
        std::cerr
            << "PatternExtractor: JSON parse error: "
            << e.what()
            << "\n";

        return;
    }

    if (!config.is_array())
    {
        std::cerr
            << "PatternExtractor: expected JSON array\n";

        return;
    }

    for (const auto& entry : config)
    {
        PatternRule rule;

        if (!entry.contains("name"))
        {
            continue;
        }

        rule.name =
            entry["name"];

        if (entry.contains("url_match"))
        {
            try
            {
                rule.url_match =
                    std::regex(
                        entry["url_match"]
                    );
            }
            catch (const std::regex_error& e)
            {
                std::cerr
                    << "PatternExtractor: invalid regex for "
                    << rule.name
                    << ": "
                    << e.what()
                    << "\n";

                continue;
            }
        }
        else
        {
            continue;
        }

        if (entry.contains("media_patterns"))
        {
            for (const auto& pattern : entry["media_patterns"])
            {
                try
                {
                    rule.media_patterns.push_back(
                        std::regex(
                            pattern
                        )
                    );
                }
                catch (const std::regex_error& e)
                {
                    std::cerr
                        << "PatternExtractor: invalid media regex for "
                        << rule.name
                        << ": "
                        << e.what()
                        << "\n";
                }
            }
        }

        if (entry.contains("headers"))
        {
            for (const auto& [key, value] : entry["headers"].items())
            {
                rule.headers[key] =
                    value;
            }
        }

        if (entry.contains("cookies_from"))
        {
            rule.cookies_from =
                entry["cookies_from"];
        }

        rules.push_back(
            std::move(rule)
        );
    }
}

bool PatternExtractor::matches(
    const std::string& url
)
{
    for (const auto& rule : rules)
    {
        if (
            std::regex_search(
                url,
                rule.url_match
            )
        )
        {
            return true;
        }
    }

    return false;
}

std::string
PatternExtractor::fetch_page(
    const std::string& url,
    const PatternRule& rule
)
{
    HttpClient client;

    HttpRequest request;

    request.url = url;

    if (!rule.cookies_from.empty())
    {
        request.cookies =
            CookieManager::load(
                rule.cookies_from
            );
    }

    for (const auto& [key, value] : rule.headers)
    {
        request.headers.push_back(
            key
            + ": "
            + value
        );
    }

    if (rule.headers.contains("Referer"))
    {
        request.referer =
            rule.headers.at("Referer");
    }

    HttpResponse response =
        client.get(request);

    if (response.status_code >= 400)
    {
        std::cerr
            << "PatternExtractor: HTTP "
            << response.status_code
            << " for "
            << url
            << "\n";

        return "";
    }

    return response.body;
}

std::string
PatternExtractor::resolve_url(
    const std::string& page_url,
    const std::string& media_url
)
{
    if (
        media_url.starts_with("http://")
        ||
        media_url.starts_with("https://")
    )
    {
        return media_url;
    }

    if (
        media_url.starts_with("//")
    )
    {
        return
            "https:"
            + media_url;
    }

    size_t protocol_pos =
        page_url.find("://");

    if (protocol_pos == std::string::npos)
    {
        return media_url;
    }

    size_t domain_start =
        protocol_pos + 3;

    size_t path_pos =
        page_url.find(
            '/',
            domain_start
        );

    std::string base;

    if (path_pos == std::string::npos)
    {
        base = page_url;
    }
    else
    {
        base =
            page_url.substr(
                0,
                path_pos
            );
    }

    if (
        !media_url.empty()
        &&
        media_url.front() == '/'
    )
    {
        return base + media_url;
    }

    return
        base
        + "/"
        + media_url;
}

std::vector<DownloadTask>
PatternExtractor::extract(
    const std::string& url
)
{
    const PatternRule* matched_rule =
        nullptr;

    for (const auto& rule : rules)
    {
        if (
            std::regex_search(
                url,
                rule.url_match
            )
        )
        {
            matched_rule = &rule;

            break;
        }
    }

    if (!matched_rule)
    {
        return {};
    }

    std::string html =
        fetch_page(
            url,
            *matched_rule
        );

    if (html.empty())
    {
        return {};
    }

    std::vector<std::string> media_urls;

    for (const auto& pattern : matched_rule->media_patterns)
    {
        std::sregex_iterator begin(
            html.begin(),
            html.end(),
            pattern
        );

        std::sregex_iterator end;

        for (
            auto it = begin;
            it != end;
            ++it
        )
        {
            media_urls.push_back(
                (*it)[1]
            );
        }
    }

    for (auto& media_url : media_urls)
    {
        media_url =
            resolve_url(
                url,
                media_url
            );
    }

    std::set<std::string> unique(
        media_urls.begin(),
        media_urls.end()
    );

    media_urls.assign(
        unique.begin(),
        unique.end()
    );

    std::vector<DownloadTask> tasks;

    for (const auto& media_url : media_urls)
    {
        DownloadTask task;

        task.url = media_url;

        if (!matched_rule->cookies_from.empty())
        {
            task.cookies =
                CookieManager::load(
                    matched_rule->cookies_from
                );
        }

        for (const auto& [key, value] : matched_rule->headers)
        {
            task.headers.push_back(
                key
                + ": "
                + value
            );
        }

        size_t last_slash =
            media_url.find_last_of('/');

        if (last_slash != std::string::npos)
        {
            task.filename =
                media_url.substr(
                    last_slash + 1
                );
        }
        else
        {
            task.filename =
                "downloaded_file";
        }

        size_t query_pos =
            task.filename.find('?');

        if (query_pos != std::string::npos)
        {
            task.filename =
                task.filename.substr(
                    0,
                    query_pos
                );
        }

        tasks.push_back(task);
    }

    return tasks;
}
