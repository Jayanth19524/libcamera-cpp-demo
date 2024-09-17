#include <opencv2/opencv.hpp>
#include <iostream>
#include <chrono>
#include <thread>
#include <cstdlib>

int main() {
    int frameCount = 0;
    int secondsToCapture = 10;  // Capture for 10 seconds
    int fps = 1;  // 1 frame per second
    int totalFrames = secondsToCapture * fps;
    
    std::string filename;

    while (frameCount < totalFrames) {
        // Use libcamera-still to capture a single image
        filename = "frame_" + std::to_string(frameCount) + ".jpg";
        std::string command = "libcamera-still -n -o " + filename;
        system(command.c_str());

        // Wait for the camera to capture the image
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        // Load and display the captured frame using OpenCV
        cv::Mat frame = cv::imread(filename);
        if (frame.empty()) {
            std::cerr << "Error: Could not load the image." << std::endl;
            break;
        }

        cv::imshow("Camera Feed", frame);

        // Wait for 1 second before capturing the next frame
        std::this_thread::sleep_for(std::chrono::seconds(1));

        if (cv::waitKey(1) == 'q') {
            break;
        }

        frameCount++;
    }

    // Clean up and close windows
    cv::destroyAllWindows();

    return 0;
}
