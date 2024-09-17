#include <libcamera/libcamera.h>
#include <stdio.h>

int main(int argc, char *argv[]) {
    libcamera::CameraManager cameraManager;

    // Start the Camera Manager
    int ret = cameraManager.start();
    if (ret) {
        printf("Failed to start Camera Manager\n");
        return -1;
    }

    // Check available cameras
    if (cameraManager.cameras().empty()) {
        printf("No cameras available\n");
        return -1;
    }

    // List available cameras
    printf("Available cameras:\n");
    for (const auto &cam : cameraManager.cameras()) {
        printf("Camera ID: %s\n", cam->id().c_str());
    }

    // Get the first camera by using the actual camera ID
    std::shared_ptr<libcamera::Camera> camera = cameraManager.get(cameraManager.cameras()[0]->id());

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
