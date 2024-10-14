#include <stdio.h>
#include <libcamera/libcamera.h>  // Use libcamera for capturing frames
#include <opencv2/opencv.hpp>     // Use OpenCV for saving frames

// g++ -o video_capture video_capture.cpp -llibcamera -lopencv_core -lopencv_imgcodecs -lopencv_highgui

using namespace cv;

int main() {
    // Initialize libcamera variables and settings
    libcamera::CameraManager camera_manager;
    libcamera::Camera *camera;
    
    camera_manager.start();
    camera = camera_manager.get("0");  // Get the first camera (Arducam 64MP)
    camera->acquire();
    
    // Configure the camera stream settings
    libcamera::Stream *stream = camera->getStream(libcamera::StreamRole::Viewfinder);
    
    // Create a video stream and capture frames
    camera->configure(stream);
    camera->start();
    
    int frame_count = 0;
    char filename[100];
    
    // Start capturing frames
    while (true) {
        libcamera::Request *request = camera->createRequest();
        if (!request) {
            printf("Failed to create request.\n");
            break;
        }
        
        // Capture the frame (you may need to use the camera's buffer system)
        libcamera::FrameBuffer *buffer = request->getBuffer(stream);
        
        // Convert the buffer data to OpenCV Mat (this depends on the format)
        Mat frame(Size(4056, 3040), CV_8UC3, buffer->data());  // Assuming 64MP resolution

        // Save the frame as an image
        sprintf(filename, "frame_%d.jpg", frame_count);
        imwrite(filename, frame);
        
        printf("Saved frame %d as %s\n", frame_count, filename);
        frame_count++;
        
        // Break the loop or set conditions for how long to capture frames
        if (frame_count >= 100) {  // Example: Capture 100 frames
            break;
        }
    }
    
    // Cleanup
    camera->stop();
    camera->release();
    camera_manager.stop();

    return 0;
}
