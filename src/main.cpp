#include "Application.h"
#include <iostream>
#include <vector>
#include <cstring>
#include <cstdlib>

void printUsage(const char* progName) {
    std::cout << "Usage: " << progName << " [options] [mesh files...]\n"
              << "\nOptions:\n"
              << "  --angle <degrees>  Crease angle threshold for subdivision (default: 30)\n"
              << "                     Edges with dihedral angle > threshold are kept sharp\n"
              << "  --help             Show this help message\n"
              << "\nControls:\n"
              << "  Left Mouse Drag    Orbit camera\n"
              << "  Middle Mouse Drag  Pan camera\n"
              << "  Right Click        Select object\n"
              << "  Scroll Wheel       Zoom in/out\n"
              << "  S                  Subdivide (Loop - smooth)\n"
              << "  D                  Subdivide (midpoint)\n"
              << "  W                  Toggle wireframe\n"
              << "  C                  Toggle back-face culling\n"
              << "  F                  Focus on scene\n"
              << "  ESC                Exit\n";
}

int main(int argc, char* argv[]) {
    std::vector<std::string> meshPaths;
    float creaseAngle = 30.0f;

    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--angle") == 0 || std::strcmp(argv[i], "-a") == 0) {
            if (i + 1 < argc) {
                creaseAngle = static_cast<float>(std::atof(argv[++i]));
            } else {
                std::cerr << "Error: --angle requires a value\n";
                return 1;
            }
        } else if (std::strcmp(argv[i], "--help") == 0 || std::strcmp(argv[i], "-h") == 0) {
            printUsage(argv[0]);
            return 0;
        } else {
            meshPaths.push_back(argv[i]);
        }
    }

    try {
        Application app(1280, 720, "OpenGL Mesh Viewer", creaseAngle);
        return app.run(meshPaths);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
