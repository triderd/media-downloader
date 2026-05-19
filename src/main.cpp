#include "downloader.hpp"
#include "extractors/extractor_manager.hpp"

#include <iostream>
#include <vector>
#include <string>

int main()
{
    std::string url;

    std::cout
        << "Page URL: ";

    std::getline(
        std::cin,
        url
    );

    ExtractorManager manager;

    std::cout
        << "Extracting media URLs...\n";

    std::vector<DownloadTask> tasks =
        manager.extract(url);

    if (tasks.empty())
    {
        std::cout
            << "No media found\n";

        return 1;
    }

    std::cout
        << "Found "
        << tasks.size()
        << " media files\n";

    Downloader downloader;

    int index = 1;

    for (const auto& task : tasks)
    {
        std::cout
            << "\n==========\n";

        std::cout
            << "Downloading:\n"
            << task.url
            << "\n";


        std::string filename =
            downloader.sanitize_filename(
                task.filename
                ); 
            
                
        bool success =
            downloader.download(
                task.url,
                filename,
		task.headers,
		task.cookies
            );

        if (!success)
        {
            std::cout
                << "Download failed\n";
        }

        ++index;
    }

    std::cout
        << "\nAll downloads finished\n";

    return 0;
}
