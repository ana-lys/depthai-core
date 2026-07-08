#include <iostream>
#include <memory>
#include "depthai/depthai.hpp"

int main() {
    std::shared_ptr<dai::Device> device;
    try {
        device = std::make_shared<dai::Device>();
    } catch (const std::exception& e) {
        std::cerr << "Failed to connect to device: " << e.what() << std::endl;
        return -1;
    }

    auto cameras = device->getConnectedCameraFeatures();
    
    std::cout << "--- CAMERA FEATURES & RESOLUTIONS ---" << std::endl;
    for (const auto& cam : cameras) {
        std::cout << "Camera: " << cam.name << " (Socket: " << static_cast<int>(cam.socket) << ") | Sensor: " << cam.sensorName << std::endl;
        std::cout << "  Max Resolution: " << cam.width << "x" << cam.height << std::endl;
        std::cout << "  Autofocus: " << (cam.hasAutofocus ? "Yes" : "No") << std::endl;
        std::cout << "  Available Configurations: " << std::endl;
        
        for (const auto& config : cam.configs) {
            std::cout << "    -> " << config.width << "x" << config.height 
                      << " @ [" << config.minFps << " - " << config.maxFps << "] FPS"
                      << " (Type: " << static_cast<int>(config.type) 
                      << ", HDR: " << (config.hdr ? "Yes" : "No") << ")" << std::endl;
        }
        std::cout << "-------------------------------------" << std::endl;
    }

    return 0;
}
