#pragma once

#include <string>

struct HttpResponse
{
    long status_code = 0;

    std::string content_type;

    bool success = false;
};
