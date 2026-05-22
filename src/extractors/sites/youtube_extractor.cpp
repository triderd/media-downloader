#include "youtube_extractor.hpp"

#include <curl/curl.h>
#include <sstream>
#include <regex>
#include <iostream>
#include <nlohmann/json.hpp>


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

    size_t total =
        size * nmemb;

    data->append(
        static_cast<char*>(ptr),
        total
    );

    return total;
}



bool YouTubeExtractor::matches(
    const std::string& url
)
{
    return
        url.find("youtube.com")
        != std::string::npos
        ||
        url.find("youtu.be")
        != std::string::npos;
}

std::string
YouTubeExtractor::extract_video_id(
    const std::string& url
)
{
    std::regex pattern(
        R"([?&]v=([^&]+))"
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
YouTubeExtractor::http_get(
    const std::string& url
)
{
    CURL* curl =
        curl_easy_init();

    if (!curl)
    {
        return "";
    }

    std::string response;

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
        &response
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
        CURLOPT_USERAGENT,
        "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/136.0.0.0 Safari/537.36"
    );

    struct curl_slist* headers = nullptr;
    
    headers = curl_slist_append(
        headers,
        "Accept-Language: en-US,en;q=0.9"
    );
    
    headers = curl_slist_append(
        headers,
        "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8"
    );
    
    headers = curl_slist_append(
        headers,
        "Connection: keep-alive"
    );
    
    headers = curl_slist_append(
        headers,
        "Upgrade-Insecure-Requests: 1"
    );
    
    curl_easy_setopt(
        curl,
        CURLOPT_HTTPHEADER,
        headers
    );




    CURLcode result =
        curl_easy_perform(curl);

    curl_slist_free_all(headers);


    if (result != CURLE_OK)
    {
        curl_easy_cleanup(curl);

        return "";
    }

    curl_easy_cleanup(curl);

    return response;
}


std::string
YouTubeExtractor::http_post(
    const std::string& url,
    const std::string& body
)
{
    CURL* curl =
        curl_easy_init();

    if (!curl)
    {
        return "";
    }

    std::string response;

    struct curl_slist* headers = nullptr;

    headers = curl_slist_append(
        headers,
        "Content-Type: application/json"
    );

    headers = curl_slist_append(
        headers,
        "User-Agent: com.google.android.youtube/20.10.38"
    );

    curl_easy_setopt(
        curl,
        CURLOPT_URL,
        url.c_str()
    );

    curl_easy_setopt(
        curl,
        CURLOPT_POST,
        1L
    );

    curl_easy_setopt(
        curl,
        CURLOPT_POSTFIELDS,
        body.c_str()
    );

    curl_easy_setopt(
        curl,
        CURLOPT_HTTPHEADER,
        headers
    );

    curl_easy_setopt(
        curl,
        CURLOPT_WRITEFUNCTION,
        write_callback
    );

    curl_easy_setopt(
        curl,
        CURLOPT_WRITEDATA,
        &response
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

    CURLcode result =
        curl_easy_perform(curl);

    curl_slist_free_all(headers);

    if (result != CURLE_OK)
    {
        curl_easy_cleanup(curl);

        return "";
    }

    curl_easy_cleanup(curl);

    return response;
}





std::string
YouTubeExtractor::extract_player_json(
    const std::string& html
)
{
    std::regex pattern(
        R"(ytInitialPlayerResponse\s*=\s*(\{[\s\S]+?\});)"
    );



    std::smatch match;

    if (
        std::regex_search(
            html,
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
YouTubeExtractor::url_decode(
    const std::string& value
)
{
    std::string result;

    for (
        size_t i = 0;
        i < value.size();
        ++i
    )
    {
        if (
            value[i] == '%'
            &&
            i + 2 < value.size()
        )
        {
            std::string hex =
                value.substr(i + 1, 2);

            char decoded =
                static_cast<char>(
                    std::stoi(
                        hex,
                        nullptr,
                        16
                    )
                );

            result += decoded;

            i += 2;
        }
        else if (value[i] == '+')
        {
            result += ' ';
        }
        else
        {
            result += value[i];
        }
    }

    return result;
}



std::string
YouTubeExtractor::extract_url_from_cipher(
    const std::string& cipher
)
{
    std::stringstream ss(cipher);

    std::string part;

    while (
        std::getline(
            ss,
            part,
            '&'
        )
    )
    {
        size_t eq =
            part.find('=');

        if (eq == std::string::npos)
        {
            continue;
        }

        std::string key =
            part.substr(0, eq);

        std::string value =
            part.substr(eq + 1);

        if (key == "url")
        {
            return url_decode(value);
        }
    }

    return "";
}



std::vector<DownloadTask>
YouTubeExtractor::extract(
    const std::string& url
)
{
    std::cout
        << "YouTube extraction...\n";

    DownloadTask task;

    task.url = url;

    task.filename =
        "youtube_video";

    task.use_ytdlp = true;

    return { task };
}
