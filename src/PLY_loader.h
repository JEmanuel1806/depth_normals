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

#include "PointCloud.h"


class PLY_loader {
public:

	bool hasNormals = false;

	PointCloud load_ply(const std::string& filepath);
	PointCloud extract_ascii_data(std::ifstream& ply_file, const std::vector<std::string>& property_order);
private:
	//PointCloud extract_binary_data(std::ifstream& ply_file);
};

