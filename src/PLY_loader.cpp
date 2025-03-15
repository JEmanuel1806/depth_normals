#include "PLY_loader.h"



std::vector<PointBuffer> PLY_loader::load_ply(const std::string& filepath) {
    std::ifstream ply_file(filepath);
    std::string ply_format = "";

    if (!ply_file.is_open()) {
        std::cerr << "Could not open file: " << filepath << std::endl;
        return {};
    }

    std::string element;

    while (std::getline(ply_file, element)) {
        
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
        return extract_ascii_data(ply_file);
    }
    if (ply_format == "BINARY") {
        //extract_binary_data(ply_file);
        return {};
    }


}

std::vector<PointBuffer> PLY_loader::extract_ascii_data(std::ifstream& ply_file)
{
    std::vector<PointBuffer> points;
    PointBuffer point;

    while (ply_file >> point.x >> point.y >> point.z) {
        points.push_back(point);
    }

    return points;
}