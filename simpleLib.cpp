#include <iostream>
#include <libcamera/libcamera.h>
#include <libcamera/camera.h>
#include <libcamera/camera_manager.h>
#include <libcamera/framebuffer_allocator.h>
#include <libcamera/request.h>
#include <libcamera/stream.h>
#include <chrono>
#include <thread>
#include <memory>

using namespace libcamera;

int main() {
    // Initialize the camera manager
    CameraManager *cameraManager = CameraManager::create();
    if (!cameraManager) {
        std::cerr << "Failed to create CameraManager" << std::endl;
        return -1;
    }

    // Start the camera manager
    cameraManager->start();

    // Get the first camera
    std::shared_ptr<Camera> camera = cameraManager->getCamera(0);
    if (!camera) {
        std::cerr << "Failed to get camera" << std::endl;
        return -1;
    }

    // Configure the camera
    CameraConfiguration *config = camera->generateConfiguration({StreamRole::StillCapture});
    if (!config) {
        std::cerr << "Failed to generate configuration" << std::endl;
        return -1;
    }

    // Setup the camera configuration
    config->at(0).pixelFormat = formats::RGB888; // Set pixel format
    config->at(0).size = {1920, 1080}; // Set resolution

    // Validate the configuration
    if (config->validate() != CameraConfiguration::Valid) {
        std::cerr << "Invalid camera configuration" << std::endl;
        return -1;
    }

    // Apply the configuration to the camera
    camera->configure(config);

    // Create a buffer allocator
    FrameBufferAllocator allocator(camera.get());
    for (const auto &stream : config->streams()) {
        if (allocator.allocate(stream) < 0) {
            std::cerr << "Failed to allocate buffers for stream" << std::endl;
            return -1;
        }
    }

    // Create a request for capturing a still image
    std::unique_ptr<Request> request = camera->createRequest();
    if (!request) {
        std::cerr << "Failed to create request" << std::endl;
        return -1;
    }

    // Attach buffers to the request
    for (const auto &stream : config->streams()) {
        const auto &buffers = allocator.buffers(stream);
        request->addBuffer(stream, std::move(buffers[0]));
    }

    // Connect to request completion signal
    camera->requestComplete.connect([&](Request *completedRequest) {
        if (completedRequest->isComplete()) {
            std::cout << "Request completed successfully" << std::endl;
            // Process the buffer here, save the image or perform further actions
        }
        camera->releaseRequest(completedRequest);
    });

    // Queue the request
    if (camera->queueRequest(request.get()) < 0) {
        std::cerr << "Failed to queue request" << std::endl;
        return -1;
    }

    // Start the camera
    camera->start();

    // Wait for the request to complete
    while (!request->isComplete()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Stop the camera
    camera->stop();

    // Cleanup
    delete config;
    cameraManager->stop();
    delete cameraManager;

    return 0;
}
