#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <opencv2/opencv.hpp>
#include <libcamera/libcamera.h>
#include <libcamera/camera_manager.h>

int main(int argc, char **argv) {
    libcamera::CameraManager *cameraManager = new libcamera::CameraManager();
    cameraManager->start();

    const auto &cameras = cameraManager->cameras();
    if (cameras.empty()) {
        printf("No cameras available\n");
        return -1;
    }

    std::shared_ptr<libcamera::Camera> camera = cameras[0];
    camera->acquire();

    auto config = camera->generateConfiguration({libcamera::StreamRole::VideoRecording});
    if (config->size() != 1) {
        printf("Failed to generate camera configuration\n");
        return -1;
    }

    config->at(0).pixelFormat = libcamera::formats::YUV420;
    config->at(0).size = libcamera::Size(1920, 1080);
    config->at(0).bufferCount = 4;

    if (config->validate() == libcamera::CameraConfiguration::Invalid) {
        printf("Camera configuration is invalid\n");
        return -1;
    }
    camera->configure(config.get());

    libcamera::ControlList controls;
    controls.set(libcamera::controls::AfMode, libcamera::controls::AfModeContinuous);

    // Create a request
    libcamera::Request *request = camera->createRequest();
    if (!request) {
        printf("Failed to create request\n");
        return -1;
    }

    // Create and add buffers to the request
    for (unsigned int i = 0; i < config->at(0).bufferCount; i++) {
        std::shared_ptr<libcamera::FrameBuffer> buffer = request->addBuffer(config->at(0), nullptr);
        if (!buffer) {
            printf("Failed to create buffer\n");
            return -1;
        }
    }

    camera->start(&controls);

    printf("Camera is capturing live feed with preview for 30 seconds...\n");
    time_t startTime = time(NULL);
    cv::namedWindow("Camera Preview", cv::WINDOW_AUTOSIZE);

    // Capture loop for 30 seconds
    while (difftime(time(NULL), startTime) < 30) {
        // Queue the request
        camera->queueRequest(request);
        camera->processRequestCompleted();

        // Access the framebuffer
        const auto &buffers = request->buffers();
        auto it = buffers.begin();
        if (it != buffers.end()) {
            libcamera::FrameBuffer *buffer = it->second.get();
            // Access the mapped buffer data correctly
            auto mappedBuffer = buffer->map();
            if (!mappedBuffer.isValid()) {
                printf("Buffer mapping failed\n");
                continue;
            }

            cv::Mat yuvImage(1080, 1920, CV_8UC1, mappedBuffer.data());
            cv::Mat bgrImage;
            cv::cvtColor(yuvImage, bgrImage, cv::COLOR_YUV2BGR_I420);

            cv::imshow("Camera Preview", bgrImage);
            if (cv::waitKey(30) >= 0) break; // Allow for exit
        }
    }

    // Cleanup
    camera->stop();
    camera->release();
    cameraManager->stop();
    delete cameraManager;

    cv::destroyWindow("Camera Preview");
    printf("Camera stopped after 30 seconds.\n");

    return 0;
}
