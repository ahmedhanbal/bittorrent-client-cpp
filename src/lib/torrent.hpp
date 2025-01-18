#ifndef TORRENT_HPP
#define TORRENT_HPP

// This file contains the data structures for the contents of a torrent file (.torrent)


#include <string>
#include <vector>
#include <cstdint>

// info dictionary
struct Info {
    
    //The name key maps to a UTF-8 encoded string.
    // It is the suggested name to save the file (or directory) as.
    std::string name;

    //piece length maps to the number of bytes in each piece the file is split into.
    // For the purposes of transfer, files are split into fixed-size pieces which are all the same length except for possibly the last one which may be truncated. 
    //piece length is almost always a power of two, most commonly 2 18 = 256 K
    int64_t plength; // piece length

    // pieces maps to a string whose length is a multiple of 20. It is to be subdivided into strings of length 20, each of which is the SHA1 hash of the piece at the corresponding index.
    // since it's not utf-8 i need datatype other than char/string
    std::vector<uint8_t> pieces;

    // let's just do single file case only
    //length - The length of the file, in bytes.
    int64_t length;

    // path - A list of UTF-8 encoded strings corresponding to subdirectory names, the last of which is the actual file name (a zero length list is an error case).
    std::vector<std::string> path;

    // hash of info
    std::vector<uint8_t> hash;
};

struct Torrent{
    
    //announce
    //The URL of the tracker.
    std::string announce;

    //info
    //This maps to a dictionary, with keys described in struct Info.
    Info info;
} torr;

#endif 