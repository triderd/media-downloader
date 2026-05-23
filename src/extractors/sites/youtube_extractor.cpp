#include "youtube_extractor.hpp"

#include <set>
#include <sstream>
#include <regex>
#include <iostream>
#include <nlohmann/json.hpp>
#include "../../http/http_client.hpp"


using json =
    nlohmann::json;


std::vector<std::string>
YouTubeExtractor::extract_playlist_urls(
    const std::string& html
)
{
    std::vector<std::string> urls;

    std::regex pattern(
        "\"videoId\":\"([^\"]+)\""
    );

    std::set<std::string> unique_ids;

    std::sregex_iterator begin(
        html.begin(),
        html.end(),
        pattern
    );

    std::sregex_iterator end;

    for (
        auto it = begin;
        it != end;
        ++it
    )
    {
        std::string video_id =
            (*it)[1];

        if (
            unique_ids.contains(
                video_id
            )
        )
        {
            continue;
        }

        unique_ids.insert(
            video_id
        );

        urls.push_back(
            "https://www.youtube.com/watch?v="
            + video_id
        );
    }

    return urls;
}



bool YouTubeExtractor::matches(
    const std::string& url
)
{
    return
        url.find("youtube.com")
        != std::string::npos
        ||
        url.find("youtu.be")
        != std::string::npos;
}

std::vector<DownloadTask>
YouTubeExtractor::extract(
    const std::string& url
)
{

    std::string playlist_name =
        YtDlpBackend::get_playlist_title(url);

    if (
        url.find("list=")
        != std::string::npos
    )
    {

        HttpClient client;

        HttpRequest http_req;

        http_req.url = url;

        http_req.user_agent =
            "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/136.0.0.0 Safari/537.36";

        http_req.headers =
        {
            "Accept-Language: en-US,en;q=0.9",
            "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8",
            "Connection: keep-alive",
            "Upgrade-Insecure-Requests: 1"
        };

        HttpResponse http_resp =
            client.get(http_req);

        std::string html =
            http_resp.body;

        auto playlist_urls =
            extract_playlist_urls(html);
        
        std::string playlist_title =
            YtDlpBackend::get_playlist_title(url);

        if (html.empty())
        {
            std::cout
                << "Failed to load playlist\n";

            return {};
        }


        auto formats =
            YtDlpBackend::get_formats(
                playlist_urls[0]
            );
        
        if (formats.empty())
        {
            std::cout
                << "No formats found\n";
        
            return {};
        }
        
        
        for (
            size_t i = 0;
            i < formats.size();
            ++i
        )
        {
            std::cout
                << "["
                << i + 1
                << "] "
                << formats[i].description;
        }
        
        
        std::string choice;
        
        std::cout
            << "\nChoose quality for playlist: ";
        
        std::getline(
            std::cin,
            choice
        );
        
        
        std::string selected_format;
        
        size_t plus_pos =
            choice.find('+');
        
        if (
            plus_pos != std::string::npos
        )
        {
            std::string left =
                choice.substr(
                    0,
                    plus_pos
                );
        
            std::string right =
                choice.substr(
                    plus_pos + 1
                );
        
            int video_index =
                std::stoi(left);
        
            int audio_index =
                std::stoi(right);
        
            selected_format =
                formats[
                    video_index - 1
                ].id
                +
                "+"
                +
                formats[
                    audio_index - 1
                ].id;
        }
        else
        {
            int index =
                std::stoi(choice);
        
            selected_format =
                formats[
                    index - 1
                ].id;
        }
        
        
        std::vector<DownloadTask> tasks;
        
        for (const auto& video_url : playlist_urls)
        {
            DownloadTask task;
        
            task.url =
                video_url;
        
            task.use_ytdlp =
                true;
        
            task.format_id =
                selected_format;

            task.playlist_title = 
		playlist_name;
       
            tasks.push_back(task);
        }



        return tasks;
    }



    auto formats =
        YtDlpBackend::get_formats(url);
    if (formats.empty())
    {
        std::cout
            << "No formats found\n";

        return {};
    }

    for (
        size_t i = 0;
        i < formats.size();
        ++i
    )
    {
        std::cout
            << "["
            << i + 1
            << "] "
            << formats[i].description;
    }


    std::string choice;
    
    std::cout
        << "\nChoose quality: ";
    
    std::getline(
        std::cin,
        choice
    );



    std::cin.ignore();


    std::string selected_format;
    
    size_t plus_pos =
        choice.find('+');
    
    if (
        plus_pos != std::string::npos
    )
    {
        std::string left =
            choice.substr(
                0,
                plus_pos
            );
    
        std::string right =
            choice.substr(
                plus_pos + 1
            );
    
        int video_index =
            std::stoi(left);
    
        int audio_index =
            std::stoi(right);
    
        if (
            video_index < 1
            ||
            video_index > formats.size()
            ||
            audio_index < 1
            ||
            audio_index > formats.size()
        )
        {
            std::cout
                << "Invalid choice\n";
    
            return {};
        }
    
        selected_format =
            formats[
                video_index - 1
            ].id
            +
            "+"
            +
            formats[
                audio_index - 1
            ].id;
    }
    else
    {
        int index =
            std::stoi(choice);
    
        if (
            index < 1
            ||
            index > formats.size()
        )
        {
            std::cout
                << "Invalid choice\n";
    
            return {};
        }
    
        selected_format =
            formats[
                index - 1
            ].id;
    }



    DownloadTask task;

    task.url =
        url;

    task.use_ytdlp =
        true;

    task.format_id =
        selected_format;

    return { task };
}
