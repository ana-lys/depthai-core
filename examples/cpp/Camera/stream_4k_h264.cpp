#include <iostream>
#include <cstdio>
#include <csignal>
#include <atomic>
#include <iomanip>
#include "depthai/depthai.hpp"

std::atomic<bool> quitEvent(false);

void printSystemInformation(const dai::SystemInformation& info) {
    const float m = 1024.0f * 1024.0f;  // MiB
    const auto& t = info.chipTemperature;
    std::cout << "\r[Telemetry] " 
              << "CPU CSS: " << std::fixed << std::setprecision(1) << info.leonCssCpuUsage.average * 100.0f << "% | "
              << "MSS: " << info.leonMssCpuUsage.average * 100.0f << "% | "
              << "RAM: " << info.ddrMemoryUsage.used / m << "/" << info.ddrMemoryUsage.total / m << " MiB | "
              << "Temp: " << t.average << "C    " << std::flush;
}

void signalHandler(int signum) {
    quitEvent = true;
}

int main() {
    signal(SIGTERM, signalHandler);
    signal(SIGINT, signalHandler);

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
    
    // Request 4K output for the encoder in NV12 format
    auto* encOut = cam->requestOutput({3840, 2160}, dai::ImgFrame::Type::NV12);

    // Create VideoEncoder node (Back to H.264 to fix ffplay pipe decoding smearing)
    auto videoEnc = pipeline.create<dai::node::VideoEncoder>();
    videoEnc->setDefaultProfilePreset(30, dai::VideoEncoderProperties::Profile::H264_MAIN);
    videoEnc->setBitrateKbps(30000); // 30 Mbps (Very sharp, but decodable)
    videoEnc->setKeyframeFrequency(15); // Send an I-Frame every 0.5 seconds to instantly fix gray screens
    
    encOut->link(videoEnc->input);

    // Create SystemLogger node
    auto sysLog = pipeline.create<dai::node::SystemLogger>();
    sysLog->setRate(1.0f); // 1 Hz update rate

    auto h264Queue = videoEnc->bitstream.createOutputQueue();
    auto sysLogQueue = sysLog->out.createOutputQueue();

    pipeline.start();

    std::cout << "Starting live 4K H.264 stream natively. Spawning ffplay hardware decoder..." << std::endl;
    std::cout << "Press Ctrl+C in this terminal to exit." << std::endl;

    // Spawn an extremely low-latency ffplay instance and stream the H.264 packets directly into it via a pipe
    FILE* ffplayPipe = popen("ffplay -f h264 -framerate 30 -fflags nobuffer -flags low_delay -framedrop -strict experimental -window_title 'Live 4K H.264 Stream' -i - 2>/dev/null", "w");
    
    if (!ffplayPipe) {
        std::cerr << "Failed to open ffplay pipe! Make sure ffmpeg is installed." << std::endl;
        return -1;
    }

    while (pipeline.isRunning() && !quitEvent) {
        // Handle H.264 bitstream
        auto h264Frame = h264Queue->tryGet<dai::ImgFrame>();
        if (h264Frame) {
            fwrite(h264Frame->getData().data(), 1, h264Frame->getData().size(), ffplayPipe);
            fflush(ffplayPipe);
        }

        // Handle System Telemetry (arrives at 1 Hz)
        auto sysInfo = sysLogQueue->tryGet<dai::SystemInformation>();
        if (sysInfo) {
            printSystemInformation(*sysInfo);
        }
    }

    std::cout << "\nShutting down..." << std::endl;
    pclose(ffplayPipe);
    return 0;
}
