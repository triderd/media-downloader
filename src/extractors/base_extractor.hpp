#pragma once

#include "../download_task.hpp"

#include <string>
#include <vector>

class BaseExtractor
{
public:
    virtual ~BaseExtractor() = default;

    virtual bool matches(
        const std::string& url
    ) = 0;

    virtual std::vector<DownloadTask>
    extract(
        const std::string& url
    ) = 0;
};
