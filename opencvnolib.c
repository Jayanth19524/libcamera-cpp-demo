#include <opencv2/opencv.hpp>
#include <iostream>
#include <chrono>
#include <thread>

int main() {
    // Open the default camera
    cv::VideoCapture cap(0);  // 0 is the default camera ID, change if needed

    if (!cap.isOpened()) {
        std::cerr << "Error: Could not open the camera." << std::endl;
        return -1;
    }

    int frameCount = 0;
    int secondsToCapture = 10;  // Capture for 10 seconds
    int fps = 1;  // 1 frame per second
    int totalFrames = secondsToCapture * fps;
    
    cv::Mat frame;
    std::string filename;

    while (frameCount < totalFrames) {
        // Capture frame
        cap >> frame;

        if (frame.empty()) {
            std::cerr << "Error: Could not grab a frame." << std::endl;
            break;
        }

        // Save each frame as an image
        filename = "frame_" + std::to_string(frameCount) + ".jpg";
        cv::imwrite(filename, frame);

        // Display the frame
        cv::imshow("Camera Feed", frame);

        // Wait for 1 second before capturing the next frame
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        if (cv::waitKey(1) == 'q') {
            break;
        }

        frameCount++;
    }

    // Clean up and close windows
    cap.release();
    cv::destroyAllWindows();

    return 0;
}
