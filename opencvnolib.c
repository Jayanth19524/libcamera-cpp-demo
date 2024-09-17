#include <libcamera/libcamera.h>
#include <libcamera/camera_manager.h>
#include <libcamera/framebuffer_allocator.h>
#include <libcamera/controls.h>
#include <libcamera/stream.h>
#include <opencv2/opencv.hpp>
#include <iostream>
#include <thread>
#include <chrono>

using namespace libcamera;
using namespace std;

// C++ wrapper functions
extern "C" {
    // Initialize the camera
    void* initialize_camera() {
        CameraManager* camManager = new CameraManager();
        camManager->start();
        if (camManager->cameras().empty()) {
            std::cerr << "No cameras found!" << std::endl;
            delete camManager;
            return nullptr;
        }

        Camera* camera = camManager->cameras()[0];
        camera->acquire();
        return camManager;
    }

    // Capture a frame and save it as a JPEG file
    void capture_frame(void* cameraManager, const char* filename) {
        CameraManager* camManager = static_cast<CameraManager*>(cameraManager);
        if (!camManager || camManager->cameras().empty()) {
            std::cerr << "Invalid camera manager or no camera available!" << std::endl;
            return;
        }

        Camera* camera = camManager->cameras()[0];
        StreamConfiguration cfg;
        cfg.pixelFormat = formats::YUV420;
        cfg.size = {640, 480};  // Resolution
        cfg.colorSpace = ColorSpace::Rec709;

        // Create and configure the camera
        camera->acquire();
        camera->configure({cfg});
        camera->start();

        // Allocate buffers
        FrameBufferAllocator allocator(camera);
        allocator.allocate(camera->streams()[0]);

        // Capture a frame
        std::this_thread::sleep_for(std::chrono::seconds(1)); // Ensure the camera is initialized
        FrameBuffer* buffer = camera->capture();
        cv::Mat frame(480, 640, CV_8UC3, buffer->map().data());

        // Convert YUV to RGB
        cv::Mat rgbFrame;
        cv::cvtColor(frame, rgbFrame, cv::COLOR_YUV2BGR_I420);

        // Save the frame
        cv::imwrite(filename, rgbFrame);

        // Clean up
        camera->stop();
        camera->release();
    }

    // Release camera and manager resources
    void release_camera(void* cameraManager) {
        CameraManager* camManager = static_cast<CameraManager*>(cameraManager);
        if (camManager) {
            camManager->stop();
            delete camManager;
        }
    }
}

// Main function
int main() {
    void* cameraManager = initialize_camera();
    if (cameraManager == nullptr) {
        std::cerr << "Error: Could not initialize camera" << std::endl;
        return -1;
    }

    capture_frame(cameraManager, "frame.jpg");
    release_camera(cameraManager);

    return 0;
}
