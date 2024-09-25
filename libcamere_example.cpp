#include <libcamera/libcamera.h>
#include <libcamera/camera.h>
#include <libcamera/framebuffer.h>
#include <libcamera/event.h>
#include <iostream>
#include <vector>

int main() {
    // Step 1: Initialize
    libcamera::CameraManager *cameraManager = new libcamera::CameraManager();
    cameraManager->start();

    // Step 2: Select Camera
    auto cameras = cameraManager->cameras();
    auto camera = cameras[0];

    // Step 3: Configure Camera
    auto config = camera->generateConfiguration({libcamera::StreamType::Raw});
    auto &streamConfig = config->at(0);
    streamConfig.size.width = 640;
    streamConfig.size.height = 480;
    streamConfig.pixelFormat = libcamera::formats::YUV420;

    // Step 4: Allocate Buffers
    camera->configure(config);
    std::vector<std::shared_ptr<libcamera::FrameBuffer>> buffers;
    for (const auto &stream : config->streams()) {
        buffers.push_back(camera->createFrameBuffer(stream));
    }

    // Step 5: Start Streaming
    camera->start();

    // Step 6: Capture Frames
    // (Add your frame capturing logic here)

    // Cleanup
    camera->stop();
    delete cameraManager;
    return 0;
}
