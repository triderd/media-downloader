
#include <nlohmann/json.hpp>

#include "pixiv_extractor.hpp"

#include <curl/curl.h>

#include <iostream>
#include <regex>
#include <set>
#include "../../http/http_client.hpp"
#include "../../auth/cookie_manager.hpp"

using json =
    nlohmann::json;



bool PixivExtractor::matches(
    const std::string& url
)
{
    return
        url.find("pixiv.net")
        != std::string::npos;
}


std::string
PixivExtractor::extract_artwork_id(
    const std::string& url
)
{
    std::regex pattern(
        R"(/artworks/(\d+))"
    );

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

    return "";
}


std::string
PixivExtractor::http_get(
    const std::string& url
)
{
    HttpClient client;

    HttpRequest request;

    request.url = url;

    request.cookies =
        CookieManager::load(
            "pixiv"
        );

    request.referer =
        "https://www.pixiv.net/";

    request.headers =
    {
        "Referer: https://www.pixiv.net/"
    };

    HttpResponse response =
        client.get(request);

    if (response.status_code >= 400)
    {
        std::cerr
            << "Pixiv API error: HTTP "
            << response.status_code
            << " for "
            << url
            << "\n";

        return "";
    }

    return response.body;
}




std::vector<DownloadTask>
PixivExtractor::extract(
    const std::string& url
)
{
    std::string artwork_id =
        extract_artwork_id(url);


    if (artwork_id.empty())
    {
        std::cerr
            << "Could not extract artwork ID from URL\n";

        return {};
    }


    std::vector<DownloadTask> tasks;


    std::string info_url =
        "https://www.pixiv.net/ajax/illust/"
        + artwork_id;
    
    std::string info_json =
        http_get(info_url);

    if (info_json.empty())
    {
        std::cerr
            << "Pixiv illust info: empty response\n";

        return {};
    }

    json info_parsed;

    try
    {
        info_parsed =
            json::parse(
                info_json
            );
    }

    catch (const std::exception& e)
    {
        std::cerr
            << "Pixiv illust info parse error: "
            << e.what()
            << "\n";

        return {};
    }


    bool is_ugoira = false;
    
    if (
        info_parsed.contains("body")
    )
    {
        const auto& body =
            info_parsed["body"];
    
        if (
            body.contains("illustType")
        )
        {
            int illust_type =
                body["illustType"];

            if (illust_type == 2)
            {
                is_ugoira = true;
            }
        }
    }


    if (is_ugoira)
    {
        std::string ugoira_url =
            "https://www.pixiv.net/ajax/illust/"
            + artwork_id +
            "/ugoira_meta";

        std::string ugoira_json =
            http_get(ugoira_url);

        if (ugoira_json.empty())
        {
            std::cerr
                << "Failed to load ugoira metadata\n";

            return {};
        }

    json ugoira_parsed;

    try
    {
        ugoira_parsed =
            json::parse(
                ugoira_json
            );
    }

    catch (const std::exception& e)
    {
        std::cerr
            << "Ugoira metadata parse error: "
            << e.what()
            << "\n";

        return {};
    }

    if (
        !ugoira_parsed.contains(
            "body"
        )
    )
    {
        std::cerr
            << "Ugoira metadata: missing 'body'\n";

        return {};
    }

    const auto& body =
        ugoira_parsed["body"];

    if (
        !body.contains(
            "originalSrc"
        )
    )
    {
        std::cerr
            << "Ugoira metadata: missing 'originalSrc'\n";

        return {};
    }

    std::string zip_url =
        body["originalSrc"];

    DownloadTask task;

    task.url =
        zip_url;

    task.headers =
    {
        "Referer: https://www.pixiv.net/"
    };

    task.cookies =
        "PHPSESSID=113246217_AYi4n9iGcpKKTlMcy9nCp9v25pQS7HNr";

    task.filename =
        artwork_id
        +
        "_ugoira.zip";

    if (
        body.contains("frames")
    )
    {
        for (const auto& frame : body["frames"])
        {
            if (frame.contains("delay"))
            {
                task.frame_delays.push_back(
                    frame["delay"]
                );
            }
        }
    }

    tasks.push_back(task);

    return tasks;


    }




    std::string api_url =
        "https://www.pixiv.net/ajax/illust/"
        + artwork_id +
        "/pages";

    std::string json_data =
        http_get(api_url);

    if (json_data.empty())
    {
        std::cerr
            << "Pixiv pages: empty response\n";

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
            << "Pixiv pages parse error: "
            << e.what()
            << "\n";

        return {};
    }
    
    
    if (!parsed.contains("body"))
    {
        return {};
    }
    
    for (const auto& page : parsed["body"])
    {
        if (
            !page.contains("urls")
            ||
            !page["urls"].contains("original")
        )
        {
            continue;
        }
    
        std::string original_url =
            page["urls"]["original"];
    
        DownloadTask task;
    
        task.url = original_url;
    
        task.headers =
        {
            "Referer: https://www.pixiv.net/"
        };
    
        task.cookies =
            "PHPSESSID=113246217_AYi4n9iGcpKKTlMcy9nCp9v25pQS7HNr";
    
        size_t last_slash =
            original_url.find_last_of('/');
    
        if (last_slash != std::string::npos)
        {
            task.filename =
                original_url.substr(
                    last_slash + 1
                );
        }
    
        tasks.push_back(task);
    }

    return tasks;
}
