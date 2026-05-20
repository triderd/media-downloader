cmake_minimum_required(VERSION 3.16)

project(media_downloader)

set(CMAKE_CXX_STANDARD 20)

add_executable(
    media_downloader
    

    src/extractors/sites/pixiv_extractor.cpp
    src/extractors/sites/youtube_extractor.cpp


    src/main.cpp
    src/downloader.cpp

    src/extractors/generic_extractor.cpp
    src/extractors/extractor_manager.cpp
)


find_package(CURL REQUIRED)

target_link_libraries(media_downloader
    CURL::libcurl
)
