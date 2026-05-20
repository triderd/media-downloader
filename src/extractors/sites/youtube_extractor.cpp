#include "youtube_extractor.hpp"

#include <curl/curl.h>
#include <sstream>
#include <regex>
#include <iostream>
#include <nlohmann/json.hpp>


using json =
    nlohmann::json;


static size_t write_callback(
    void* ptr,
    size_t size,
    size_t nmemb,
    void* userdata
)
{
    std::string* data =
        static_cast<std::string*>(
            userdata
        );

    size_t total =
        size * nmemb;

    data->append(
        static_cast<char*>(ptr),
        total
    );

    return total;
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

std::string
YouTubeExtractor::extract_video_id(
    const std::string& url
)
{
    std::regex pattern(
        R"([?&]v=([^&]+))"
    );

    std::smatch match;

    if (
        std::regex_search(
            url,
            match,
            pattern
        )
    )
    {
        return match[1];
    }

    return "";
}


std::string
YouTubeExtractor::http_get(
    const std::string& url
)
{
    CURL* curl =
        curl_easy_init();

    if (!curl)
    {
        return "";
    }

    std::string response;

    curl_easy_setopt(
        curl,
        CURLOPT_URL,
        url.c_str()
    );

    curl_easy_setopt(
        curl,
        CURLOPT_WRITEFUNCTION,
        write_callback
    );

    curl_easy_setopt(
        curl,
        CURLOPT_WRITEDATA,
        &response
    );

    curl_easy_setopt(
        curl,
        CURLOPT_FOLLOWLOCATION,
        1L
    );

    curl_easy_setopt(
        curl,
        CURLOPT_TIMEOUT,
        30L
    );

    curl_easy_setopt(
        curl,
        CURLOPT_USERAGENT,
        "Mozilla/5.0"
    );

    CURLcode result =
        curl_easy_perform(curl);

    if (result != CURLE_OK)
    {
        curl_easy_cleanup(curl);

        return "";
    }

    curl_easy_cleanup(curl);

    return response;
}


std::string
YouTubeExtractor::extract_player_json(
    const std::string& html
)
{
    size_t start =
        html.find(
            "ytInitialPlayerResponse"
        );

    if (start == std::string::npos)
    {
        return "";
    }

    start =
        html.find('{', start);

    if (start == std::string::npos)
    {
        return "";
    }

    int brace_count = 0;

    size_t end = start;

    for (
        size_t i = start;
        i < html.size();
        ++i
    )
    {
        if (html[i] == '{')
        {
            ++brace_count;
        }
        else if (html[i] == '}')
        {
            --brace_count;

            if (brace_count == 0)
            {
                end = i;

                break;
            }
        }
    }

    if (brace_count != 0)
    {
        return "";
    }

    return html.substr(
        start,
        end - start + 1
    );
}



std::string
YouTubeExtractor::url_decode(
    const std::string& value
)
{
    std::string result;

    for (
        size_t i = 0;
        i < value.size();
        ++i
    )
    {
        if (
            value[i] == '%'
            &&
            i + 2 < value.size()
        )
        {
            std::string hex =
                value.substr(i + 1, 2);

            char decoded =
                static_cast<char>(
                    std::stoi(
                        hex,
                        nullptr,
                        16
                    )
                );

            result += decoded;

            i += 2;
        }
        else if (value[i] == '+')
        {
            result += ' ';
        }
        else
        {
            result += value[i];
        }
    }

    return result;
}



std::string
YouTubeExtractor::extract_url_from_cipher(
    const std::string& cipher
)
{
    std::regex pattern(
        R"(url=([^&]+))"
    );

    std::smatch match;

    if (
        std::regex_search(
            cipher,
            match,
            pattern
        )
    )
    {
        return url_decode(
            match[1]
        );
    }

    return "";
}




std::vector<DownloadTask>
YouTubeExtractor::extract(
    const std::string& url
)
{
    std::cout
        << "YouTube extraction...\n";

    std::string video_id =
        extract_video_id(url);

    std::cout
        << "Video ID: "
        << video_id
        << "\n";
    

    std::string html =
        http_get(url);
    
    if (html.empty())
    {
        std::cout
            << "Failed to load page\n";
    
        return {};
    }
    
    std::cout
        << "HTML size: "
        << html.size()
        << "\n";


   size_t player_pos =
        html.find(
    	"ytInitialPlayerResponse"
        );
    
    if (
        player_pos ==
        std::string::npos
    )
    {
        std::cout
    	<< "Player JSON not found\n";
    
        return {};
    }
    
    std::cout
        << "Player JSON found\n"; 


    std::string player_json =
        extract_player_json(
            html
        );
    
    if (player_json.empty())
    {
        std::cout
            << "Failed to extract player JSON\n";
    
        return {};
    }
    
    std::cout
        << "Player JSON size: "
        << player_json.size()
        << "\n";

   json parsed;

    try
    {
	parsed =
	    json::parse(
		player_json
	    );
    }
    catch (const std::exception& e)
    {
	std::cout
	    << "JSON parse error: "
	    << e.what()
	    << "\n";

	return {};
    }

    std::cout
	<< "Player JSON parsed\n"; 

    std::vector<DownloadTask> tasks;


    if (
        !parsed.contains(
            "streamingData"
        )
    )
    {
        std::cout
            << "No streamingData\n";
    
        return {};
    }
    
    std::cout
        << "streamingData found\n";
    
    json streaming_data =
        parsed["streamingData"];


    
    if (
        !streaming_data.contains(
            "adaptiveFormats"
        )
    )
    {
        std::cout
            << "No adaptiveFormats\n";
    
        return {};
    }
    
    const auto& formats =
        streaming_data["adaptiveFormats"];


    std::cout
        << "Formats count: "
        << formats.size()
        << "\n";


    for (const auto& format : formats)
    {
        std::string stream_url;
        
        if (format.contains("url"))
        {
            stream_url =
                format["url"];
        }
        else if (
            format.contains(
                "signatureCipher"
            )
        )
        {
            std::string cipher =
                format[
                    "signatureCipher"
                ];
        
            stream_url =
                extract_url_from_cipher(
                    cipher
                );
        
            std::cout
                << "Extracted URL from cipher\n";
        }
        
        if (stream_url.empty())
        {
            continue;
        }   
    
	    
        std::string mime_type =
            format.value(
                "mimeType",
                ""
            );
    
        int bitrate =
            format.value(
                "bitrate",
                0
            );
    
        int width =
            format.value(
                "width",
                0
            );
    
        int height =
            format.value(
                "height",
                0
            );
    


        DownloadTask task;
        
        task.url =
            stream_url;


        std::string extension =
            "mp4";
        
        if (
            mime_type.find("webm")
            != std::string::npos
        )
        {
            extension = "webm";
        }


        task.filename =
            "youtube_"
            +
            std::to_string(width)
            +
            "x"
            +
            std::to_string(height)
            +
            "."
            +
            extension;


        tasks.push_back(task);

    
        std::cout
            << "Format:\n";
    
        std::cout
            << "  MIME: "
            << mime_type
            << "\n";
    
        std::cout
            << "  Resolution: "
            << width
            << "x"
            << height
            << "\n";
    
        std::cout
            << "  Bitrate: "
            << bitrate
            << "\n";
    
        std::cout
            << "  URL size: "
            << stream_url.size()
            << "\n\n";
    }

    std::cout
        << "Tasks created: "
        << tasks.size()
        << "\n";
    
    return tasks;
}
