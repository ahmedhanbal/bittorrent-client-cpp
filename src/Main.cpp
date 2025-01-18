// this is the entry point of program


#include "lib/sha1.hpp" // sha1 hash
#include "lib/decode.hpp" // decode and encode string,integer,list,dictionary
#include "lib/utils.hpp" // read files, parse torrent
#include "lib/peers.hpp" // show and discover peers
#include "lib/download.hpp" // download functionality
#include "lib/nlohmann/json.hpp" // json library to efficiently store bencoded content
using json = nlohmann::json;


// help fn, shows commands and usage
void show_help(const std::string& program_name);

// start
int main(int argc, char* argv[]) {
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " command [arguments...]" << std::endl;
        std::cout << "Use '" << argv[0] << " help' for more information" << std::endl;
        return 1;
    }

    std::string command = argv[1];

    try {
        if (command == "decode") {
            if (argc < 3) {
                std::cerr << "Usage: " << argv[0] << " decode <encoded_value>" << std::endl;
                return 1;
            }
            std::string encoded_value = argv[2];
            json decoded_value = decode_bencoded_value(encoded_value);
            std::cout << decoded_value.dump() << std::endl;
        } else if (command == "info") {
            if(argc < 3) {
                std::cerr << "Usage: " << argv[0] << " info <torrent_file>" << std::endl;
                return 1;
            }
            std::string encoded_value = read_file(argv[2]);
            info_torrent(encoded_value);
        } else if (command == "peers") {
            if(argc < 3) {
                std::cerr << "Usage: " << argv[0] << " peers <torrent_file>" << std::endl;
                return 1;
            }
            std::string encoded_value = read_file(argv[2]);
            peers_request(encoded_value);
        } else if (command == "handshake") {
            if (argc < 4) {
                std::cerr << "Usage: " << argv[0] << " handshake <torrent_file> <peer_ip:port>" << std::endl;
                return 1;
            }
            std::string encoded_value = read_file(argv[2]);
            std::string peer_ip_port = argv[3];
            handle_handshake(encoded_value, peer_ip_port);
        } else if (command == "download_piece") {
            if (argc < 5) {
                std::cerr << "Usage: " << argv[0] << " download_piece -o <output_path> <torrent_file> <piece_index>" << std::endl;
                return 1;
            }
            if (std::string(argv[2]) != "-o") {
                std::cerr << "Expected -o option" << std::endl;
                return 1;
            }
            std::string output_path = argv[3];
            std::string encoded_value = read_file(argv[4]);
            int piece_index = std::stoi(argv[5]);
            handle_download_piece(encoded_value, output_path, piece_index);
        } 
        else if (command == "download") {
            if (argc < 5) {
                std::cerr << "Usage: " << argv[0] << " download -o <output_path|default> <torrent_file>" << std::endl;
                return 1;
            }
            if (std::string(argv[2]) != "-o") {
                std::cerr << "Expected -o option" << std::endl;
                return 1;
            }
            std::string output_path = argv[3];
            std::string encoded_value = read_file(argv[4]);
            download_complete_file(encoded_value, output_path);
        } else if (command == "help") {
            show_help(argv[0]);
        } else {
            std::cerr << "Unknown command: " << command << std::endl;
            show_help(argv[0]);
            return 1;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}

void show_help(const std::string& program_name) {
    std::cout << "Usage: " << program_name << " command [arguments...]" << std::endl;
    std::cout << "Commands:" << std::endl;
    std::cout << "  decode <encoded_value>                    Decode a bencoded value" << std::endl;
    std::cout << "  info <torrent_file>                       Show info about a torrent file" << std::endl;
    std::cout << "  peers <torrent_file>                      Show peers from a torrent file" << std::endl;
    std::cout << "  download -o <output_path> <torrent_file>  Download complete file from torrent" << std::endl;
    std::cout << "  download_piece -o <output_path> <torrent_file> <piece_index>" << std::endl;
    std::cout << "  help                                      Show this help message" << std::endl;
}
