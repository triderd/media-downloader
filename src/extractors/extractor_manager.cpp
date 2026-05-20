#include "extractor_manager.hpp"
#include "generic_extractor.hpp"


#include "sites/pixiv_extractor.hpp"
#include "sites/youtube_extractor.hpp"

#include <iostream>


ExtractorManager::ExtractorManager()
{

    
    extractors.push_back(
        std::make_unique<
            PixivExtractor
        >()
    );

    extractors.push_back(
        std::make_unique<
            YouTubeExtractor
        >()
    );

    extractors.push_back(
        std::make_unique<
            GenericExtractor
        >()
    );
}


std::vector<DownloadTask>
ExtractorManager::extract(
    const std::string& url
)
{
    for (auto& extractor : extractors)
    {
        if (extractor->matches(url))
        {
            std::cout
                << "Using extractor...\n";

            return extractor->extract(
                url
            );
        }
    }

    return {};
}
