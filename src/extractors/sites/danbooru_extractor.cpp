#include "danbooru_extractor.hpp"
#include "../../http/http_client.hpp"
#include "../../auth/cookie_manager.hpp"

#include <iostream>
#include <nlohmann/json.hpp>

using json =
    nlohmann::json;

bool DanbooruExtractor::matches(
    const std::string& url
)
{
    return
        url.find("danbooru.donmai.us")
        != std::string::npos
        ||
        url.find("gelbooru.com")
        != std::string::npos
        ||
        url.find("safebooru.org")
        != std::string::npos;
}

DanbooruExtractor::Site
DanbooruExtractor::detect_site(
    const std::string& url
)
{
    if (
        url.find("danbooru.donmai.us")
        != std::string::npos
    )
    {
        return Site::Danbooru;
    }

    if (
        url.find("gelbooru.com")
        != std::string::npos
    )
    {
        return Site::Gelbooru;
    }

    return Site::Unknown;
}

std::string
DanbooruExtractor::extract_post_id(
    const std::string& url
)
{
    std::vector<std::regex> patterns =
    {
        std::regex(
            R"DELIM(/posts/(\d+))DELIM"
        ),
        std::regex(
            R"DELIM([?&]id=(\d+))DELIM"
        ),
        std::regex(
            R"DELIM(&id=(\d+))DELIM"
        )
    };

    for (const auto& pattern : patterns)
    {
        std::smatch match;

        if (
            std::regex_search(
                url,
                match,
                pattern
            )
        )
        {
            return match[1];
        }
    }

    return "";
}

std::string
DanbooruExtractor::fetch_json(
    const std::string& url,
    const std::string& referer,
    const std::string& site_name
)
{
    HttpClient client;

    HttpRequest request;

    request.url = url;

    request.user_agent =
        "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/136.0.0.0 Safari/537.36";

    request.referer = referer;

    request.headers =
    {
        "Accept: application/json",
        "Accept-Language: en-US,en;q=0.9"
    };

    request.cookies =
        CookieManager::load(
            site_name
        );

    HttpResponse response =
        client.get(request);

    if (response.status_code >= 400)
    {
        std::cerr
            << "Danbooru: HTTP "
            << response.status_code
            << " for "
            << url
            << "\n";

        return "";
    }

    return response.body;
}

std::string
DanbooruExtractor::fetch_page(
    const std::string& url,
    const std::string& referer,
    const std::string& site_name
)
{
    HttpClient client;

    HttpRequest request;

    request.url = url;

    request.user_agent =
        "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/136.0.0.0 Safari/537.36";

    request.referer = referer;

    request.headers =
    {
        "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8",
        "Accept-Language: en-US,en;q=0.9"
    };

    HttpResponse response =
        client.get(request);

    if (response.status_code >= 400)
    {
        std::cerr
            << "Gelbooru page: HTTP "
            << response.status_code
            << "\n";

        return "";
    }

    return response.body;
}

std::vector<DownloadTask>
DanbooruExtractor::extract(
    const std::string& url
)
{
    Site site =
        detect_site(url);

    std::string post_id =
        extract_post_id(url);

    if (post_id.empty())
    {
        if (
            site == Site::Danbooru
            &&
            (
                url.find("tags=")
                != std::string::npos
            )
        )
        {
            return extract_search(url);
        }

        std::cerr
            << "Danbooru: could not extract post ID\n";

        return {};
    }

    std::string api_url;
    std::string referer;
    std::string site_name;

    if (site == Site::Gelbooru)
    {
        std::string page_url =
            "https://gelbooru.com/index.php"
            "?page=post&s=view&id="
            + post_id;

        std::string page_html =
            fetch_page(
                page_url,
                "https://gelbooru.com/",
                "gelbooru"
            );

        if (!page_html.empty())
        {
            std::regex img_regex(
                R"DELIM(<img[^>]*id="image"[^>]*src="([^"]+)")DELIM"
            );

            std::smatch img_match;

            if (
                std::regex_search(
                    page_html,
                    img_match,
                    img_regex
                )
            )
            {
                DownloadTask task;

                task.url =
                    img_match[1];

                task.headers =
                {
                    "Referer: https://gelbooru.com/"
                };

                size_t last_slash =
                    task.url.find_last_of('/');

                if (
                    last_slash
                    != std::string::npos
                )
                {
                    task.filename =
                        task.url.substr(
                            last_slash + 1
                        );
                }
                else
                {
                    task.filename =
                        "gelbooru_"
                        + post_id;
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

                return { task };
            }
        }

        return {};
    }

    if (site == Site::Danbooru)
    {
        api_url =
            "https://danbooru.donmai.us/posts/"
            + post_id
            + ".json";

        referer =
            "https://danbooru.donmai.us/";

        site_name = "danbooru";
    }
    else
    {
        return {};
    }

    std::string json_data =
        fetch_json(
            api_url,
            referer,
            site_name
        );

    if (json_data.empty())
    {
        return {};
    }

    json parsed;

    try
    {
        parsed =
            json::parse(
                json_data
            );
    }
    catch (const std::exception& e)
    {
        std::cerr
            << "Danbooru: JSON parse error: "
            << e.what()
            << "\n";

        return {};
    }

    std::string file_url;

    bool has_sample = false;

    if (site == Site::Danbooru)
    {
        if (
            parsed.contains("has_large")
            &&
            parsed["has_large"].is_boolean()
        )
        {
            has_sample =
                parsed["has_large"];
        }

        if (has_sample && parsed.contains("large_file_url"))
        {
            file_url =
                parsed["large_file_url"];
        }
        else if (has_sample && parsed.contains("md5"))
        {
            std::string md5 =
                parsed["md5"];

            file_url =
                "https://cdn.donmai.us/sample/"
                + md5.substr(0, 2)
                + "/"
                + md5.substr(2, 2)
                + "/sample-"
                + md5
                + ".jpg";
        }
        else if (parsed.contains("file_url"))
        {
            file_url =
                parsed["file_url"];
        }
    }
    else if (site == Site::Gelbooru)
    {
        if (
            parsed.contains("post")
            &&
            parsed["post"].is_array()
            &&
            !parsed["post"].empty()
        )
        {
            const auto& post =
                parsed["post"][0];

            if (post.contains("file_url"))
            {
                file_url =
                    post["file_url"];
            }
        }
    }

    if (file_url.empty())
    {
        std::cerr
            << "Danbooru: no file URL found\n";

        return {};
    }

    if (
        site == Site::Danbooru
        &&
        file_url.find("/original/")
        != std::string::npos
        &&
        parsed.contains("md5")
        &&
        parsed.contains("file_ext")
    )
    {
        std::string md5 =
            parsed["md5"];

        std::string ext =
            parsed["file_ext"];

        std::vector<std::string> tag_parts;

        std::string tag_string =
            parsed.value(
                "tag_string_character",
                ""
            );

        if (!tag_string.empty())
        {
            std::string with_underscores;

            for (char c : tag_string)
            {
                with_underscores +=
                    (c == ' ')
                    ? '_'
                    : c;
            }

            tag_parts.push_back(
                with_underscores
            );
        }

        std::string copyright_str =
            parsed.value(
                "tag_string_copyright",
                ""
            );

        if (!copyright_str.empty())
        {
            std::string with_underscores;

            for (char c : copyright_str)
            {
                with_underscores +=
                    (c == ' ')
                    ? '_'
                    : c;
            }

            tag_parts.push_back(
                with_underscores
            );
        }

        std::string artist_str =
            parsed.value(
                "tag_string_artist",
                ""
            );

        if (!artist_str.empty())
        {
            std::string with_underscores;

            for (char c : artist_str)
            {
                with_underscores +=
                    (c == ' ')
                    ? '_'
                    : c;
            }

            tag_parts.push_back(
                with_underscores
            );
        }

        if (!tag_parts.empty())
        {
            std::string joined;

            for (
                size_t i = 0;
                i < tag_parts.size();
                ++i
            )
            {
                if (i > 0)
                {
                    joined += "_";
                }

                joined += tag_parts[i];
            }

            std::string descriptive_url =
                "https://cdn.donmai.us/original/"
                + md5.substr(0, 2)
                + "/"
                + md5.substr(2, 2)
                + "/__"
                + joined
                + "__"
                + md5
                + "."
                + ext;

            file_url = descriptive_url;
        }
    }

    DownloadTask task;

    task.url = file_url;

    task.headers =
    {
        "Referer: " + referer
    };

    task.cookies =
        CookieManager::load(
            site_name
        );

    size_t last_slash =
        file_url.find_last_of('/');

    if (last_slash != std::string::npos)
    {
        task.filename =
            file_url.substr(
                last_slash + 1
            );
    }
    else
    {
        task.filename =
            "danbooru_"
            + post_id;
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

    return { task };
}

std::vector<DownloadTask>
DanbooruExtractor::extract_search(
    const std::string& url
)
{
    size_t tags_pos =
        url.find("tags=");

    if (tags_pos == std::string::npos)
    {
        return {};
    }

    std::string tags =
        url.substr(
            tags_pos + 5
        );

    size_t amp_pos =
        tags.find('&');

    if (amp_pos != std::string::npos)
    {
        tags =
            tags.substr(
                0,
                amp_pos
            );
    }

    std::string api_url =
        "https://danbooru.donmai.us/posts.json"
        "?tags="
        + tags
        + "&limit=200";

    std::string json_data =
        fetch_json(
            api_url,
            "https://danbooru.donmai.us/",
            "danbooru"
        );

    if (json_data.empty())
    {
        return {};
    }

    json parsed;

    try
    {
        parsed =
            json::parse(
                json_data
            );
    }
    catch (const std::exception& e)
    {
        std::cerr
            << "Danbooru search: JSON parse error: "
            << e.what()
            << "\n";

        return {};
    }

    if (
        !parsed.is_array()
        ||
        parsed.empty()
    )
    {
        std::cerr
            << "Danbooru search: no results\n";

        return {};
    }

    std::vector<DownloadTask> tasks;

    for (const auto& post : parsed)
    {
        std::string file_url =
            get_file_url_from_post(
                post
            );

        if (file_url.empty())
        {
            continue;
        }

        DownloadTask task;

        task.url = file_url;

        task.headers =
        {
            "Referer: https://danbooru.donmai.us/"
        };

        task.cookies =
            CookieManager::load(
                "danbooru"
            );

        size_t last_slash =
            file_url.find_last_of('/');

        if (last_slash != std::string::npos)
        {
            task.filename =
                file_url.substr(
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

        if (task.filename.empty())
        {
            if (post.contains("id"))
            {
                task.filename =
                    "danbooru_"
                    + std::to_string(
                        post["id"].get<int>()
                    );
            }
        }

        tasks.push_back(task);
    }

    return tasks;
}

std::string
DanbooruExtractor::get_file_url_from_post(
    const json& post
)
{
    bool has_sample = false;

    if (
        post.contains("has_large")
        &&
        post["has_large"].is_boolean()
    )
    {
        has_sample =
            post["has_large"];
    }

    if (has_sample && post.contains("large_file_url"))
    {
        return post["large_file_url"];
    }

    if (
        post.contains("file_url")
    )
    {
        std::string file_url =
            post["file_url"];

        if (
            file_url.find("/original/")
            != std::string::npos
            &&
            post.contains("md5")
            &&
            post.contains("file_ext")
        )
        {
            std::string md5 =
                post["md5"];

            std::string ext =
                post["file_ext"];

            std::vector<std::string> tag_parts;

            for (
                const char* field :
                {
                    "tag_string_character",
                    "tag_string_copyright",
                    "tag_string_artist"
                }
            )
            {
                std::string tag_str =
                    post.value(
                        field,
                        ""
                    );

                if (!tag_str.empty())
                {
                    std::string with_underscores;

                    for (char c : tag_str)
                    {
                        with_underscores +=
                            (c == ' ')
                            ? '_'
                            : c;
                    }

                    tag_parts.push_back(
                        with_underscores
                    );
                }
            }

            if (!tag_parts.empty())
            {
                std::string joined;

                for (
                    size_t i = 0;
                    i < tag_parts.size();
                    ++i
                )
                {
                    if (i > 0)
                    {
                        joined += "_";
                    }

                    joined += tag_parts[i];
                }

                return
                    "https://cdn.donmai.us/original/"
                    + md5.substr(0, 2)
                    + "/"
                    + md5.substr(2, 2)
                    + "/__"
                    + joined
                    + "__"
                    + md5
                    + "."
                    + ext;
            }
        }

        return file_url;
    }

    return "";
}
