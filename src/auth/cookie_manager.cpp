#include "cookie_manager.hpp"
#include "../paths.hpp"

#include <fstream>
#include <sstream>

std::string
CookieManager::get_cookie_path(
    const std::string& site
)
{
    std::string filename =
        "cookies/"
        + site +
        ".txt";

    return Paths::resolve(
        filename
    );
}

std::string
CookieManager::load(
    const std::string& site
)
{
    std::string path =
        get_cookie_path(site);

    std::ifstream file(path);

    if (!file.is_open())
    {
        return "";
    }

    std::stringstream buffer;

    buffer << file.rdbuf();

    std::string cookies =
        buffer.str();

    return cookies;
}
