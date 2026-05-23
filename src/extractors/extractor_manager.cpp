#include "extractor_manager.hpp"

#include "sites/pixiv_extractor.hpp"
#include "sites/youtube_extractor.hpp"
#include "sites/telegram_extractor.hpp"
#include "sites/danbooru_extractor.hpp"
#include "sites/pattern_extractor.hpp"
#include "sites/ytdlp_extractor.hpp"

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
            TelegramExtractor
        >()
    );

    extractors.push_back(
        std::make_unique<
            DanbooruExtractor
        >()
    );

    extractors.push_back(
        std::make_unique<
            PatternExtractor
        >()
    );

    extractors.push_back(
        std::make_unique<
            YtDlpExtractor
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
            return extractor->extract(
                url
            );
        }
    }

    return {};
}
