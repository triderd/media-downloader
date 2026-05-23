#pragma once

#include <string>

class Config
{
public:
    static void load(
        const std::string& path =
        "config.json"
    );

    static std::string
    get_download_dir();

    static std::string
    get_default_format();

    static int
    get_concurrent_downloads();

private:
    static std::string download_dir;

    static std::string default_format;

    static int concurrent_downloads;
};
