#include <libcamera/libcamera.h>
#include <stdio.h>

int main(int argc, char *argv[]) {
    libcamera::CameraManager cameraManager;

    // Start the Camera Manager
    cameraManager.start();

    // Get available cameras
    if (cameraManager.cameras().empty()) {
        printf("No cameras available\n");
        return -1;
    }

    // Get the first camera
    std::shared_ptr<libcamera::Camera> camera = cameraManager.get("camera_0");

    if (!camera) {
        printf("Failed to get camera\n");
        return -1;
    }

    // Open the camera
    if (camera->acquire()) {
        printf("Failed to acquire camera\n");
        return -1;
    }

    printf("Camera acquired: %s\n", camera->id().c_str());

    // Here you would configure the camera, request buffers, and capture images...

    // Release the camera
    camera->release();

    // Stop the Camera Manager
    cameraManager.stop();

    return 0;
}
