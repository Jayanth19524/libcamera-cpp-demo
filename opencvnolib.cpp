#include <libcamera/libcamera.h>
#include <libcamera/controls.h>
#include <libcamera/framebuffer_allocator.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <opencv2/opencv.hpp>

using namespace libcamera;
using namespace std;

// Utility function to convert YUV420 to RGB
cv::Mat yuv420ToRgb(const unsigned char *yuv, int width, int height) {
    cv::Mat yuvImg(height + height / 2, width, CV_8UC1, (void *)yuv);
    cv::Mat rgbImg;
    cv::cvtColor(yuvImg, rgbImg, cv::COLOR_YUV420p2RGB);
    return rgbImg;
}

class FrameCapture {
public:
    FrameCapture() {
        cameraManager = make_unique<CameraManager>();
        cameraManager->start();

        auto cameras = cameraManager->cameras();
        if (cameras.empty()) {
            cerr << "No cameras found" << endl;
            exit(1);
        }
        camera = cameras[0];

        if (camera->acquire()) {
            cerr << "Failed to acquire camera" << endl;
            exit(1);
        }

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
    unique_ptr<CameraManager> cameraManager;
    shared_ptr<Camera> camera;
    unique_ptr<CameraConfiguration> cameraConfig;
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

        allocator = FrameBufferAllocator(camera);
        allocator.allocate(cameraConfig->streams());
    }

    void captureFrame(int frameCount) {
        auto request = camera->createRequest();
        if (!request) {
            cerr << "Failed to create request" << endl;
            return;
        }

        auto buffer = allocator.buffers(cameraConfig->streams().at(0)).front();
        request->addBuffer(cameraConfig->streams().at(0), buffer);
        camera->queueRequest(request);

        camera->start();
        auto completedRequest = camera->waitForRequest();
        camera->stop();

        if (completedRequest->status() == Request::RequestCompleted) {
            auto yuvBuffer = request->buffers().at(0)->planes().at(0).mem();
            auto width = cameraConfig->at(0).size.width;
            auto height = cameraConfig->at(0).size.height;
            
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
    }
};

int main() {
    FrameCapture frameCapture;
    frameCapture.startCapture(30);  // Capture for 30 seconds
    return 0;
}
