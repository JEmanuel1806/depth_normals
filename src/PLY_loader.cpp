#include "PLY_loader.h"

/*
 * load_ply
 *
 * Parses a given PLY file and extracts its data into a PointCloud object.
 *
 * Open the file, parse the header and store encountered properties (x,y,z..). Check the format 
 * and extract its content.
 *
 * Binary format is left as a TODO 
 * 
 */

PointCloud PLY_loader::LoadPLY(const std::string& filepath) {
    std::ifstream ply_file(filepath, std::ios::binary); // IMPORTANT: binary mode
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

            // Skip list types or malformed lines
            if (type == "list") {
                std::string list_count_type, list_value_type, list_name;
                iss >> list_count_type >> list_value_type >> list_name;
            }
            else {
                property_order.push_back(name);
            }
        }
        else if (keyword == "end_header") {
            break;
        }
    }

    if (ply_format == "ascii") {
        return ExtractAsciiData(ply_file, property_order);
    }
    else if (ply_format == "binary_little_endian") {
        return ExtractBinaryData(ply_file, property_order);
    }
    else {
        std::cerr << "Unsupported PLY format: " << ply_format << std::endl;
        return {};
    }
}

/*
 * extract_ascii_data
 *
 * Parses the content of an ASCII PLY file and fills a PointCloud object
 *
 *  - Iterates over each line (i.e., each point) and reads values based on property_order
 *  - Assigns a unique ID to each point
 *  - Detects presence of normal attributes (nx, ny, nz) (no calculation for ply with normal data)
 *  - Assigns default color (255,255,255) if none provided
 *  - Adds the point to the cloud
 *
 */
PointCloud PLY_loader::ExtractAsciiData(std::ifstream& ply_file, const std::vector<std::string>& property_order) {
    PointCloud cloud;
    int id_counter = 0;
    std::string line;
    bool has_nx = false, has_ny = false, has_nz = false;

    while (std::getline(ply_file, line)) {
        std::istringstream iss(line);
        Point point;
        point.m_pointID = id_counter++;

        int r = 255, g = 255, b = 255; // fallback color

        // Fill the point with properties of ply file
        for (const std::string& prop : property_order) {
            if (prop == "x") iss >> point.m_position.x;
            else if (prop == "y") iss >> point.m_position.y;
            else if (prop == "z") iss >> point.m_position.z;
            else if (prop == "nx") { iss >> point.m_normal.x; has_nx = true; }
            else if (prop == "ny") { iss >> point.m_normal.y; has_ny = true; }
            else if (prop == "nz") { iss >> point.m_normal.z; has_nz = true; }
            else if (prop == "red") iss >> r;
            else if (prop == "green") iss >> g;
            else if (prop == "blue") iss >> b;
        }

        cloud.m_hasNormals = has_nx && has_ny && has_nz;


        // default color values
        point.m_color = glm::vec3(r / 255.0f, g / 255.0f, b / 255.0f);
        cloud.AddPoint(point);
    }



    std::cerr << "Loaded points: " << cloud.PointsAmount() << std::endl;

    return cloud;
}

PointCloud PLY_loader::ExtractBinaryData(std::ifstream& ply_file, const std::vector<std::string>& property_order) {
    PointCloud cloud;
    int id_counter = 0;
    bool has_nx = false, has_ny = false, has_nz = false;

    while (ply_file.peek() != EOF) {
        Point point;
        point.m_pointID = id_counter++;
        int r = 255, g = 255, b = 255;

        for (const std::string& prop : property_order) {
            if (prop == "x") {
                float x; ply_file.read(reinterpret_cast<char*>(&x), sizeof(float));
                point.m_position.x = x;
            }
            else if (prop == "y") {
                float y; ply_file.read(reinterpret_cast<char*>(&y), sizeof(float));
                point.m_position.y = y;
            }
            else if (prop == "z") {
                float z; ply_file.read(reinterpret_cast<char*>(&z), sizeof(float));
                point.m_position.z = z;
            }
            else if (prop == "nx") {
                float nx; ply_file.read(reinterpret_cast<char*>(&nx), sizeof(float));
                point.m_normal.x = nx; has_nx = true;
            }
            else if (prop == "ny") {
                float ny; ply_file.read(reinterpret_cast<char*>(&ny), sizeof(float));
                point.m_normal.y = ny; has_ny = true;
            }
            else if (prop == "nz") {
                float nz; ply_file.read(reinterpret_cast<char*>(&nz), sizeof(float));
                point.m_normal.z = nz; has_nz = true;
            }
            else if (prop == "red") {
                uint8_t c; ply_file.read(reinterpret_cast<char*>(&c), 1); r = c;
            }
            else if (prop == "green") {
                uint8_t c; ply_file.read(reinterpret_cast<char*>(&c), 1); g = c;
            }
            else if (prop == "blue") {
                uint8_t c; ply_file.read(reinterpret_cast<char*>(&c), 1); b = c;
            }
            else {
                // Skip unknown property by type size if needed, or just ignore
                std::cerr << "Unknown property in binary PLY: " << prop << std::endl;
            }
        }

        point.m_color = glm::vec3(r / 255.0f, g / 255.0f, b / 255.0f);
        cloud.AddPoint(point);
    }

    cloud.m_hasNormals = has_nx && has_ny && has_nz;
    std::cerr << "Loaded (binary) points: " << cloud.PointsAmount() << std::endl;
    return cloud;
}

