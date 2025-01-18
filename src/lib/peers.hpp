#ifndef PEERS_HPP
#define PEERS_HPP

// this file contains fns for peers request using curl and sockets

#include <string>
#include <vector>
#include <cstdint>
#include <stdexcept>
#include <curl/curl.h>
#include <random>
#include "utils.hpp"

// Platform-independent socket headers
#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
#endif


// Platform-independent socket type
#ifdef _WIN32
    typedef SOCKET socket_t;
    #define INVALID_SOCKET_VALUE INVALID_SOCKET
    #define SOCKET_ERROR_VALUE SOCKET_ERROR
    #define CLOSE_SOCKET closesocket
#else
    typedef int socket_t;
    #define INVALID_SOCKET_VALUE (-1)
    #define SOCKET_ERROR_VALUE (-1)
    #define CLOSE_SOCKET close
#endif

// Windows-specific socket initialization
class WSAInitializer {
public:
    WSAInitializer() {
        #ifdef _WIN32
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            throw std::runtime_error("Failed to initialize Winsock");
        }
        #endif
    }
    ~WSAInitializer() {
        #ifdef _WIN32
        WSACleanup();
        #endif
    }
};

// Function to read a peer message
struct PeerMessage {
    uint32_t length;
    uint8_t id;
    std::vector<uint8_t> payload;
};

PeerMessage read_peer_message(socket_t sock) {
    PeerMessage msg;
    
    // Read message length (4 bytes)
    uint32_t length_buffer;
    if (recv(sock, reinterpret_cast<char*>(&length_buffer), 4, 0) != 4) {
        throw std::runtime_error("Failed to read message length");
    }
    msg.length = ntohl(length_buffer); // Convert from network byte order
    
    // If length is 0, it's a keep-alive message
    if (msg.length == 0) {
        msg.id = 0xFF; // Special ID for keep-alive
        return msg;
    }
    
    // Read message ID (1 byte)
    if (recv(sock, reinterpret_cast<char*>(&msg.id), 1, 0) != 1) {
        throw std::runtime_error("Failed to read message ID");
    }
    
    // Read payload
    size_t payload_length = msg.length - 1; // Subtract 1 for message ID
    msg.payload.resize(payload_length);
    size_t total_received = 0;
    while (total_received < payload_length) {
        ssize_t received = recv(sock, reinterpret_cast<char*>(msg.payload.data() + total_received), 
                              payload_length - total_received, 0);
        if (received <= 0) {
            throw std::runtime_error("Failed to read message payload");
        }
        total_received += received;
    }
    
    return msg;
}

// Function to send a peer message
void send_peer_message(socket_t sock, uint8_t id, const std::vector<uint8_t>& payload = {}) {
    // Calculate total message length (payload + 1 byte for id)
    uint32_t length = htonl(payload.size() + 1); // Convert to network byte order
    
    // Send length prefix
    if (send(sock, reinterpret_cast<char*>(&length), 4, 0) != 4) {
        throw std::runtime_error("Failed to send message length");
    }
    
    // Send message ID
    if (send(sock, reinterpret_cast<char*>(&id), 1, 0) != 1) {
        throw std::runtime_error("Failed to send message ID");
    }
    
    // Send payload if any
    if (!payload.empty()) {
        if (send(sock, reinterpret_cast<const char*>(payload.data()), payload.size(), 0) != payload.size()) {
            throw std::runtime_error("Failed to send message payload");
        }
    }
}

// Callback function for CURL to write response data
size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
    userp->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// Function to format peer IP address
std::string format_ip_address(const std::string& peers, size_t offset) {
    return std::to_string(static_cast<uint8_t>(peers[offset])) + "." +
           std::to_string(static_cast<uint8_t>(peers[offset + 1])) + "." +
           std::to_string(static_cast<uint8_t>(peers[offset + 2])) + "." +
           std::to_string(static_cast<uint8_t>(peers[offset + 3]));
}

// Function to get port number from peer data
uint16_t get_peer_port(const std::string& peers, size_t offset) {
    return (static_cast<uint16_t>(static_cast<uint8_t>(peers[offset])) << 8) |
           static_cast<uint16_t>(static_cast<uint8_t>(peers[offset + 1]));
}

// Request peers from the tracker
void peers_request(const std::string& encoded_value) {
    // parse file content
    parse_torrent(encoded_value);
    // init curl
    CURL* curl = curl_easy_init();
    if (!curl) {
        throw std::runtime_error("Failed to initialize CURL");
    }

    // Construct tracker URL with required parameters
    std::string url = torr.announce;
    url += "?info_hash=";
    
    // URL encode the binary info hash
    for (uint8_t byte : torr.info.hash) {
        char hex[4];
        snprintf(hex, sizeof(hex), "%%%02x", byte);
        url += hex;
    }
    std:: string peer_id = "PC0001-1234567890123";
    url += "&peer_id=" + peer_id;
    url += "&port=6881"; 
    url += "&uploaded=0";
    url += "&downloaded=0";
    url += "&left=" + std::to_string(torr.info.length);
    url += "&compact=1";

    // Debug print the full URL
    // std::cout << "Request URL: " << url << std::endl;

    // Set up CURL options
    std::string response;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    
    // Perform the request
    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        curl_easy_cleanup(curl);
        throw std::runtime_error("Failed to perform request: " + std::string(curl_easy_strerror(res)));
    }
    
    // Clean up CURL
    curl_easy_cleanup(curl);
    // Decode the tracker response (it's bencoded)
    json decoded_response = decode_bencoded_value(response);
    
    if(decoded_response.contains("failure reason")){
        std::cerr << "Tracker response: " << decoded_response["failure reason"].get<std::string>() << std::endl;
        return;
    }
    // Get the peers data
    std::string peers = decoded_response["peers"].get<std::string>();
    
    // Each peer is represented by 6 bytes (4 for IP, 2 for port)
    const size_t PEER_SIZE = 6;
    size_t num_peers = peers.length() / PEER_SIZE;
    
    // Print each peer's IP and port
    for (size_t i = 0; i < num_peers; i++) {
        size_t offset = i * PEER_SIZE;
        std::string ip = format_ip_address(peers, offset);
        uint16_t port = get_peer_port(peers, offset + 4);
        
        std::cout << ip << ":" << port << std::endl;
    }
}



// Function to generate random peer ID
std::string generate_peer_id() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);
    
    std::string peer_id;
    peer_id.reserve(20);
    for (int i = 0; i < 20; i++) {
        peer_id.push_back(static_cast<char>(dis(gen)));
    }
    return peer_id;
}

// Function to perform handshake with peer
std::string perform_handshake(const std::string& peer_ip, int peer_port, const std::vector<uint8_t>& info_hash) {
    // Initialize WinSock if on Windows
    WSAInitializer wsa;
    
    // Create socket
    socket_t sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET_VALUE) {
        throw std::runtime_error("Failed to create socket");
    }

    // Set up peer address
    struct sockaddr_in peer_addr;
    peer_addr.sin_family = AF_INET;
    peer_addr.sin_port = htons(peer_port);
    if (inet_pton(AF_INET, peer_ip.c_str(), &peer_addr.sin_addr) <= 0) {
        CLOSE_SOCKET(sock);
        throw std::runtime_error("Invalid peer IP address");
    }

    // Connect to peer
    if (connect(sock, (struct sockaddr*)&peer_addr, sizeof(peer_addr)) == SOCKET_ERROR_VALUE) {
        CLOSE_SOCKET(sock);
        throw std::runtime_error("Failed to connect to peer");
    }

    // Construct handshake message
    std::string handshake;
    handshake.reserve(68); // Total size of handshake message

    // 1. Protocol string length (1 byte)
    handshake.push_back(19);

    // 2. Protocol string (19 bytes)
    handshake += "BitTorrent protocol";

    // 3. Reserved bytes (8 bytes)
    handshake.append(8, '\0');

    // 4. Info hash (20 bytes)
    handshake.append(info_hash.begin(), info_hash.end());

    // 5. Peer ID (20 bytes)
    std::string peer_id = generate_peer_id();
    handshake += peer_id;

    // Send handshake
    #ifdef _WIN32
        if (send(sock, handshake.c_str(), static_cast<int>(handshake.length()), 0) != handshake.length()) {
    #else
        if (send(sock, handshake.c_str(), handshake.length(), 0) != static_cast<ssize_t>(handshake.length())) {
    #endif
        CLOSE_SOCKET(sock);
        throw std::runtime_error("Failed to send handshake");
    }

    // Receive response
    char response[68];
    #ifdef _WIN32
        int received = recv(sock, response, sizeof(response), 0);
    #else
        ssize_t received = recv(sock, response, sizeof(response), 0);
    #endif

    if (received != 68) {
        CLOSE_SOCKET(sock);
        throw std::runtime_error("Failed to receive complete handshake response");
    }

    CLOSE_SOCKET(sock);

    // Extract peer ID from response (last 20 bytes)
    return std::string(response + 48, 20);
}

void handle_handshake(const std::string& encoded_value, const std::string& peer_ip_port) {
    // Parse torrent file
    parse_torrent(encoded_value);

    // Split peer_ip_port into IP and port
    size_t colon_pos = peer_ip_port.find(':');
    if (colon_pos == std::string::npos) {
        throw std::runtime_error("Invalid peer IP:port format");
    }

    std::string peer_ip = peer_ip_port.substr(0, colon_pos);
    int peer_port = std::stoi(peer_ip_port.substr(colon_pos + 1));

    // Perform handshake and get peer's ID
    std::string received_peer_id = perform_handshake(peer_ip, peer_port, torr.info.hash);

    // Print peer ID in hexadecimal format
    std::cout << "Peer ID: ";
    for (unsigned char c : received_peer_id) {
        printf("%02x", static_cast<unsigned char>(c));
    }
    std::cout << std::endl;
}

#endif