#include "downloader.hpp"

#include <curl/curl.h>

#include <iomanip>
#include <string>
#include <cstdio>
#include <iostream>
#include <filesystem>
#include <algorithm>


std::chrono::steady_clock::time_point
Downloader::last_progress_update =
    std::chrono::steady_clock::now();

std::chrono::steady_clock::time_point
Downloader::download_start_time =
    std::chrono::steady_clock::now();



size_t Downloader::write_data(
    void* ptr,
    size_t size,
    size_t nmemb,
    void* userdata
)
{
    FILE* stream = static_cast<FILE*>(userdata);

    return fwrite(ptr, size, nmemb, stream);
}



int Downloader::progress_callback(
    void* clientp,
    curl_off_t dltotal,
    curl_off_t dlnow,
    curl_off_t ultotal,
    curl_off_t ulnow
)
{
    if (dltotal <= 0)
    {
        return 0;
    }

    auto now =
        std::chrono::steady_clock::now();

    auto elapsed =
        std::chrono::duration_cast<
            std::chrono::milliseconds
        >(
            now - last_progress_update
        ).count();

    if (elapsed < 200)
    {
        return 0;
    }

    last_progress_update = now;

    double progress =
        (double)dlnow /
        (double)dltotal * 100.0;



    auto total_elapsed =
        std::chrono::duration_cast<
            std::chrono::milliseconds
        >(
            now - download_start_time
        ).count();
    
        double seconds =
            total_elapsed / 1000.0;
    
        double speed_bytes =
            seconds > 0
            ? (double)dlnow / seconds
            : 0;

    double speed_mb =
        speed_bytes /
        1024.0 / 1024.0;

    double remaining_bytes =
        (double)dltotal -
        (double)dlnow;

    double eta_seconds =
        speed_bytes > 0
        ? remaining_bytes / speed_bytes
        : 0;


    double downloaded_mb =
        (double)dlnow /
        1024.0 / 1024.0;

    double total_mb =
        (double)dltotal /
        1024.0 / 1024.0;

    std::cout
        << "\r["
        << std::fixed
        << std::setprecision(1)
        << progress
        << "%] "
        << downloaded_mb
        << " MB / "
        << total_mb
        << " MB | "
        << speed_mb
        << " MB/s | ETA "
        << (int)eta_seconds
        << "s";
    return 0;
}




size_t Downloader::header_callback(
    char* buffer,
    size_t size,
    size_t nitems,
    void* userdata
)
{
    size_t total_size =
        size * nitems;

    std::string header(
        buffer,
        total_size
    );

    std::cout
        << "[HEADER] "
        << header;

    DownloadContext* context =
        static_cast<DownloadContext*>(userdata);

    std::string prefix =
        "Content-Disposition:";

    if (header.find(prefix) != std::string::npos)
    {
        size_t filename_pos =
            header.find("filename=");

        if (filename_pos != std::string::npos)
        {
            std::string filename =
                header.substr(
                    filename_pos + 9
                );

            if (!filename.empty())
            {
                if (filename.front() == '"')
                {
                    filename.erase(0, 1);
                }

                if (!filename.empty() &&
                    filename.back() == '\n')
                {
                    filename.pop_back();
                }

                if (!filename.empty() &&
                    filename.back() == '\r')
                {
                    filename.pop_back();
                }

                if (!filename.empty() &&
                    filename.back() == '"')
                {
                    filename.pop_back();
                }

                context->filename =
                    filename;

                std::cout
                    << "Detected filename: "
                    << filename
                    << "\n";
            }
        }
    }

    return total_size;
}



std::string Downloader::get_filename_from_url(
    const std::string& url
)
{
    size_t last_slash =
        url.find_last_of('/');

    if (last_slash == std::string::npos)
    {
        return "downloaded_file";
    }

    std::string filename =
        url.substr(last_slash + 1);

    if (filename.empty())
    {
        return "downloaded_file";
    }

    return filename;
}




bool Downloader::download(
    const std::string& url,
    const std::string& output_file,
    const std::vector<std::string>& headers,
    const std::string& cookies
)
{

    std::string final_name =
       output_file;

   if (final_name.empty())
   {
       final_name =
           get_filename_from_url(url);
   }



    CURL* curl = curl_easy_init();
    
    download_start_time =
        std::chrono::steady_clock::now();


    curl_off_t existing_size = 0;

    if (std::filesystem::exists(output_file))
    {
        existing_size =
            std::filesystem::file_size(output_file);

        std::cout
            << "Resuming from "
            << existing_size
            << " bytes\n";
    }



    if (!curl)
    {
        std::cerr << "Failed to init curl\n";
        return false;
    }


    std::string final_filename =
        output_file;
    
    if (final_filename.empty())
    {
        final_filename =
            "downloaded_file";
    }


    FILE* file = fopen(
        final_name.c_str(),
        existing_size > 0 ? "ab" : "wb"
    );



    if (!file)
    {
        std::cerr << "Failed to open file\n";

        curl_easy_cleanup(curl);

        return false;
    }
    
    DownloadContext context;

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());


    curl_easy_setopt(
        curl,
        CURLOPT_HTTP_VERSION,
        CURL_HTTP_VERSION_1_1
    );
    
    curl_easy_setopt(
        curl,
        CURLOPT_FOLLOWLOCATION,
        1L
    );
    
    curl_easy_setopt(
        curl,
        CURLOPT_TIMEOUT,
        60L
    );
    
    curl_easy_setopt(
        curl,
        CURLOPT_CONNECTTIMEOUT,
        15L
    );


    curl_easy_setopt(
        curl,
        CURLOPT_WRITEFUNCTION,
        write_data
    );

    curl_easy_setopt(
        curl,
        CURLOPT_WRITEDATA,
        file
    );

    
    curl_easy_setopt(
        curl,
        CURLOPT_HEADERFUNCTION,
        header_callback
    );

    curl_easy_setopt(
        curl,
        CURLOPT_HEADERDATA,
        &context
    );


    curl_easy_setopt(
        curl,
        CURLOPT_USERAGENT,
        "Mozilla/5.0"
    );



    curl_easy_setopt(
        curl,
        CURLOPT_FOLLOWLOCATION,
        1L
    );
    
    
        
        curl_easy_setopt(
            curl,
            CURLOPT_FOLLOWLOCATION,
            1L
        );


   /* 
    if (existing_size > 0)
    {
        curl_easy_setopt(
            curl,
            CURLOPT_RESUME_FROM_LARGE,
            existing_size
        );
    }*/



    curl_easy_setopt(
        curl,
        CURLOPT_XFERINFOFUNCTION,
        progress_callback
    );

    curl_easy_setopt(
        curl,
        CURLOPT_NOPROGRESS,
        0L
    );

    curl_slist* header_list = nullptr;

    for (const auto& header : headers)
    {
	

        std::cout
        << "[HEADER SEND] "
        << header
        << "\n";






        header_list =
            curl_slist_append(
                header_list,
                header.c_str()
            );
    }


    
    header_list =
        curl_slist_append(
            header_list,
            "Accept: image/avif,image/webp,image/apng,image/*,*/*;q=0.8"
        );
    
    header_list =
        curl_slist_append(
            header_list,
            "Accept-Language: en-US,en;q=0.9"
        );
    
    header_list =
        curl_slist_append(
            header_list,
            "Connection: keep-alive"
        );
    
    header_list =
        curl_slist_append(
            header_list,
            "Sec-Fetch-Dest: image"
        );
    
    header_list =
        curl_slist_append(
            header_list,
            "Sec-Fetch-Mode: no-cors"
        );
    
    header_list =
        curl_slist_append(
            header_list,
            "Sec-Fetch-Site: cross-site"
        );



    
    if (header_list)
    {
        curl_easy_setopt(
            curl,
            CURLOPT_HTTPHEADER,
            header_list
        );

        if (!cookies.empty())
        {
            curl_easy_setopt(
                curl,
                CURLOPT_COOKIE,
                cookies.c_str()
            );
        }

    }


    CURLcode result = curl_easy_perform(curl);

    
    

    
    long response_code = 0;

    curl_easy_getinfo(
        curl,
        CURLINFO_RESPONSE_CODE,
        &response_code
    );
    

    if (response_code >= 400)
    {
        std::cerr
            << "HTTP error: "
            << response_code
            << "\n";
    
        fclose(file);
    
        if (header_list)
        {
            curl_slist_free_all(
                header_list
            );
        }


        curl_easy_cleanup(curl);
    
        return false;
    }


    char* content_type = nullptr;

    curl_easy_getinfo(
        curl,
        CURLINFO_CONTENT_TYPE,
        &content_type
    );

    curl_off_t content_length = 0;

    curl_easy_getinfo(
        curl,
        CURLINFO_CONTENT_LENGTH_DOWNLOAD_T,
        &content_length
    );	
	
    std::cout<< "HTTP Status: "<< response_code<< "\n";

    if (content_type)
    {
    std::cout<< "Content-Type: "<< content_type<< "\n";
    }


    if (
        content_type &&
        !is_media_content_type(
            content_type
        )
    )
    {
        std::cerr
            << "Invalid content type: "
            << content_type
            << "\n";
    
        fclose(file);
    
        std::remove(
            final_filename.c_str()
        );
    
        curl_easy_cleanup(curl);
    
        return false;
    }

    

    if (content_type)
    {
        std::string type =
            content_type;
    
        bool is_media =
            type.starts_with("image/") ||
            type.starts_with("video/") ||
            type.starts_with("audio/") ||
            type == "application/octet-stream";
    
        if (!is_media)
        {
            std::cerr
                << "Warning: response may not be media\n";
        }
    }



    if (content_length > 0)
    {
    std::cout<< "Content-Length: "<< content_length<< " bytes\n";
    }


    fclose(file);

    curl_easy_cleanup(curl);

    if (result != CURLE_OK)
    {
        std::cerr
            << "Download failed\n"
	    << "CURL error: "
            << curl_easy_strerror(result)
            << "\n";

        return false;
    }

    return true;
}


std::string Downloader::extract_filename(
    const std::string& url
)
{
    size_t last_slash =
        url.find_last_of('/');

    if (last_slash == std::string::npos)
    {
        return "downloaded_file";
    }

    std::string filename =
        url.substr(last_slash + 1);

    size_t query_pos =
        filename.find('?');

    if (query_pos != std::string::npos)
    {
        filename =
            filename.substr(
                0,
                query_pos
            );
    }

    if (filename.empty())
    {
        return "downloaded_file";
    }

    return filename;
}


std::string Downloader::sanitize_filename(
    const std::string& filename
)
{
    std::string safe = filename;

    const std::string invalid_chars =
        "\\/:*?\"<>|";

    for (char& c : safe)
    {
        if (
            invalid_chars.find(c)
            != std::string::npos
        )
        {
            c = '_';
        }
    }

    if (safe.empty())
    {
        return "downloaded_file";
    }

    return safe;
}


bool Downloader::is_media_content_type(
    const std::string& content_type
)
{
    return
        content_type.starts_with("image/")
        ||
        content_type.starts_with("video/")
        ||
        content_type.starts_with("audio/")
        ||
        content_type == "application/octet-stream"
        ||
        content_type == "application/zip";
}
