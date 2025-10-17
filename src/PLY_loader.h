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
	bool m_isMesh = false;

	PointCloud LoadPLY(const std::string& filepath);
	void SavePLY(std::string path, PointCloud pointCloud);

private:
	PointCloud ExtractAsciiData(std::ifstream& ply_file, const std::vector<std::string>& property_order, int vertices, int faces);
	PointCloud ExtractBinaryData(std::ifstream& ply_file, const std::vector<std::string>& property_order,int vertices, int faces);
};

