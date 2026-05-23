cmake_minimum_required(VERSION 3.16)

project(media_downloader)

set(CMAKE_CXX_STANDARD 20)

find_package(CURL QUIET)

if(NOT CURL_FOUND)
    if(DEFINED CURL_INCLUDE_DIR AND DEFINED CURL_LIBRARY)
        set(CURL_INCLUDE_DIRS ${CURL_INCLUDE_DIR})
        set(CURL_LIBRARIES ${CURL_LIBRARY})
    else()
        message(FATAL_ERROR "CURL not found. Install libcurl or set CURL_INCLUDE_DIR/CURL_LIBRARY")
    endif()
endif()

add_executable(
    media_downloader

    src/main.cpp
    src/downloader.cpp
    src/ytdlp_backend.cpp
    src/config.cpp
    src/http/http_client.cpp
    src/auth/cookie_manager.cpp

    src/extractors/extractor_manager.cpp
    src/extractors/sites/youtube_extractor.cpp
    src/extractors/sites/pixiv_extractor.cpp
    src/extractors/sites/telegram_extractor.cpp
    src/extractors/sites/danbooru_extractor.cpp
    src/extractors/sites/pattern_extractor.cpp
    src/extractors/sites/ytdlp_extractor.cpp
)

target_include_directories(
    media_downloader
    PRIVATE
    src
    ${CURL_INCLUDE_DIRS}
)

target_link_libraries(
    media_downloader
    PRIVATE
    ${CURL_LIBRARIES}
)

install(
    TARGETS media_downloader
    RENAME mdw
    DESTINATION bin
)

install(
    DIRECTORY cookies/
    DESTINATION share/media-downloader/cookies
    OPTIONAL
)

install(
    FILES config.json patterns.json
    DESTINATION share/media-downloader
)
