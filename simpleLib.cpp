#include <iostream>
#include <libcamera/libcamera.h>
#include <opencv2/opencv.hpp>

using namespace cv;
using namespace std;
using namespace libcamera;

int main() {
    // Initialize the CameraManager
    CameraManager cameraManager;
    cameraManager.start();

    // Get the first available camera
    auto cameras = cameraManager.cameras();
    if (cameras.empty()) {
        cerr << "No cameras found!" << endl;
        return 1;
    }

    shared_ptr<Camera> camera = cameras[0]; // Use the first camera
    camera->acquire();

    // Configure the camera
    CameraConfiguration *config = camera->generateConfiguration({ StreamRole::Viewfinder });
    if (!config) {
        cerr << "Failed to generate camera configuration!" << endl;
        return 1;
    }

    // Set the desired format and resolution
    StreamConfiguration &streamConfig = config->at(0);
    streamConfig.pixelFormat = formats::RGB888; // Use RGB888 format
    streamConfig.size = Size(4056, 3040); // Set resolution to 64MP

    // Apply the configuration
    camera->configure(config);
    camera->start();

    int frameCount = 0;
    char filename[100];

    while (frameCount < 100) { // Capture 100 frames
        unique_ptr<Request> request = camera->createRequest();
        if (!request) {
            cerr << "Failed to create request." << endl;
            break;
        }

        // Queue the request
        camera->queueRequest(request.get());

        // Wait for the request to complete
        if (request->waitForCompletion() != 0) {
            cerr << "Request failed." << endl;
            break;
        }

        // Get the buffer and convert to OpenCV Mat
        FrameBuffer *buffer = request->findBuffer(camera->streams()[0]);
        Mat frame(Size(4056, 3040), CV_8UC3, buffer->data());
        
        // Save the frame as an image
        sprintf(filename, "frame_%d.jpg", frameCount);
        imwrite(filename, frame);
        cout << "Saved frame " << frameCount << " as " << filename << endl;
        frameCount++;
    }

    // Cleanup
    camera->stop();
    camera->release();
    cameraManager.stop();

    return 0;
}
