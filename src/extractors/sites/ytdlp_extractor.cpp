#include "ytdlp_extractor.hpp"
#include "../../ytdlp_backend.hpp"
#include "../../config.hpp"

bool YtDlpExtractor::matches(
    const std::string& url
)
{
    return true;
}

std::vector<DownloadTask>
YtDlpExtractor::extract(
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
