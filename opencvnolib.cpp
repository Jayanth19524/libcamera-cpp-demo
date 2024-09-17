#include <libcamera/libcamera.h>
#include <libcamera/controls.h>
#include <libcamera/framebuffer_allocator.h>
#include <libcamera/camera_configuration.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <thread>
#include <chrono>
#include <opencv2/opencv.hpp>

using namespace libcamera;
using namespace std;

// Utility function to convert YUV420 to RGB
cv::Mat yuv420ToRgb(const unsigned char *yuv, int width, int height) {
    // Create a cv::Mat for YUV420
    cv::Mat yuvImg(height + height / 2, width, CV_8UC1, (void *)yuv);
    cv::Mat rgbImg;
    // Convert YUV420 to RGB
    cv::cvtColor(yuvImg, rgbImg, cv::COLOR_YUV420p2RGB);
    return rgbImg;
}

class FrameCapture {
public:
    FrameCapture() {
        // Create CameraManager
        cameraManager = CameraManager::create();
        cameraManager->start();
        
        // Get the first camera
        auto cameras = cameraManager->cameras();
        if (cameras.empty()) {
            cerr << "No cameras found" << endl;
            exit(1);
        }
        camera = cameras[0];

        // Initialize the camera
        if (camera->acquire()) {
            cerr << "Failed to acquire camera" << endl;
            exit(1);
        }

        // Configure the camera
        configureCamera();
    }

    ~FrameCapture() {
        camera->release();
        cameraManager->stop();
    }

    void startCapture(int durationSec) {
        int fps = 30;  // Assume 30 frames per second
        int totalFrames = fps * durationSec;
        int frameCount = 0;

        while (frameCount < totalFrames) {
            captureFrame(frameCount);
            frameCount++;
            std::this_thread::sleep_for(std::chrono::milliseconds(1000 / fps));
        }
    }

private:
    std::unique_ptr<CameraManager> cameraManager;
    Camera *camera;
    std::unique_ptr<CameraConfiguration> cameraConfig;
    FrameBufferAllocator allocator;

    void configureCamera() {
        cameraConfig = camera->generateConfiguration({ StreamRole::Viewfinder });
        if (!cameraConfig) {
            cerr << "Failed to generate camera configuration" << endl;
            exit(1);
        }

        auto &streamConfig = cameraConfig->at(0);
        streamConfig.size = { 640, 480 };  // Set resolution to 640x480
        streamConfig.pixelFormat = formats::YUV420;  // Capture in YUV420

        cameraConfig->validate();
        camera->configure(cameraConfig.get());

        allocator.allocate(cameraConfig->streams());
    }

    void captureFrame(int frameCount) {
        camera->start();

        auto buffer = allocator.get(cameraConfig->streams().at(0)->stream()).front();
        auto request = camera->createRequest();
        request->addBuffer(cameraConfig->streams().at(0)->stream(), buffer);
        camera->queueRequest(request);

        // Wait for a frame to be available
        auto completedRequest = camera->waitForRequest();
        if (completedRequest->status() == Request::RequestCompleted) {
            auto yuvBuffer = request->buffers().at(0)->planes()[0].mem();
            auto width = cameraConfig->streams().at(0)->configuration().size.width;
            auto height = cameraConfig->streams().at(0)->configuration().size.height;
            
            // Convert YUV420 to RGB
            cv::Mat rgbFrame = yuv420ToRgb(yuvBuffer, width, height);

            // Save the RGB frame as JPEG
            std::string filename = "frame_" + std::to_string(frameCount) + ".jpg";
            if (!cv::imwrite(filename, rgbFrame)) {
                cerr << "Failed to save frame: " << filename << endl;
            }
        } else {
            cerr << "Failed to capture frame" << endl;
        }

        camera->stop();
    }
};

int main() {
    FrameCapture frameCapture;
    frameCapture.startCapture(30);  // Capture for 30 seconds
    return 0;
}
