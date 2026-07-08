#include <iostream>
#include <chrono>
#include <opencv2/opencv.hpp>
#include "depthai/depthai.hpp"

int main() {
    std::shared_ptr<dai::Device> device;
    try {
        device = std::make_shared<dai::Device>();
    } catch (const std::exception& e) {
        std::cerr << "Failed to connect to device: " << e.what() << std::endl;
        return -1;
    }

    std::cout << "USB Speed: " << device->getUsbSpeed() << std::endl;

    dai::Pipeline pipeline(device);

    // Create Camera node
    auto cam = pipeline.create<dai::node::Camera>()->build(dai::CameraBoardSocket::CAM_A);
    
    // Request raw 4K output to the host
    auto* videoOut = cam->requestOutput({3840, 2160}, dai::ImgFrame::Type::BGR888i);

    pipeline.start();

    auto videoQueue = videoOut->createOutputQueue();

    std::cout << "Streaming LIVE 4K Video. Press 'q' to exit." << std::endl;

    while (pipeline.isRunning()) {
        auto frame = videoQueue->tryGet<dai::ImgFrame>();
        if (frame) {
            // OpenCV can render it, but 4K might be too big for your monitor! 
            // We'll let OpenCV handle the window scaling so it fits on your screen.
            cv::namedWindow("4K Live", cv::WINDOW_NORMAL);
            cv::imshow("4K Live", frame->getCvFrame());
        }

        int key = cv::waitKey(1);
        if (key == 'q') {
            break;
        }
    }

    return 0;
}
