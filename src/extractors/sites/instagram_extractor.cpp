#include "instagram_extractor.hpp"
#include "../../config.hpp"

bool InstagramExtractor::matches(
    const std::string& url
)
{
    return
        url.find("instagram.com")
        != std::string::npos;
}

std::vector<DownloadTask>
InstagramExtractor::extract(
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
