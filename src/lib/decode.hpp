#ifndef DECODE_HPP
#define DECODE_HPP

// this file contains the functions to decode bencoded values

#include <string>
#include "nlohmann/json.hpp"
using json = nlohmann::json;

json decode_bencoded_value(std::string::const_iterator& it, const std::string::const_iterator& end);
json decode_string(std::string::const_iterator& it, const std::string::const_iterator& end);
json decode_integer(std::string::const_iterator& it, const std::string::const_iterator& end);
json decode_list(std::string::const_iterator& it, const std::string::const_iterator& end);
json decode_dict(std::string::const_iterator& it, const std::string::const_iterator& end);

json decode_string(std::string::const_iterator& it, const std::string::const_iterator& end) {
    std::string number_string;
    while (it != end && std::isdigit(*it)) {
        number_string.push_back(*it++);
    }
    if (it == end || *it != ':') {
        throw std::runtime_error("Invalid encoded string");
    }
    ++it; // Skip ':'
    int64_t length = std::stoll(number_string.c_str());
    std::string str(it, it + length);
    it += length;
    return json(str);
}

json decode_integer(std::string::const_iterator& it, const std::string::const_iterator& end) {
    ++it; // Skip 'i'
    std::string integer_string;
    while (it != end && *it != 'e') {
        integer_string.push_back(*it++);
    }
    if (it == end || *it != 'e') {
        throw std::runtime_error("Invalid encoded integer");
    }
    ++it; // Skip 'e'
    int64_t integer = std::stoll(integer_string.c_str());
    return json(integer);
}

json decode_list(std::string::const_iterator& it, const std::string::const_iterator& end) {
    ++it; // Skip 'l'
    json list = json::array();
    while (it != end && *it != 'e') {
        list.push_back(decode_bencoded_value(it, end));
    }
    if (it == end || *it != 'e') {
        throw std::runtime_error("Invalid encoded list");
    }
    ++it; // Skip 'e'
    return list;
}

json decode_dict(std::string::const_iterator& it, const std::string::const_iterator& end) {
    ++it; // Skip 'd'
    json dict = json::object();
    while (it != end && *it != 'e') {
        json key = decode_bencoded_value(it, end);
        json value = decode_bencoded_value(it, end);
        dict[key.get<std::string>()] = value;
    }
    if (it == end || *it != 'e') {
        throw std::runtime_error("Invalid encoded dictionary");
    }
    ++it; // Skip 'e'
    return dict;
}

json decode_bencoded_value(std::string::const_iterator& it, const std::string::const_iterator& end) {
    if (it == end) {
        throw std::runtime_error("Empty encoded value");
    }
    char type = *it;
    if (std::isdigit(type)) {
        return decode_string(it, end);
    } else if (type == 'i') {
        return decode_integer(it, end);
    } else if (type == 'l') {
        return decode_list(it, end);
    } else if (type == 'd') {
        return decode_dict(it, end);
    }
    else {
        throw std::runtime_error("Unhandled encoded value: " + std::string(1, type));
    }
}

json decode_bencoded_value(const std::string& encoded_value) {
    auto it = encoded_value.begin();
    auto end = encoded_value.end();
    return decode_bencoded_value(it, end);
}

std::string bencode_decoded_value(json &decoded_value) {
    std::string encoded_value;
    if (decoded_value.is_string()) {
        std::string str = decoded_value.get<std::string>();
        encoded_value += std::to_string(str.size()) + ":" + str;
    } else if (decoded_value.is_number_integer()) {
        encoded_value += "i" + std::to_string(decoded_value.get<int64_t>()) + "e";
    } else if (decoded_value.is_array()) {
        encoded_value += "l";
        for (const auto& value : decoded_value) {
            json obj = value.object();
            encoded_value += bencode_decoded_value(obj);
        }
        encoded_value += "e";
    } else if (decoded_value.is_object()) {
        encoded_value += "d";
        for (auto it = decoded_value.items().begin(); it != decoded_value.items().end(); ++it) {
            encoded_value += std::to_string(it.key().size()) + ":" + it.key() + bencode_decoded_value(it.value());
        }
        encoded_value += "e";
    } else {
        throw std::runtime_error("Unhandled decoded value");
    }
    return encoded_value;
}

#endif