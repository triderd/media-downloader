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
    
    Downloader downloader;
    
    if (
        url.ends_with(".jpg")
        ||
        url.ends_with(".png")
        ||
        url.ends_with(".jpeg")
        ||
        url.ends_with(".gif")
        ||
        url.ends_with(".webp")
        ||
        url.ends_with(".mp4")
        ||
        url.ends_with(".webm")
    )
    {
        DownloadTask task;
    
        task.url = url;
    
        size_t last_slash =
            url.find_last_of('/');
    
        if (last_slash != std::string::npos)
        {
            task.filename =
                url.substr(
                    last_slash + 1
                );
        }
        else
        {
            task.filename =
                "downloaded_file";
        }
    
        bool success =
            downloader.download(
                task.url,
                task.filename,
                task.headers,
                task.cookies
            );
    
        if (!success)
        {
            std::cout
                << "Download failed\n";
        }
    
        return 0;
    }


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
            

        bool success;
        
        if (task.use_ytdlp)
        {
            std::string command =
                "yt-dlp -f mp4 \""
                + task.url
                + "\"";
        
            std::cout
                << "Running yt-dlp...\n";
        
            int result =
                system(command.c_str());
        
            success =
                (result == 0);
        }
        else
        {
            success =
                downloader.download(
                    task.url,
                    filename,
                    task.headers,
                    task.cookies
                );
        }



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
