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
	float nx, ny, nz;
	int color[3];
};

class PLY_loader {
public:
	std::vector<PointBuffer> load_ply(const std::string& filepath);
private:
};