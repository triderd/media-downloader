#include "pixiv_extractor.hpp"

#include <curl/curl.h>

#include <iostream>
#include <regex>
#include <set>


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

    std::string json =
        http_get(api_url);

    if (json.empty())
    {
        return {};
    }

    std::regex pattern(
        R"DELIM("original":"(https:\\/\\/i\.pximg\.net[^"]+)")DELIM"
    );

    std::sregex_iterator begin(
        json.begin(),
        json.end(),
        pattern
    );

    std::sregex_iterator end;

    std::vector<DownloadTask> tasks;

    for (
        auto it = begin;
        it != end;
        ++it
    )
    {
        std::string escaped =
            (*it)[1];

        std::string fixed;

        for (
            size_t i = 0;
            i < escaped.size();
            ++i
        )
        {
            if (
                i + 1 < escaped.size() &&
                escaped[i] == '\\' &&
                escaped[i + 1] == '/'
            )
            {
                fixed += '/';
                ++i;
            }
            else
            {
                fixed += escaped[i];
            }
        }

        DownloadTask task;

        task.url = fixed;

        task.headers =
        {
            "Referer: https://www.pixiv.net/"
        };

        task.cookies =
            "PHPSESSID=113246217_AYi4n9iGcpKKTlMcy9nCp9v25pQS7HNr";


        size_t last_slash =
            fixed.find_last_of('/');

        if (last_slash != std::string::npos)
        {
            task.filename =
                fixed.substr(
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
