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

    // Allocate frame buffers for the selected stream
    Stream *stream = config->at(0).stream();
    FrameBufferAllocator allocator(camera);
    if (allocator.allocate(stream) < 0) {
        std::cerr << "Failed to allocate frame buffers." << std::endl;
        return -1;
    }

    // Start the camera
    if (camera->start() < 0) {
        std::cerr << "Failed to start camera." << std::endl;
        return -1;
    }

    // Capture frames for a specified duration
    for (int i = 0; i < 10; ++i) {
        // Create a new request for capturing a frame
        std::unique_ptr<Request> request = camera->createRequest();
        if (!request) {
            std::cerr << "Failed to create request." << std::endl;
            return -1;
        }

        // Attach buffers to the request
        for (const auto &buffer : allocator.buffers(stream)) {
            request->addBuffer(stream, buffer.get());
        }

        // Queue the request for processing
        if (camera->queueRequest(request.get()) < 0) {
            std::cerr << "Failed to queue request." << std::endl;
            return -1;
        }

        // Wait for the request to complete
        camera->requestComplete.connect([&](Request *completedRequest) {
            if (completedRequest->status() == Request::RequestComplete) {
                // Get the completed buffer
                const FrameBuffer *buffer = completedRequest->buffers().begin()->second;

                // Access the frame data
                const FrameBuffer::Plane &plane = buffer->planes()[0];
                void *data = mmap(nullptr, plane.length, PROT_READ, MAP_SHARED, plane.fd.get(), 0);
                if (data == MAP_FAILED) {
                    std::cerr << "Failed to mmap buffer." << std::endl;
                    return;
                }

                // Create OpenCV Mat from the data
                cv::Mat img(config->at(0).size.height, config->at(0).size.width, CV_8UC3, data);

                // Display the image
                cv::imshow("Captured Frame", img);
                cv::waitKey(1);  // Show for a brief moment

                munmap(data, plane.length);  // Unmap the buffer
            }
        });

        // Sleep for a short duration before capturing the next frame
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    // Stop the camera
    camera->stop();
    camera->release();

    return 0;
}
