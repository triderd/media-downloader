#include "tiktok_extractor.hpp"
#include "../../config.hpp"

bool TikTokExtractor::matches(
    const std::string& url
)
{
    return
        url.find("tiktok.com")
        != std::string::npos;
}

std::vector<DownloadTask>
TikTokExtractor::extract(
    const std::string& url
)
{
    DownloadTask task;

    task.url = url;

    task.use_ytdlp = true;

    task.format_id =
        Config::get_default_format();

    return { task };
}
