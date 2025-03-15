#pragma once

#include <vector>
#include <string>
#include <iostream>
#include <fstream>     
#include <sstream>     
#include <vector>      
#include <string>  
#include <algorithm>
#include <cctype>

struct PointBuffer {
	float x, y, z;
	//float nx, ny, nz;
	//int r, g, b;
};

class PLY_loader {
public:
	std::vector<PointBuffer> load_ply(const std::string& filepath);
private:
	std::vector<PointBuffer> extract_ascii_data(std::ifstream& ply_file);
	//std::vector<PointBuffer> extract_binary_data(std::ifstream& ply_file);
};

