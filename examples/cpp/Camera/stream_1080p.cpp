#include <atomic>
#include <csignal>
#include <iostream>
#include <memory>
#include <chrono>
#include <cmath>

#include "depthai/depthai.hpp"

std::atomic<bool> quitEvent(false);
void signalHandler(int) { quitEvent = true; }

class Stats {
public:
    void add(double val) {
        sum += val;
        sq_sum += val * val;
        count++;
    }
    double mean() const { return count == 0 ? 0 : sum / count; }
    double sigma() const {
        if (count == 0) return 0;
        double m = mean();
        double variance = (sq_sum / count) - (m * m);
        return variance > 0 ? std::sqrt(variance) : 0.0;
    }
    void reset() { sum = 0; sq_sum = 0; count = 0; }
private:
    double sum = 0;
    double sq_sum = 0;
    size_t count = 0;
};

int main() {
    signal(SIGTERM, signalHandler);
    signal(SIGINT, signalHandler);

    std::cout << "Initializing Device..." << std::endl;
    std::shared_ptr<dai::Device> device;
    try {
        device = std::make_shared<dai::Device>();
    } catch (const std::exception& e) {
        std::cerr << "Failed to connect to device: " << e.what() << std::endl;
        return -1;
    }

    std::cout << "Connected cameras: ";
    for (const auto& cam : device->getConnectedCameraFeatures()) {
        std::cout << cam.name << " (" << static_cast<int>(cam.socket) << ") ";
    }
    std::cout << std::endl;

    std::cout << "USB Speed: " << device->getUsbSpeed() << std::endl;

    dai::Pipeline pipeline(device);
    
    // Auto-discover the ColorCamera socket. Usually CAM_A.
    auto cam = pipeline.create<dai::node::Camera>()->build(dai::CameraBoardSocket::CAM_A);
    
    // Limit exposure to 10,000 microseconds (10ms) to drastically reduce motion blur.
    // Note: The image will become darker, especially in a dark room.
    cam->initialControl.setAutoExposureLimit(10000);
    
    // Request 1080p output
    auto videoQueue = cam->requestOutput(std::make_pair(1920, 1080))->createOutputQueue();

    pipeline.start();
    std::cout << "Pipeline started. Press 'q' on the video window or Ctrl+C to exit." << std::endl;

    auto t0 = std::chrono::steady_clock::now();
    int frameCount = 0;
    Stats delayStats;

    while(pipeline.isRunning() && !quitEvent) {
        auto videoIn = videoQueue->get<dai::ImgFrame>();
        if(videoIn == nullptr) continue;

        auto now = std::chrono::steady_clock::now();
        
        // Calculate latency using host-mapped capture timestamp
        auto delay_ms = std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(now - videoIn->getTimestamp()).count();
        
        delayStats.add(delay_ms);
        frameCount++;

        auto elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(now - t0).count();
        if (elapsed >= 1.0) {
            double fps = frameCount / elapsed;
            std::cout << "FPS: " << fps 
                      << " | Delay Mean: " << delayStats.mean() << " ms"
                      << " | Delay Sigma: " << delayStats.sigma() << " ms" << std::endl;
            
            // Reset for the next interval
            frameCount = 0;
            t0 = now;
            delayStats.reset();
        }

        cv::Mat frame = videoIn->getCvFrame();
        cv::Mat displayFrame;
        // Resize for display purposes so it fits on standard screens
        cv::resize(frame, displayFrame, cv::Size(960, 540));
        cv::imshow("1080p RGB", displayFrame);

        int key = cv::waitKey(1);
        if(key == 'q' || key == 27) {
            break;
        }
    }

    pipeline.stop();
    pipeline.wait();

    return 0;
}
