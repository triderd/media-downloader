#pragma once

#include <vector>
#include <curl/curl.h>
#include <chrono>
#include <string>




struct DownloadContext
{
    std::string filename;
};







class Downloader
{
public:
    bool download(
        const std::string& url,
        const std::string& output_file,
        const std::vector<std::string>& headers = {},
	const std::string& cookies = ""
    );

     std::string extract_filename(
        const std::string& url
    );

    
    std::string sanitize_filename(
        const std::string& filename
    );


private:

    static std::chrono::steady_clock::time_point
    last_progress_update;
    

    static std::chrono::steady_clock::time_point
    download_start_time;

    static size_t write_data(
        void* ptr,
        size_t size,
        size_t nmemb,
        void* userdata
    );

    static int progress_callback(
        void* clientp,
        curl_off_t dltotal,
        curl_off_t dlnow,
        curl_off_t ultotal,
        curl_off_t ulnow

    );

    
   

    static size_t header_callback(
        char* buffer,
        size_t size,
        size_t nitems,
        void* userdata
    );


    std::string get_filename_from_url(
        const std::string& url
    );
};
