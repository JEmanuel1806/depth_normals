#include "App.h"


int main(int argc, char* argv[]) {

    std::string plyFile = "data/custom/no_normals/plane.ply";

    if (argc < 2) {
        App app(1920, 1080, plyFile);               
        app.run();
    }
    else {
        std::string plyFile = argv[1];
        App app(1920, 1080, plyFile);      
        app.run();
    }
}

