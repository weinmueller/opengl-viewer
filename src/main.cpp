#include "Application.h"
#include <iostream>
#include <vector>

int main(int argc, char* argv[]) {
    std::vector<std::string> meshPaths;

    for (int i = 1; i < argc; ++i) {
        meshPaths.push_back(argv[i]);
    }

    try {
        Application app(1280, 720, "OpenGL Mesh Viewer");
        return app.run(meshPaths);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
