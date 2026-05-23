#pragma once

#include <string>
#include <vector>
#include <map>

struct HttpResponse
{
    long status_code = 0;

    std::string body;

    std::string content_type;

    std::map<std::string, std::string> headers;
};

struct HttpRequest
{
    std::string url;

    std::vector<std::string> headers;

    std::string cookies;

    std::string referer;

    std::string user_agent =
        "Mozilla/5.0 (compatible; media-downloader/1.0)";

    long timeout = 30L;

    bool follow_redirects = true;

    std::string body;
};

class HttpClient
{
public:

    HttpResponse get(
        const HttpRequest& request
    );

    HttpResponse post(
        const HttpRequest& request
    );

private:

    static size_t write_callback(
        void* ptr,
        size_t size,
        size_t nmemb,
        void* userdata
    );

    static size_t header_callback(
        char* buffer,
        size_t size,
        size_t nitems,
        void* userdata
    );
};
