#include "PLY_loader.h"



PointCloud PLY_loader::load_ply(const std::string& filepath) {
    std::ifstream ply_file(filepath);
    std::string ply_format = "";
    std::vector<std::string> property_order;

    if (!ply_file.is_open()) {
        std::cerr << "Could not open file: " << filepath << std::endl;
        return {};
    }

    std::string line;
    while (std::getline(ply_file, line)) {
        std::istringstream iss(line);
        std::string keyword;
        iss >> keyword;

        if (keyword == "format") {
            std::string format;
            iss >> format;
            ply_format = format;
        }
        else if (keyword == "property") {
            std::string type, name;
            iss >> type >> name;
            property_order.push_back(name);
        }
        else if (keyword == "end_header") {
            break;
        }
    }

    if (ply_format == "ascii") {
        return extract_ascii_data(ply_file, property_order);
    }

    return {};
}

PointCloud PLY_loader::extract_ascii_data(std::ifstream& ply_file, const std::vector<std::string>& property_order) {
    PointCloud cloud;
    int id_counter = 0;
    std::string line;

    while (std::getline(ply_file, line)) {
        std::istringstream iss(line);
        Point point;
        point.ID = id_counter++;

        int r = 255, g = 255, b = 255;

        for (const std::string& prop : property_order) {
            if (prop == "x") iss >> point.position.x;
            else if (prop == "y") iss >> point.position.y;
            else if (prop == "z") iss >> point.position.z;
            else if (prop == "nx") iss >> point.normal.x;
            else if (prop == "ny") iss >> point.normal.y;
            else if (prop == "nz") iss >> point.normal.z;
            else if (prop == "red") iss >> r;
            else if (prop == "green") iss >> g;
            else if (prop == "blue") iss >> b;
        }

        point.color = glm::vec3(r / 255.0f, g / 255.0f, b / 255.0f);
        cloud.addPoint(point);
    }

    std::cerr << "Loaded points: " << cloud.points_amount() << std::endl;

    return cloud;
}