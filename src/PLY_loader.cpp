#include "PLY_loader.h"

std::string ply_format = "";

std::vector<PointBuffer> PLY_loader::load_ply(const std::string& filepath) {
    std::ifstream ply_file(filepath);

    if (!ply_file.is_open()) {
        std::cerr << "Could not open file: " << filepath << std::endl;
        return {};
    }

    std::string element;

    while (std::getline(ply_file, element)) {
        
        /*
        // Get rid of whitespaces for getline()

        // Leading
        size_t start = element.find_first_not_of(" \t\n\r");
        if (start != std::string::npos) {
            element = element.substr(start); 
        }

        //Trailing
        size_t end = element.find_last_not_of(" \t\n\r");
        if (end != std::string::npos) {
            element = element.substr(0, end + 1); 
        }
        */

        std::istringstream iss(element);
        std::string key;
        iss >> key;

        if (key == "format") {
            std::string format;
            iss >> format;
            if (format == "ascii") {
                std::cout << "FORMAT: ASCII" << std::endl;
                ply_format = "ASCII";
            }
            else {
                std::cout << "FORMAT: BINARY" << std::endl;
                ply_format = "BINARY";
            }
        }
        if (element == "end_header") {
            break;
        }

    }

    if (ply_format == "ASCII") {
        extract_ascii_data();
    }
    if (ply_format == "BINARY") {
        extract_binary_data();
    }

    return {};
}

void extract_ascii_data() {
    std::vector<PointBuffer> points;

}

void extract_binary_data() {

}