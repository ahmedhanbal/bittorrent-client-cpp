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
```sudo apt-get install libcurl4-openssl-dev```

#### Windows
- Install [vcpkg](https://github.com/microsoft/vcpkg)
- Install libcurl:
vcpkg install curl:x64-windows

## Building

### Using CMake (Recommended)
```
cmake .
make
```
### Manual Compilation

If CMake isn't available, you can compile directly using g++:

#### Linux
```
g++ src/Main.cpp -std=c++17 -lcurl -o bittorrent
```
#### Windows (using MinGW)
```
g++ src/Main.cpp -std=c++17 -lcurl -lws2_32 -o bittorrent.exe
```


## Command Reference

| Command | Usage | Description |
|---------|-------|-------------|
| `help` | `./bittorrent help` | Display all available commands and their usage |
| `decode` | `./bittorrent decode <encoded_value>` | Decode a bencoded value and display as JSON |
| `info` | `./bittorrent info <torrent_file>` | Show detailed information about a torrent file including:<br>- Tracker URL<br>- File length<br>- Info hash<br>- Piece length<br>- Piece hashes |
| `peers` | `./bittorrent peers <torrent_file>` | List all peers sharing this torrent from tracker |
| `handshake` | `./bittorrent handshake <torrent_file> <peer_ip:port>` | Perform BitTorrent handshake with a specific peer |
| `download_piece` | `./bittorrent download_piece -o <output_file> <torrent_file> <piece_index>` | Download a specific piece from the torrent |
| `download` | `./bittorrent download -o <output_path> <torrent_file>` | Download the complete file from the torrent<br>Use "default" as output_path to use original filename |

### Examples

```bash
# Decode a bencoded string
./bittorrent decode "d8:announce3:url4:infod4:name6:sample12:piece lengthi16384e6:pieces20:aabbccddee..."

# Get information about a torrent
./bittorrent info sample.torrent

# List peers
./bittorrent peers sample.torrent

# Download piece 0 to output.txt
./bittorrent download_piece -o output.txt sample.torrent 0

# Download complete file using original filename
./bittorrent download -o default sample.torrent

# Download complete file with custom name
./bittorrent download -o my_movie.mp4 sample.torrent
```

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
