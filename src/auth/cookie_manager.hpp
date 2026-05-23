#pragma once

#include <string>

class CookieManager
{
public:

    static std::string load(
        const std::string& site
    );

private:

    static std::string get_cookie_path(
        const std::string& site
    );
};
