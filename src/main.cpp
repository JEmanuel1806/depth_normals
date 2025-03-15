#include "Camera.h"
#include "App.h"
#include "Renderer.h"
#include "Shader.h"
#include "PLY_loader.h"


int main() {
	
	PLY_loader ply_loader;
	ply_loader.load_ply("data/table.ply");
	
	//App app(800, 600);
	//app.run();

	return 0;
}

