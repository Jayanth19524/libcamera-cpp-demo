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
    // Initialize camera manager
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
    CameraConfiguration config = camera->generateConfiguration(roles);
    
    // Set desired resolution and pixel format
    config.at(0).size = Size(4056, 3040);  // Set desired resolution
    config.at(0).pixelFormat = formats::RGB888;  // Set pixel format
    config.at(0).bufferCount = 4;  // Number of buffers

    // Validate the configuration
    if (camera->configure(&config) < 0) {
        std::cerr << "Failed to configure camera." << std::endl;
        return -1;
    }

    // Start the camera
    if (camera->start() < 0) {
        std::cerr << "Failed to start camera." << std::endl;
        return -1;
    }

    // Allocate frame buffers
    FrameBufferAllocator allocator(camera.get());
    if (allocator.allocate(config.at(0).stream()) < 0) {
        std::cerr << "Failed to allocate buffers." << std::endl;
        return -1;
    }

    // Create requests
    std::vector<std::unique_ptr<Request>> requests;
    for (const auto &buffer : allocator.buffers(config.at(0).stream())) {
        std::unique_ptr<Request> request = camera->createRequest();
        request->addBuffer(config.at(0).stream(), buffer.get());
        requests.push_back(std::move(request));
    }

    // Capture frames
    for (int i = 0; i < 10; ++i) {
        for (const auto &request : requests) {
            if (camera->queueRequest(request.get()) < 0) {
                std::cerr << "Failed to queue request." << std::endl;
                return -1;
            }
        }

        // Wait for completed requests
        for (const auto &request : requests) {
            camera->requestCompleted.connect(
                [request = request.get()](Request *completedRequest) {
                    if (completedRequest->status() == Request::RequestCancelled) return;

                    const Request::BufferMap &buffers = completedRequest->buffers();
                    for (const auto &bufferPair : buffers) {
                        FrameBuffer *buffer = bufferPair.second;
                        void *data = buffer->planes()[0].mem.ptr();
                        
                        // Create OpenCV Mat from the frame data
                        cv::Mat frame(cv::Size(4056, 3040), CV_8UC3, data);

                        // Display the frame
                        cv::imshow("Frame", frame);
                        cv::waitKey(100); // Display for 100ms
                    }
                }
            );

            // Wait for some time to allow frames to be displayed
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    }

    // Stop and release the camera
    camera->stop();
    camera->release();
    cameraManager.stop();

    return 0;
}
