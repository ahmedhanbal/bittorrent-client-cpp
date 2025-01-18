cmake_minimum_required(VERSION 3.13)

project(bittorrent-starter-cpp)

# Set C++ standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add include directories
include_directories(${PROJECT_SOURCE_DIR}/src)
include_directories(${PROJECT_SOURCE_DIR}/src/lib)

# Find required packages
find_package(CURL REQUIRED)
include_directories(${CURL_INCLUDE_DIR})

# Add source files
set(SOURCES
    src/Main.cpp
)

# Add header files
set(HEADERS
    src/lib/sha1.hpp
    src/lib/decode.hpp
    src/lib/utils.hpp
    src/lib/peers.hpp
    src/lib/download.hpp
    src/lib/torrent.hpp
)

# Create executable
add_executable(bittorrent ${SOURCES} ${HEADERS})

# Link libraries
target_link_libraries(bittorrent PRIVATE ${CURL_LIBRARIES})

# Add platform-specific libraries
if(WIN32)
    target_link_libraries(bittorrent PRIVATE wsock32 ws2_32)
endif()