#include "twitter_extractor.hpp"
#include "../../config.hpp"

bool TwitterExtractor::matches(
    const std::string& url
)
{
    return
        url.find("twitter.com")
        != std::string::npos
        ||
        url.find("x.com")
        != std::string::npos;
}

std::vector<DownloadTask>
TwitterExtractor::extract(
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
