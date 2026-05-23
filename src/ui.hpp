#pragma once

#include <iostream>
#include <string>
#include <vector>

namespace UI
{
    inline void clear()
    {
        std::cout
            << "\033[2J\033[1;1H";
    }

    inline void title(
        const std::string& text
    )
    {
        std::cout
            << "\033[1;36m"
            << text
            << "\033[0m\n\n";
    }

    inline void success(
        const std::string& text
    )
    {
        std::cout
            << "\033[1;32m"
            << text
            << "\033[0m\n";
    }

    inline void error(
        const std::string& text
    )
    {
        std::cout
            << "\033[1;31m"
            << text
            << "\033[0m\n";
    }

    inline void info(
        const std::string& text
    )
    {
        std::cout
            << "\033[1;33m"
            << text
            << "\033[0m\n";
    }

    inline void separator()
    {
        std::cout
            << "\033[90m"
            << "------------------------------------------------------------"
            << "\033[0m\n";
    }
}
