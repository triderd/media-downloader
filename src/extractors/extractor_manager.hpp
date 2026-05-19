#pragma once

#include "base_extractor.hpp"

#include <memory>
#include <vector>

class ExtractorManager
{
public:
    ExtractorManager();

    std::vector<DownloadTask>
    extract(
        const std::string& url
    );

private:
    std::vector<
        std::unique_ptr<BaseExtractor>
    > extractors;
};
