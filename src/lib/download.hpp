#ifndef DOWNLOAD_HPP
#define DOWNLOAD_HPP

// this file contains fns needed to download a piece or complete file in a torrent

#include <iostream>
#include <condition_variable>
#include <mutex>
#include <queue>
#include "peers.hpp"

// WorkerQueue to manage indices of pieces to be downloaded
class WorkerQueue {
public:
    void add_piece(int piece_index) {
        std::lock_guard<std::mutex> lock(queue_mutex);
        queue.push(piece_index);
    }

    bool get_next_piece(int& piece_index) {
        std::lock_guard<std::mutex> lock(queue_mutex);
        if (queue.empty()) {
            return false;
        }
        piece_index = queue.front();
        queue.pop();
        return true;
    }

    bool contains(int piece_index) const {
        std::lock_guard<std::mutex> lock(queue_mutex);
        std::queue<int> temp = queue;
        while (!temp.empty()) {
            if (temp.front() == piece_index) {
                return true;
            }
            temp.pop();
        }
        return false;
    }

    bool empty() const {
        std::lock_guard<std::mutex> lock(queue_mutex);
        return queue.empty();
    }

private:
    std::queue<int> queue;
    mutable std::mutex queue_mutex;
};



// Function to download a specific piece
std::vector<uint8_t> download_piece(const std::string& peer_ip, int peer_port, 
                                  const Info& info, const std::vector<uint8_t>& info_hash, 
                                  int piece_index) {
    // Initialize socket
    WSAInitializer wsa;
    socket_t sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET_VALUE) {
        throw std::runtime_error("Failed to create socket");
    }
    
    // Connect to peer
    struct sockaddr_in peer_addr;
    peer_addr.sin_family = AF_INET;
    peer_addr.sin_port = htons(peer_port);
    if (inet_pton(AF_INET, peer_ip.c_str(), &peer_addr.sin_addr) <= 0) {
        CLOSE_SOCKET(sock);
        throw std::runtime_error("Invalid peer IP address");
    }
    
    if (connect(sock, (struct sockaddr*)&peer_addr, sizeof(peer_addr)) == SOCKET_ERROR_VALUE) {
        CLOSE_SOCKET(sock);
        throw std::runtime_error("Failed to connect to peer");
    }
    
    // Perform handshake
    std::string peer_id = generate_peer_id();
    std::string handshake;
    handshake.push_back(19);
    handshake += "BitTorrent protocol";
    handshake.append(8, '\0');
    handshake.append(info_hash.begin(), info_hash.end());
    handshake += peer_id;
    
    if (send(sock, handshake.c_str(), handshake.length(), 0) != handshake.length()) {
        CLOSE_SOCKET(sock);
        throw std::runtime_error("Failed to send handshake");
    }
    
    // Receive handshake response
    char response[68];
    if (recv(sock, response, sizeof(response), 0) != 68) {
        CLOSE_SOCKET(sock);
        throw std::runtime_error("Failed to receive handshake response");
    }
    
    // Wait for bitfield message
    PeerMessage bitfield = read_peer_message(sock);
    if (bitfield.id != 5) { // 5 is bitfield message ID
        CLOSE_SOCKET(sock);
        throw std::runtime_error("Expected bitfield message");
    }
    
    // Send interested message
    send_peer_message(sock, 2); // 2 is interested message ID
    
    // Wait for unchoke message
    PeerMessage unchoke = read_peer_message(sock);
    if (unchoke.id != 1) { // 1 is unchoke message ID
        CLOSE_SOCKET(sock);
        throw std::runtime_error("Expected unchoke message");
    }
    
    // Calculate piece length and block size
    const int BLOCK_SIZE = 16 * 1024; // 16 KiB
    int64_t piece_length = (piece_index == info.pieces.size() / 20 - 1) 
        ? (info.length % info.plength) : info.plength;
    
    // Download piece in blocks
    std::vector<uint8_t> piece_data;
    piece_data.reserve(piece_length);
    
    for (int64_t offset = 0; offset < piece_length; offset += BLOCK_SIZE) {
        // Calculate block length (last block might be smaller)
        int block_length = std::min(BLOCK_SIZE, static_cast<int>(piece_length - offset));
        
        // Prepare request message payload
        std::vector<uint8_t> request_payload(12);
        uint32_t index = htonl(piece_index);
        uint32_t begin = htonl(offset);
        uint32_t length = htonl(block_length);
        
        memcpy(request_payload.data(), &index, 4);
        memcpy(request_payload.data() + 4, &begin, 4);
        memcpy(request_payload.data() + 8, &length, 4);
        
        // Send request message
        send_peer_message(sock, 6, request_payload); // 6 is request message ID
        
        // Receive piece message
        PeerMessage piece_msg = read_peer_message(sock);
        if (piece_msg.id != 7) { // 7 is piece message ID
            CLOSE_SOCKET(sock);
            throw std::runtime_error("Expected piece message");
        }
        
        // Extract block data (skip first 8 bytes of payload which contain index and begin)
        piece_data.insert(piece_data.end(), 
                         piece_msg.payload.begin() + 8, 
                         piece_msg.payload.end());
    }
    
    // Verify piece hash
    SHA1 sha1;
    std::string piece_str(piece_data.begin(), piece_data.end());
    sha1.update(piece_str);
    std::string hash = sha1.final();
    
    // Convert the hex string to bytes for comparison
    std::vector<uint8_t> piece_hash;
    for (size_t i = 0; i < hash.length(); i += 2) {
        std::string byte = hash.substr(i, 2);
        piece_hash.push_back(static_cast<uint8_t>(std::stoi(byte, nullptr, 16)));
    }
    
    // Compare with expected hash from torrent file
    for (size_t i = 0; i < 20; i++) {
        if (piece_hash[i] != info.pieces[piece_index * 20 + i]) {
            CLOSE_SOCKET(sock);
            throw std::runtime_error("Piece hash verification failed");
        }
    }
    CLOSE_SOCKET(sock);
    return piece_data;
}

void handle_download_piece(const std::string& encoded_value, const std::string& output_path, int piece_index) {
    // Parse torrent file
    parse_torrent(encoded_value);
    
    // Get peers from tracker
    CURL* curl = curl_easy_init();
    if (!curl) {
        throw std::runtime_error("Failed to initialize CURL");
    }
    
    std::string url = torr.announce + "?info_hash=";
    for (uint8_t byte : torr.info.hash) {
        char hex[4];
        snprintf(hex, sizeof(hex), "%%%02x", byte);
        url += hex;
    }
    url += "&peer_id=PC0001-1234567890123";
    url += "&port=6881&uploaded=0&downloaded=0&left=" + std::to_string(torr.info.length);
    url += "&compact=1";
    
    std::string response;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    
    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        curl_easy_cleanup(curl);
        throw std::runtime_error("Failed to perform tracker request");
    }
    curl_easy_cleanup(curl);
    
    // Parse tracker response
    json decoded_response = decode_bencoded_value(response);
    std::string peers = decoded_response["peers"].get<std::string>();
    
    // Get first peer
    std::string peer_ip = format_ip_address(peers, 0);
    uint16_t peer_port = get_peer_port(peers, 4);
    
    // Download the piece
    std::vector<uint8_t> piece_data = download_piece(peer_ip, peer_port, torr.info, torr.info.hash, piece_index);
    
    // Save the piece to file
    std::ofstream output_file(output_path, std::ios::binary);
    if (!output_file) {
        throw std::runtime_error("Failed to create output file");
    }
    output_file.write(reinterpret_cast<const char*>(piece_data.data()), piece_data.size());
}

// Function to get default output path from torrent info
std::string get_default_output_path(const Info& info) {
    // If there's a path specified in the torrent, use the last component
    if (!info.path.empty()) {
        return info.path.back();
    }
    // Otherwise use the name field
    return info.name;
}

void show_progress(size_t downloaded, size_t total) {
    int bar_width = 50;
    float progress = static_cast<float>(downloaded) / static_cast<float>(total);
    int pos = static_cast<int>(bar_width * progress);
    std::cout << "[";
    for (int i = 0; i < bar_width; ++i) {
        if (i < pos) std::cout << "=";
        else if (i == pos) std::cout << ">";
        else std::cout << " ";
    }
    std::cout << "] " << static_cast<int>(progress * 100.0) << "%\r";
    std::cout.flush();
}

void recheck_existing_file(const std::string& output_path, const Info& info, WorkerQueue& worker_queue) {
    std::ifstream file(output_path, std::ios::binary);
    if (!file) {
        std::cerr << "No existing file found. All pieces will be added to the queue.\n";
        for (size_t i = 0; i < info.pieces.size() / 20; ++i) {
            worker_queue.add_piece(static_cast<int>(i));
        }
        return;
    }

    const size_t piece_count = info.pieces.size() / 20;
    for (size_t i = 0; i < piece_count; ++i) {
        size_t piece_length = (i == piece_count - 1) ? (info.length % info.plength) : info.plength;
        std::vector<char> buffer(piece_length);  // Use char buffer for reading file

        file.seekg(i * info.plength);
        file.read(buffer.data(), piece_length);

        if (file.gcount() != static_cast<std::streamsize>(piece_length)) {
            worker_queue.add_piece(static_cast<int>(i));
            continue;
        }

        // Convert the char buffer to a string
        std::string piece_str(buffer.begin(), buffer.end());

        SHA1 sha1;
        sha1.update(piece_str);  // Pass the string to sha1.update
        std::string piece_hash_bytes = sha1.final();

        std::string expected_hash(info.pieces.begin() + i * 20, info.pieces.begin() + (i + 1) * 20);
        if (piece_hash_bytes == expected_hash) {
            std::cout << "Piece " << i << " verified.\n";
        } else {
            worker_queue.add_piece(static_cast<int>(i));
        }
    }
}

// Function to download complete file
void download_complete_file(const std::string& encoded_value, const std::string& output_path) {
    parse_torrent(encoded_value);
    
    std::string actual_output_path = (output_path == "default") ? get_default_output_path(torr.info) : output_path;
    std::cerr << "Using output path: " << actual_output_path << std::endl;

    WorkerQueue worker_queue;

    // Create empty file if it doesn't exist
    {
        std::ofstream init_file(actual_output_path, std::ios::binary);
        if (!init_file) {
            throw std::runtime_error("Failed to create output file: " + actual_output_path);
        }
        // Pre-allocate the file size
        init_file.seekp(torr.info.length - 1);
        init_file.put('\0');
        init_file.close();
    }

    recheck_existing_file(actual_output_path, torr.info, worker_queue);

    if (worker_queue.empty()) {
        std::cout << "File is already complete and valid. Nothing to download.\n";
        return;
    }

    CURL* curl = curl_easy_init();
    if (!curl) {
        throw std::runtime_error("Failed to initialize CURL");
    }

    std::string url = torr.announce + "?info_hash=";
    for (uint8_t byte : torr.info.hash) {
        char hex[4];
        snprintf(hex, sizeof(hex), "%%%02x", byte);
        url += hex;
    }
    url += "&peer_id=PC0001-1234567890123";
    url += "&port=6881&uploaded=0&downloaded=0&left=" + std::to_string(torr.info.length);
    url += "&compact=1";

    std::string response;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    
    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        curl_easy_cleanup(curl);
        throw std::runtime_error("Failed to perform tracker request");
    }
    curl_easy_cleanup(curl);

    json decoded_response = decode_bencoded_value(response);
    std::string peers = decoded_response["peers"].get<std::string>();
    
    std::string peer_ip = format_ip_address(peers, 0);
    uint16_t peer_port = get_peer_port(peers, 4);
    
    size_t downloaded_size = 0;
    size_t total_pieces = torr.info.pieces.size() / 20;
    show_progress(downloaded_size, torr.info.length);
    // Count already downloaded pieces for progress bar
    std::ifstream count_file(actual_output_path, std::ios::binary);
    if (count_file) {
        for (size_t i = 0; i < total_pieces; ++i) {
            if (!worker_queue.contains(static_cast<int>(i))) {
                downloaded_size += (i == total_pieces - 1) ? 
                    (torr.info.length % torr.info.plength) : torr.info.plength;
            }
        }
    }

    // Open file in binary mode for reading and writing
    std::fstream output_file(actual_output_path, std::ios::binary | std::ios::in | std::ios::out);
    if (!output_file) {
        throw std::runtime_error("Failed to open output file for writing: " + actual_output_path);
    }

    int piece_index;
    int retry_count = 0;
    const int MAX_RETRIES = 3;

    while (worker_queue.get_next_piece(piece_index)) {
        try {
            std::vector<uint8_t> piece_data = download_piece(peer_ip, peer_port, torr.info, torr.info.hash, piece_index);
            
            // Seek to the correct position and write the piece
            output_file.seekp(static_cast<std::streampos>(piece_index) * torr.info.plength);
            if (!output_file.good()) {
                throw std::runtime_error("Failed to seek in output file");
            }

            output_file.write(reinterpret_cast<const char*>(piece_data.data()), piece_data.size());
            if (!output_file.good()) {
                throw std::runtime_error("Failed to write piece to file");
            }

            // Flush after each piece to ensure it's written to disk
            output_file.flush();
            
            downloaded_size += piece_data.size();
            show_progress(downloaded_size, torr.info.length);
            retry_count = 0;  // Reset retry counter after successful download
        }
        catch (const std::exception& e) {
            std::cerr << "\nError downloading piece " << piece_index << ": " << e.what() << std::endl;
            if (++retry_count < MAX_RETRIES) {
                std::cerr << "Retrying... (Attempt " << retry_count + 1 << " of " << MAX_RETRIES << ")" << std::endl;
                worker_queue.add_piece(piece_index);  // Put the piece back in the queue
                continue;
            }
            throw;  // Re-throw after max retries
        }
    }

    output_file.close();
    std::cout << "\nDownload completed successfully!" << std::endl;
}

#endif