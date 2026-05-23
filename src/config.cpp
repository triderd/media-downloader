#include "config.hpp"
#include "paths.hpp"

#include <fstream>
#include <nlohmann/json.hpp>

using json =
    nlohmann::json;

std::string Config::download_dir =
    ".";

std::string Config::default_format =
    "bestvideo+bestaudio/best";

int Config::concurrent_downloads =
    3;

void Config::load(
    const std::string& path
)
{
    std::string resolved =
        Paths::resolve(path);

    std::ifstream file(
        resolved
    );

    if (!file.is_open())
    {
        return;
    }

    json config;

    try
    {
        config =
            json::parse(file);
    }
    catch (const std::exception& e)
    {
        return;
    }

    if (config.contains("download_dir"))
    {
        download_dir =
            config["download_dir"];
    }

    if (config.contains("default_format"))
    {
        default_format =
            config["default_format"];
    }

    if (config.contains("concurrent_downloads"))
    {
        concurrent_downloads =
            config["concurrent_downloads"];
    }
}

std::string
Config::get_download_dir()
{
    return download_dir;
}

std::string
Config::get_default_format()
{
    return default_format;
}

int
Config::get_concurrent_downloads()
{
    return concurrent_downloads;
}
