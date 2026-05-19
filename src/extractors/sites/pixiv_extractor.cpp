
#include <nlohmann/json.hpp>

#include "pixiv_extractor.hpp"

#include <curl/curl.h>

#include <iostream>
#include <regex>
#include <set>


using json =
    nlohmann::json;

static size_t write_callback(
    void* ptr,
    size_t size,
    size_t nmemb,
    void* userdata
)
{
    std::string* data =
        static_cast<std::string*>(
            userdata
        );

    data->append(
        static_cast<char*>(ptr),
        size * nmemb
    );

    return size * nmemb;
}


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
    CURL* curl =
        curl_easy_init();

    if (!curl)
    {
        return "";
    }

    std::string html;

    curl_easy_setopt(
        curl,
        CURLOPT_URL,
        url.c_str()
    );

    curl_easy_setopt(
        curl,
        CURLOPT_WRITEFUNCTION,
        write_callback
    );

    curl_easy_setopt(
        curl,
        CURLOPT_WRITEDATA,
        &html
    );

    curl_easy_setopt(
        curl,
        CURLOPT_FOLLOWLOCATION,
        1L
    );


    curl_easy_setopt(
        curl,
        CURLOPT_TIMEOUT,
        30L
    );
    
    curl_easy_setopt(
        curl,
        CURLOPT_CONNECTTIMEOUT,
        10L
    );


    curl_easy_setopt(
        curl,
        CURLOPT_HTTP_VERSION,
        CURL_HTTP_VERSION_1_1
    );


    curl_easy_setopt(
        curl,
        CURLOPT_USERAGENT,
        "Mozilla/5.0"
    );


    curl_slist* headers = nullptr;

    headers = curl_slist_append(
        headers,
        "Referer: https://www.pixiv.net/"
    );
    
    headers = curl_slist_append(
        headers,
        "User-Agent: Mozilla/5.0"
    );
    
    curl_easy_setopt(
        curl,
        CURLOPT_HTTPHEADER,
        headers
    );


    std::cout
        << "Sending Pixiv API request...\n";

    CURLcode result =
        curl_easy_perform(curl);


    std::cout
        << "Pixiv API response received\n";

    
    curl_slist_free_all(
        headers
    );

    curl_easy_cleanup(curl);

    if (result != CURLE_OK)
    {
        return "";
    }

    return html;
}





std::vector<DownloadTask>
PixivExtractor::extract(
    const std::string& url
)
{
    std::cout
        << "Pixiv extraction...\n";

    std::string artwork_id =
        extract_artwork_id(url);

    if (artwork_id.empty())
    {
        return {};
    }

    std::string api_url =
        "https://www.pixiv.net/ajax/illust/"
        + artwork_id +
        "/pages";

    std::cout
        << "Pixiv API: "
        << api_url
        << "\n";

    std::string json_data =
        http_get(api_url);

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
            << "JSON parse error: "
            << e.what()
            << "\n";
    
        return {};
    }
    
    std::vector<DownloadTask> tasks;
    
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
            "PHPSESSID=PUT_YOUR_SESSION_HERE";
    
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



    std::cout
        << "Pixiv tasks: "
        << tasks.size()
        << "\n";

    return tasks;
}
