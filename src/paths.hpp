#pragma once

#include <string>
#include <filesystem>
#include <cstdlib>

namespace Paths
{
    inline std::string get_config_dir()
    {
        const char* xdg =
            std::getenv(
                "XDG_CONFIG_HOME"
            );

        if (xdg && xdg[0] != '\0')
        {
            return
                std::string(xdg)
                + "/media-downloader";
        }

        const char* home =
            std::getenv("HOME");

        if (home && home[0] != '\0')
        {
            return
                std::string(home)
                + "/.config/media-downloader";
        }

        return ".";
    }

    inline std::string resolve(
        const std::string& filename
    )
    {
        if (
            std::filesystem::exists(
                filename
            )
        )
        {
            return filename;
        }

        std::string config_path =
            get_config_dir()
            + "/"
            + filename;

        return config_path;
    }
}
