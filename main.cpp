#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <opencv2/opencv.hpp> // OpenCV header for preview
#include <libcamera/libcamera.h>
#include <libcamera/camera_manager.h>
#include <libcamera/camera.h>
#include <libcamera/framebuffer.h>
#include <libcamera/request.h>
#include <libcamera/controls.h>
#include <libcamera/property_ids.h>
#include <libcamera/control_ids.h>

int main(int argc, char **argv) {
    libcamera::CameraManager *cameraManager = new libcamera::CameraManager();

    // Start the camera manager
    cameraManager->start();

    // List all available cameras
    const std::vector<std::shared_ptr<libcamera::Camera>> &cameras = cameraManager->cameras();
    if (cameras.empty()) {
        printf("No cameras available\n");
        return -1;
    }

    // Open the first camera (Arducam 64MP)
    std::shared_ptr<libcamera::Camera> camera = cameras[0];
    if (!camera) {
        printf("Failed to open camera\n");
        return -1;
    }
    camera->acquire();

    // Configure camera with autofocus and capture settings
    std::unique_ptr<libcamera::CameraConfiguration> config = camera->generateConfiguration({libcamera::StreamRole::VideoRecording});

    if (config->size() != 1) {
        printf("Failed to generate camera configuration\n");
        return -1;
    }

    // Set desired resolution (adjust to match the 64MP camera's capability)
    config->at(0).pixelFormat = libcamera::formats::YUV420;
    config->at(0).size = libcamera::Size(1920, 1080);  // Full HD
    config->at(0).bufferCount = 4;

    // Apply the configuration
    libcamera::CameraConfiguration::Status status = config->validate();
    if (status == libcamera::CameraConfiguration::Invalid) {
        printf("Camera configuration is invalid\n");
        return -1;
    }
    camera->configure(config.get());

    // Setup autofocus control
    libcamera::ControlList controls(camera->controls());
    controls.set(libcamera::controls::AfMode, libcamera::controls::AfModeContinuous);
    
    // Create and queue framebuffers for capture
    for (unsigned int i = 0; i < config->at(0).bufferCount; i++) {
        std::unique_ptr<libcamera::FrameBuffer> buffer = std::make_unique<libcamera::FrameBuffer>();
        camera->request()->addBuffer(buffer.get());
    }

    // Start capturing
    camera->start(&controls);

    printf("Camera is capturing live feed with preview for 30 seconds...\n");

    // Capture for 30 seconds
    time_t startTime = time(NULL);
    time_t currentTime;

    // OpenCV window setup for preview
    cv::namedWindow("Camera Preview", cv::WINDOW_AUTOSIZE);

    // Capture loop for 30 seconds
    while (1) {
        currentTime = time(NULL);
        if (difftime(currentTime, startTime) >= 30) {
            break;  // Exit the loop after 30 seconds
        }

        libcamera::Request *request = camera->createRequest();
        if (!request) {
            printf("Failed to create request\n");
            return -1;
        }

        // Queue the request
        camera->queueRequest(request);
        camera->processRequestCompleted();

        // Access the framebuffer (for simplicity assuming YUV format)
        libcamera::FrameBuffer *buffer = request->buffers().begin()->second;

        // Convert frame buffer to OpenCV format (YUV to BGR conversion for display)
        cv::Mat yuvImage(1080, 1920, CV_8UC1, buffer->map().data());  // Modify to match actual format
        cv::Mat bgrImage;
        cv::cvtColor(yuvImage, bgrImage, cv::COLOR_YUV2BGR_I420);  // YUV to BGR conversion
        
        // Display the image
        cv::imshow("Camera Preview", bgrImage);

        // Wait for a key press or 30ms delay to display frame at 33fps
        if (cv::waitKey(30) >= 0) break;
    }

    // Close OpenCV window
    cv::destroyWindow("Camera Preview");

    // Stop capturing
    camera->stop();
    camera->release();
    cameraManager->stop();

    delete cameraManager;

    printf("Camera stopped after 30 seconds.\n");

    return 0;
}
