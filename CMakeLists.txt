cmake_minimum_required(VERSION 3.13)

project(bittorrent-starter-cpp)

file(GLOB_RECURSE SOURCE_FILES src/*.cpp src/*.hpp)

set(CMAKE_CXX_STANDARD 23) # Enable the C++23 standard

add_executable(bittorrent ${SOURCE_FILES})

# add curl library
find_package(CURL REQUIRED)
include_directories(${CURL_INCLUDE_DIR})

# if on win add socket lib
if(WIN32)
    target_link_libraries(bittorrent PRIVATE 
        ${CURL_LIBRARIES}
        ws2_32
    )
else()
    target_link_libraries(bittorrent PRIVATE 
        ${CURL_LIBRARIES}
    )
endif()