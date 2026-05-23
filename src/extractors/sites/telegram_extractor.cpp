#include "telegram_extractor.hpp"
#include "../../http/http_client.hpp"

#include <iostream>
#include <set>
#include <regex>
#include <sstream>

bool TelegramExtractor::matches(
    const std::string& url
)
{
    return
        url.find("t.me/")
        != std::string::npos
        ||
        url.find("telegram.me/")
        != std::string::npos;
}

std::string
TelegramExtractor::fetch_embed(
    const std::string& channel,
    const std::string& post_id
)
{
    HttpClient client;

    HttpRequest request;

    request.url =
        "https://t.me/s/"
        + channel
        + "/"
        + post_id;

    request.user_agent =
        "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/136.0.0.0 Safari/537.36";

    HttpResponse response =
        client.get(request);

    if (response.status_code >= 400)
    {
        std::cerr
            << "Telegram: HTTP "
            << response.status_code
            << "\n";

        return "";
    }

    return response.body;
}

std::vector<std::string>
TelegramExtractor::extract_media_urls(
    const std::string& html,
    const std::string& post_id
)
{
    std::vector<std::string> urls;

    std::regex photo_pattern(
        R"DELIM(<a[^>]*class="tgme_widget_message_photo_wrap[^"]*"[^>]*href="[^"]*/(\d+)"[^>]*style="[^"]*background-image:url\('([^']+)'\))DELIM"
    );

    std::sregex_iterator begin(
        html.begin(),
        html.end(),
        photo_pattern
    );

    std::sregex_iterator end;

    for (
        auto it = begin;
        it != end;
        ++it
    )
    {
        std::string match_post_id =
            (*it)[1];

        std::string media_url =
            (*it)[2];

        if (match_post_id != post_id)
        {
            continue;
        }

        media_url = clean_telegram_url(
            media_url
        );

        if (
            !media_url.empty()
            &&
            (
                media_url.starts_with("https://")
                ||
                media_url.starts_with("http://")
            )
        )
        {
            urls.push_back(media_url);
        }
    }

    if (!urls.empty())
    {
        return urls;
    }

    std::vector<std::regex> fallback_patterns =
    {
        std::regex(
            R"DELIM(<video[^>]*src="([^"]+)")DELIM"
        ),
        std::regex(
            R"DELIM(<meta[^>]*property="og:image"[^>]*content="([^"]+)")DELIM"
        ),
        std::regex(
            R"DELIM(<meta[^>]*property="og:video"[^>]*content="([^"]+)")DELIM"
        )
    };

    for (const auto& pattern : fallback_patterns)
    {
        std::sregex_iterator fb_begin(
            html.begin(),
            html.end(),
            pattern
        );

        std::sregex_iterator fb_end;

        for (
            auto it = fb_begin;
            it != fb_end;
            ++it
        )
        {
            std::string media_url =
                (*it)[1];

            media_url = clean_telegram_url(
                media_url
            );

            if (
                !media_url.empty()
                &&
                (
                    media_url.starts_with("https://")
                    ||
                    media_url.starts_with("http://")
                )
            )
            {
                urls.push_back(media_url);
            }
        }

        if (!urls.empty())
        {
            break;
        }
    }

    return urls;
}

std::string
TelegramExtractor::clean_telegram_url(
    const std::string& url
)
{
    size_t query_pos =
        url.find('?');

    if (query_pos == std::string::npos)
    {
        return url;
    }

    std::string base =
        url.substr(
            0,
            query_pos
        );

    std::string query =
        url.substr(
            query_pos + 1
        );

    std::vector<
        std::pair<
            std::string,
            std::string
        >
    > params;

    std::stringstream ss(query);
    std::string param;

    while (
        std::getline(
            ss,
            param,
            '&'
        )
    )
    {
        size_t eq =
            param.find('=');

        if (eq != std::string::npos)
        {
            std::string key =
                param.substr(0, eq);

            std::string value =
                param.substr(eq + 1);

            if (key != "size")
            {
                params.push_back({
                    key,
                    value
                });
            }
        }
    }

    if (params.empty())
    {
        return base;
    }

    std::string result =
        base + "?";

    for (
        size_t i = 0;
        i < params.size();
        ++i
    )
    {
        if (i > 0)
        {
            result += "&";
        }

        result +=
            params[i].first
            + "="
            + params[i].second;
    }

    return result;
}

std::vector<DownloadTask>
TelegramExtractor::extract(
    const std::string& url
)
{
    std::string clean_url = url;

    size_t s_pos =
        clean_url.find("t.me/s/");

    if (s_pos != std::string::npos)
    {
        clean_url =
            clean_url.substr(
                s_pos + 7
            );
    }

    std::regex url_pattern(
        R"DELIM(t(?:elegram)?\.me/([^/?]+)/(\d+))DELIM"
    );

    std::smatch match;

    if (
        !std::regex_search(
            clean_url,
            match,
            url_pattern
        )
    )
    {
        std::cerr
            << "Telegram: unsupported URL format\n";

        return {};
    }

    std::string channel =
        match[1];

    std::string post_id =
        match[2];

    std::string html =
        fetch_embed(
            channel,
            post_id
        );

    if (html.empty())
    {
        std::cerr
            << "Telegram: empty response\n";

        return {};
    }

    std::vector<std::string> media_urls =
        extract_media_urls(
            html,
            post_id
        );

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

        task.headers =
        {
            "Referer: https://t.me/"
        };

        size_t last_slash =
            media_url.find_last_of('/');

        if (
            last_slash != std::string::npos
            &&
            last_slash + 1 < media_url.size()
        )
        {
            task.filename =
                media_url.substr(
                    last_slash + 1
                );
        }

        size_t qpos =
            task.filename.find('?');

        if (qpos != std::string::npos)
        {
            task.filename =
                task.filename.substr(
                    0,
                    qpos
                );
        }

        if (
            task.filename.empty()
            ||
            task.filename.size() > 200
        )
        {
            task.filename =
                "tg_"
                + channel
                + "_"
                + post_id;

            if (
                !media_url.empty()
            )
            {
                size_t dot =
                    media_url.find_last_of('.');

                size_t slash_or_q =
                    media_url.find_first_of(
                        "/?",
                        dot != std::string::npos
                            ? dot
                            : 0
                    );

                if (
                    dot != std::string::npos
                    &&
                    (
                        slash_or_q == std::string::npos
                        ||
                        slash_or_q > dot
                    )
                )
                {
                    std::string ext =
                        media_url.substr(
                            dot,
                            slash_or_q != std::string::npos
                                ? slash_or_q - dot
                                : std::string::npos
                        );

                    if (ext.size() <= 10)
                    {
                        task.filename += ext;
                    }
                }
            }
        }

        tasks.push_back(task);
    }

    return tasks;
}
