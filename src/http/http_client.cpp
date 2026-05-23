#include "http_client.hpp"

#include <curl/curl.h>

#include <iostream>

size_t HttpClient::write_callback(
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

size_t HttpClient::header_callback(
    char* buffer,
    size_t size,
    size_t nitems,
    void* userdata
)
{
    size_t total =
        size * nitems;

    auto* headers =
        static_cast<
            std::map<
                std::string,
                std::string
            >*
        >(userdata);

    std::string line(
        buffer,
        total
    );

    size_t colon =
        line.find(':');

    if (colon != std::string::npos)
    {
        std::string key =
            line.substr(0, colon);

        std::string value =
            line.substr(colon + 1);

        headers->insert({
            key,
            value
        });
    }

    return total;
}

HttpResponse HttpClient::get(
    const HttpRequest& request
)
{
    HttpResponse response;

    CURL* curl =
        curl_easy_init();

    if (!curl)
    {
        return response;
    }

    curl_easy_setopt(
        curl,
        CURLOPT_URL,
        request.url.c_str()
    );

    curl_easy_setopt(
        curl,
        CURLOPT_WRITEFUNCTION,
        write_callback
    );

    curl_easy_setopt(
        curl,
        CURLOPT_WRITEDATA,
        &response.body
    );

    curl_easy_setopt(
        curl,
        CURLOPT_HEADERFUNCTION,
        header_callback
    );

    curl_easy_setopt(
        curl,
        CURLOPT_HEADERDATA,
        &response.headers
    );

    curl_easy_setopt(
        curl,
        CURLOPT_FOLLOWLOCATION,
        request.follow_redirects ? 1L : 0L
    );

    curl_easy_setopt(
        curl,
        CURLOPT_TIMEOUT,
        request.timeout
    );

    curl_easy_setopt(
        curl,
        CURLOPT_USERAGENT,
        request.user_agent.c_str()
    );

    curl_easy_setopt(
        curl,
        CURLOPT_ACCEPT_ENCODING,
        ""
    );

    curl_easy_setopt(
        curl,
        CURLOPT_SSL_VERIFYPEER,
        0L
    );

    curl_easy_setopt(
        curl,
        CURLOPT_SSL_VERIFYHOST,
        0L
    );

    if (!request.cookies.empty())
    {
        curl_easy_setopt(
            curl,
            CURLOPT_COOKIE,
            request.cookies.c_str()
        );
    }

    if (!request.referer.empty())
    {
        curl_easy_setopt(
            curl,
            CURLOPT_REFERER,
            request.referer.c_str()
        );
    }

    curl_slist* header_list =
        nullptr;

    for (const auto& header : request.headers)
    {
        header_list =
            curl_slist_append(
                header_list,
                header.c_str()
            );
    }

    if (header_list)
    {
        curl_easy_setopt(
            curl,
            CURLOPT_HTTPHEADER,
            header_list
        );
    }

    CURLcode result =
        curl_easy_perform(curl);

    if (result != CURLE_OK)
    {
        std::cerr
            << "HTTP request failed: "
            << curl_easy_strerror(result)
            << "\n";
    }

    curl_easy_getinfo(
        curl,
        CURLINFO_RESPONSE_CODE,
        &response.status_code
    );

    char* content_type =
        nullptr;

    curl_easy_getinfo(
        curl,
        CURLINFO_CONTENT_TYPE,
        &content_type
    );

    if (content_type)
    {
        response.content_type =
            content_type;
    }

    if (header_list)
    {
        curl_slist_free_all(
            header_list
        );
    }

    curl_easy_cleanup(curl);

    return response;
}

HttpResponse HttpClient::post(
    const HttpRequest& request
)
{
    HttpResponse response;

    CURL* curl =
        curl_easy_init();

    if (!curl)
    {
        return response;
    }

    curl_easy_setopt(
        curl,
        CURLOPT_URL,
        request.url.c_str()
    );

    curl_easy_setopt(
        curl,
        CURLOPT_POST,
        1L
    );

    curl_easy_setopt(
        curl,
        CURLOPT_POSTFIELDS,
        request.body.c_str()
    );

    curl_easy_setopt(
        curl,
        CURLOPT_POSTFIELDSIZE,
        request.body.size()
    );

    curl_easy_setopt(
        curl,
        CURLOPT_WRITEFUNCTION,
        write_callback
    );

    curl_easy_setopt(
        curl,
        CURLOPT_WRITEDATA,
        &response.body
    );

    curl_easy_setopt(
        curl,
        CURLOPT_HEADERFUNCTION,
        header_callback
    );

    curl_easy_setopt(
        curl,
        CURLOPT_HEADERDATA,
        &response.headers
    );

    curl_easy_setopt(
        curl,
        CURLOPT_FOLLOWLOCATION,
        request.follow_redirects ? 1L : 0L
    );

    curl_easy_setopt(
        curl,
        CURLOPT_TIMEOUT,
        request.timeout
    );

    curl_easy_setopt(
        curl,
        CURLOPT_USERAGENT,
        request.user_agent.c_str()
    );

    curl_easy_setopt(
        curl,
        CURLOPT_ACCEPT_ENCODING,
        ""
    );

    curl_easy_setopt(
        curl,
        CURLOPT_SSL_VERIFYPEER,
        0L
    );

    curl_easy_setopt(
        curl,
        CURLOPT_SSL_VERIFYHOST,
        0L
    );

    if (!request.cookies.empty())
    {
        curl_easy_setopt(
            curl,
            CURLOPT_COOKIE,
            request.cookies.c_str()
        );
    }

    if (!request.referer.empty())
    {
        curl_easy_setopt(
            curl,
            CURLOPT_REFERER,
            request.referer.c_str()
        );
    }

    curl_slist* header_list =
        nullptr;

    for (const auto& header : request.headers)
    {
        header_list =
            curl_slist_append(
                header_list,
                header.c_str()
            );
    }

    if (header_list)
    {
        curl_easy_setopt(
            curl,
            CURLOPT_HTTPHEADER,
            header_list
        );
    }

    CURLcode result =
        curl_easy_perform(curl);

    if (result != CURLE_OK)
    {
        std::cerr
            << "HTTP request failed: "
            << curl_easy_strerror(result)
            << "\n";
    }

    curl_easy_getinfo(
        curl,
        CURLINFO_RESPONSE_CODE,
        &response.status_code
    );

    char* content_type =
        nullptr;

    curl_easy_getinfo(
        curl,
        CURLINFO_CONTENT_TYPE,
        &content_type
    );

    if (content_type)
    {
        response.content_type =
            content_type;
    }

    if (header_list)
    {
        curl_slist_free_all(
            header_list
        );
    }

    curl_easy_cleanup(curl);

    return response;
}
