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

	bool m_hasNormals = false;

	PointCloud LoadPLY(const std::string& filepath);
	
private:
	//PointCloud ExtractBinaryData(std::ifstream& ply_file);
	PointCloud ExtractAsciiData(std::ifstream& ply_file, const std::vector<std::string>& property_order, int vertices);
	PointCloud ExtractBinaryData(std::ifstream& ply_file, const std::vector<std::string>& property_order,int vertices);
};

