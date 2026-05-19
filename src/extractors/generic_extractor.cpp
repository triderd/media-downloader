#include "generic_extractor.hpp"

#include <curl/curl.h>

#include <iostream>
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


bool GenericExtractor::matches(
    const std::string& url
)
{
    return true;
}


std::string
GenericExtractor::download_page(
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
        CURLOPT_USERAGENT,
        "Mozilla/5.0"
    );

    curl_easy_setopt(
        curl,
        CURLOPT_TIMEOUT,
        30L
    );

    CURLcode result =
        curl_easy_perform(curl);

    curl_easy_cleanup(curl);

    if (result != CURLE_OK)
    {
        return "";
    }

    return html;
}


std::vector<std::string>
GenericExtractor::find_media(
    const std::string& html
)
{
    std::vector<std::string> media;

    std::vector<std::regex> patterns =
    {
        std::regex(
            R"DELIM(<img[^>]+src="([^"]+)")DELIM"
        ),

        std::regex(
            R"DELIM(<video[^>]+src="([^"]+)")DELIM"
        )
    };

    for (const auto& pattern : patterns)
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
            std::smatch match = *it;

            media.push_back(
                match[1]
            );
        }
    }

    return media;
}


std::string
GenericExtractor::resolve_url(
    const std::string& page_url,
    const std::string& media_url
)
{
    if (media_url.starts_with("http://") ||
        media_url.starts_with("https://"))
    {
        return media_url;
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

    if (!media_url.empty() &&
        media_url.front() == '/')
    {
        return base + media_url;
    }

    return base + "/" + media_url;
}


std::vector<DownloadTask>
GenericExtractor::extract(
    const std::string& url
)
{
    std::cout
        << "Generic extraction...\n";

    std::string html =
        download_page(url);

    if (html.empty())
    {
        return {};
    }

    auto media =
        find_media(html);

    for (auto& media_url : media)
    {
        media_url =
            resolve_url(
                url,
                media_url
            );
    }

    std::set<std::string> unique(
        media.begin(),
        media.end()
    );

    media.assign(
        unique.begin(),
        unique.end()
    );


    std::vector<DownloadTask> tasks;

    for (const auto& media_url : media)
    {
        DownloadTask task;
    
        task.url = media_url;


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
