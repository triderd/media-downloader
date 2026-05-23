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
        std::cerr
            << "Danbooru: could not extract post ID\n";

        return {};
    }

    std::string api_url;
    std::string referer;
    std::string site_name;

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
    else if (site == Site::Gelbooru)
    {
        api_url =
            "https://gelbooru.com/index.php"
            "?page=dapi&s=post&q=index&json=1"
            "&id="
            + post_id;

        referer =
            "https://gelbooru.com/";

        site_name = "gelbooru";
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
