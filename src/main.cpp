#include "App.h"


int main(int argc, char* argv[]) {

    std::string plyFile = "data/custom/no_normals/dog7_final.ply";

    std::cout << __cplusplus << std::endl;

    // start with plyFile from string
    if (argc < 2) {
        App app(1920, 1080, plyFile);               
        app.run();
    }
    // drag and drop
    else {
        std::string plyFile = argv[1];
        App app(1920, 1080, plyFile);      
        app.run();
    }
}

