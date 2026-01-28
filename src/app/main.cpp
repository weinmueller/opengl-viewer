#include "Application.h"
#include <iostream>
#include <vector>
#include <cstring>
#include <cstdlib>

void printUsage(const char* progName) {
    std::cout << "Usage: " << progName << " [options] [mesh files...]\n"
              << "\nOptions:\n"
              << "  --angle <degrees>  Crease angle threshold for subdivision (default: 180)\n"
              << "                     Edges with dihedral angle > threshold are kept sharp\n"
              << "                     Use lower values (e.g., 30) to preserve sharp edges\n"
              << "  --texture <path>   Default texture for all objects (default: assets/textures/default_grid.png)\n"
              << "                     Built-in options: default_grid, checker, uv_test, brushed_metal, wood, concrete\n"
              << "  --help             Show this help message\n"
              << "\nControls:\n"
              << "  Left Mouse Drag    Orbit camera\n"
              << "  Middle Mouse Drag  Pan camera\n"
              << "  Right Click        Select object\n"
              << "  Scroll Wheel       Zoom in/out\n"
              << "  S                  Subdivide (Loop - smooth)\n"
              << "  D                  Subdivide (midpoint)\n"
              << "  W                  Toggle wireframe\n"
              << "  T                  Toggle textures\n"
              << "  C                  Toggle back-face culling\n"
              << "  F                  Focus on scene\n"
              << "  H                  Toggle help overlay\n"
              << "  ESC                Exit\n";
}

int main(int argc, char* argv[]) {
    std::vector<std::string> meshPaths;
    float creaseAngle = 180.0f;
    std::string texturePath = "assets/textures/default_grid.png";

    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--angle") == 0 || std::strcmp(argv[i], "-a") == 0) {
            if (i + 1 < argc) {
                creaseAngle = static_cast<float>(std::atof(argv[++i]));
            } else {
                std::cerr << "Error: --angle requires a value\n";
                return 1;
            }
        } else if (std::strcmp(argv[i], "--texture") == 0 || std::strcmp(argv[i], "-t") == 0) {
            if (i + 1 < argc) {
                std::string arg = argv[++i];
                // Check if it's a built-in texture name (no path separator)
                if (arg.find('/') == std::string::npos && arg.find('\\') == std::string::npos) {
                    // Add .png extension if not present
                    if (arg.find('.') == std::string::npos) {
                        arg += ".png";
                    }
                    texturePath = "assets/textures/" + arg;
                } else {
                    texturePath = arg;
                }
            } else {
                std::cerr << "Error: --texture requires a path\n";
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
        Application app(1280, 720, "OpenGL Mesh Viewer", creaseAngle, texturePath);
        return app.run(meshPaths);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
