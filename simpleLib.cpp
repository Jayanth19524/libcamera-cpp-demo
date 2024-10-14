#include <iostream>
#include <libcamera/libcamera.h>
#include <libcamera/camera.h>
#include <libcamera/camera_manager.h>
#include <libcamera/framebuffer_allocator.h>
#include <libcamera/request.h>
#include <libcamera/stream.h>
#include <libcamera/controls.h>
#include <opencv2/opencv.hpp>
#include <thread>
#include <chrono>

using namespace libcamera;

int main() {
    // Initialize the camera manager
    CameraManager cameraManager;
    cameraManager.start();

    // Get the first available camera
    std::shared_ptr<Camera> camera = cameraManager.cameras()[0];
    if (!camera) {
        std::cerr << "No camera found." << std::endl;
        return -1;
    }

    // Acquire the camera
    if (camera->acquire() < 0) {
        std::cerr << "Failed to acquire camera." << std::endl;
        return -1;
    }

    // Configure the camera for still capture
    std::vector<StreamRole> roles = {StreamRole::StillCapture};
    std::unique_ptr<CameraConfiguration> config = camera->generateConfiguration(roles);

    // Set desired resolution and pixel format
    config->at(0).size = Size(4056, 3040);  // Set desired resolution
    config->at(0).pixelFormat = formats::RGB888;  // Set pixel format
    config->at(0).bufferCount = 4;  // Number of buffers

    // Validate the configuration
    if (camera->configure(config.get()) < 0) {
        std::cerr << "Failed to configure camera." << std::endl;
        return -1;
    }

    // Allocate frame buffers
    FrameBufferAllocator allocator(camera);
    if (allocator.allocate(*config) < 0) {
        std::cerr << "Failed to allocate frame buffers." << std::endl;
        return -1;
    }

    // Create a request to capture frames
    std::unique_ptr<Request> request = camera->createRequest();
    if (!request) {
        std::cerr << "Failed to create request." << std::endl;
        return -1;
    }

    // Attach buffers to the request
    for (const auto &buffer : allocator.buffers()) {
        request->addBuffer(config->at(0).stream(), buffer.get());
    }

    // Start the camera
    if (camera->start() < 0) {
        std::cerr << "Failed to start camera." << std::endl;
        return -1;
    }

    // Capture frames for a specified duration
    for (int i = 0; i < 10; ++i) {
        // Queue the request for processing
        if (camera->queueRequest(request.get()) < 0) {
            std::cerr << "Failed to queue request." << std::endl;
            return -1;
        }

        // Wait for the request to complete
        request->wait();

        // Get the captured buffer
        auto buffers = request->buffers();
        if (!buffers.empty()) {
            FrameBuffer *buffer = buffers[0];

            // Access the frame data
            void *data = buffer->planes()[0].mem.ptr();
            cv::Mat img(config->at(0).size.height, config->at(0).size.width, CV_8UC3, data);

            // Process or display the image
            cv::imshow("Captured Frame", img);
            cv::waitKey(1); // Show for a brief moment
        }

        // Sleep for a short duration before capturing the next frame
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        // Create a new request for the next capture
        request = camera->createRequest();
        if (!request) {
            std::cerr << "Failed to create new request." << std::endl;
            return -1;
        }

        // Reattach buffers to the new request
        for (const auto &buffer : allocator.buffers()) {
            request->addBuffer(config->at(0).stream(), buffer.get());
        }
    }

    // Stop the camera
    camera->stop();
    camera->release();

    return 0;
}
