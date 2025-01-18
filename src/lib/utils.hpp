#ifndef UTILS_HPP
#define UTILS_HPP

// this file contains helper fns 

#include "decode.hpp"
#include "sha1.hpp"
#include "torrent.hpp"

// convert bytes to hex and divides it into 20 bytes each
std::vector<std::string> bytes_to_hex(const std::vector<uint8_t>& pieces) {
    static const char* hex_chars = "0123456789abcdef";
    size_t size = pieces.size();

    if (size % 20 != 0) {
        throw std::runtime_error("Invalid pieces length");
    }

    std::vector<std::string> hex(size / 20); // 20 bytes per piece
    for (size_t i = 0; i < size; ++i) {
        size_t index = i / 20;
        uint8_t byte = pieces[i];
        hex[index] += hex_chars[byte >> 4];    
        hex[index] += hex_chars[byte & 0x0F]; 
    }
    return hex;
}

// parse torrent from string and calculate Tracker URL,Length and info hash
void parse_torrent(const std::string& encoded_value) {
    json decoded_value = decode_bencoded_value(encoded_value);
    std::string info_bencoded = bencode_decoded_value(decoded_value["info"]);

    // Get the raw pieces string from JSON
    std::string pieces_str = decoded_value["info"]["pieces"].get<std::string>();
    
    // Convert the raw string to vector<uint8_t> for pieces
    std::vector<uint8_t> pieces_data;
    pieces_data.reserve(pieces_str.size());
    for (char c : pieces_str) {
        pieces_data.push_back((uint8_t)(c));
    }

     // Calculate info hash
    SHA1 sha1;
    sha1.update(info_bencoded);
    std::string hex_hash = sha1.final();
    
    // Convert hex string to binary data
    std::vector<uint8_t> binary_hash;
    binary_hash.reserve(20);
    
    for (size_t i = 0; i < hex_hash.length(); i += 2) {
        std::string byte = hex_hash.substr(i, 2);
        binary_hash.push_back(static_cast<uint8_t>(std::stoi(byte, nullptr, 16)));
    }

    // Populate contents of torr
    torr.announce = decoded_value.contains("announce") ? decoded_value["announce"].get<std::string>() : "";
    torr.info.name = decoded_value["info"].contains("name") ? decoded_value["info"]["name"].get<std::string>() : "";
    torr.info.plength = decoded_value["info"].contains("piece length") ? decoded_value["info"]["piece length"].get<int64_t>() : 0;
    torr.info.length = decoded_value["info"].contains("length") ? decoded_value["info"]["length"].get<int64_t>() : 0;
    torr.info.pieces = pieces_data;
    torr.info.hash = binary_hash;
    // If path exists, use it, otherwise use name 
    if (decoded_value["info"].contains("path")) {
        torr.info.path = decoded_value["info"]["path"].get<std::vector<std::string>>();
    } else {
        torr.info.path = {torr.info.name};
    }
    
}


void info_torrent(const std::string& encoded_value) {
    // parse content
    parse_torrent(encoded_value);
    // get hex pieces
    std::vector<std::string> hex = bytes_to_hex(torr.info.pieces);

    // Print the torrent information
    std::cout << "Tracker URL: " << torr.announce << std::endl;
    std::cout << "Length: " << torr.info.length << std::endl;
    std::cout << "Info Hash: ";
    for (uint8_t byte : torr.info.hash) {
        printf("%02x", byte);
    }
    std::cout << std::endl;
    std::cout << "Name: " << torr.info.name << std::endl;
    std::cout << "Piece Length: " << torr.info.plength << std::endl;
    // std::cout << "Path: ";
    // for (const auto& path : torr.info.path) {
    //     std::cout << path << "/";
    // }
    // std::cout <<'\n';
    std::cout << "Pieces: \n";
    for (const auto& piece : hex) {
        std::cout << piece << " \n";
    }
}

// read content of torrent file and return it as string
std::string read_file(const std::string& torrent_file) {
    std::ifstream file(torrent_file, std::ios::binary);
    std::stringstream buffer;
    if (file) {
        buffer << file.rdbuf();
        file.close();
        return buffer.str();
    } else {
        throw std::runtime_error("Failed to open file: " + torrent_file);
    }
}

#endif