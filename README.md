# BitTorrent Client in C++

A lightweight BitTorrent client implementation in C++ that supports downloading single-file torrents. This client implements core BitTorrent protocol features including peer discovery, piece verification, and parallel downloading.

## Features

- Decode and encode BitTorrent metadata (bencode format)
- Display torrent information (tracker URL, file size, piece hashes)
- Peer discovery via tracker
- Peer handshake implementation
- Download individual pieces
- Download complete files with progress tracking
- Resume interrupted downloads
- Cross-platform support (Windows/Linux)

## Prerequisites

- C++17 or later
- libcurl
- CMake 3.10 or later (for CMake build)

### Installing Dependencies

#### Ubuntu/Debian
sudo apt-get install libcurl4-openssl-dev

#### Windows
- Install [vcpkg](https://github.com/microsoft/vcpkg)
- Install libcurl:
vcpkg install curl:x64-windows

## Building

### Using CMake (Recommended)
mkdir build
cd build
cmake ..
cmake --build .

### Manual Compilation

If CMake isn't available, you can compile directly using g++:

#### Linux
g++ src/Main.cpp -std=c++17 -lcurl -o bittorrent

#### Windows (using MinGW)
g++ src/Main.cpp -std=c++17 -lcurl -lws2_32 -o bittorrent.exe

## Usage

# Show help
./bittorrent help

# Decode a bencoded value
./bittorrent decode "encoded_value"

# Show torrent information
./bittorrent info path/to/file.torrent

# List peers from tracker
./bittorrent peers path/to/file.torrent

# Perform handshake with a peer
./bittorrent handshake path/to/file.torrent peer_ip:port

# Download a specific piece
./bittorrent download_piece -o output_file path/to/file.torrent piece_index

# Download complete file
./bittorrent download -o output_path path/to/file.torrent

## Project Structure

- src/Main.cpp - Entry point and command handling
- src/lib/
  - decode.hpp - Bencode encoding/decoding
  - torrent.hpp - Torrent metadata structures
  - peers.hpp - Peer discovery and communication
  - download.hpp - Download functionality
  - utils.hpp - Helper functions
  - sha1.hpp - SHA1 hash implementation

## Platform-Specific Notes

### Windows
- Uses Winsock2 for networking
- Requires ws2_32 library
- Automatically handles Winsock initialization/cleanup

### Linux
- Uses standard POSIX sockets
- No additional socket initialization required

## Limitations

- Currently supports only single-file torrents
- Does not support seeding
- Basic peer selection strategy
- No DHT support
