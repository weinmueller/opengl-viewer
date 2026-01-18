#include "Application.h"
#include <iostream>

int main(int argc, char* argv[]) {
    std::string meshPath;

    if (argc > 1) {
        meshPath = argv[1];
    }

    try {
        Application app(1280, 720, "OpenGL Mesh Viewer");
        return app.run(meshPath);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
