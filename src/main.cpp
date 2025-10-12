#include "App.h"
#include <iostream>
#include <filesystem>

int main(int argc, char* argv[]) {
    std::string plyFile;


    // Debug mode
    if (argc < 2) {
        plyFile = "data/custom/no_normals/horse7_final.ply";
    }
    // Drag and Drop
    else {
        plyFile = argv[1];
    }

    if (!std::filesystem::exists(plyFile)) {
        std::cerr << "Error: File not found: " << plyFile << std::endl;
        return 1;
    }

    App app(1920, 1080, plyFile);
    app.run();
    return 0;
}