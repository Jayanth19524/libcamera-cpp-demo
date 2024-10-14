#include <iostream>
#include <opencv2/opencv.hpp>
#include <thread>
#include <chrono>
#include <vector>

// Start recording video in the background
void startVideoRecording() {
    system("libcamera-vid -t 1800000 --inline --output live_feed.h264 &");
}

// Function to capture frames using OpenCV
void captureFrames() {
    cv::VideoCapture cap("/dev/video0"); // Ensure this matches your camera device
    if (!cap.isOpened()) {
        std::cerr << "Error: Could not open the camera feed." << std::endl;
        return;
    }

    cv::Mat frame;
    while (true) {
        cap >> frame; // Capture a frame
        if (frame.empty()) {
            std::cerr << "Error: Could not grab a frame." << std::endl;
            break;
        }

        // Display the current frame
        cv::imshow("Camera Feed", frame);

        // Logic to save frames if needed
        // For example, save every 10th frame
        static int frameCount = 0;
        if (frameCount % 10 == 0) {
            std::string filename = "frame_" + std::to_string(frameCount) + ".jpg";
            cv::imwrite(filename, frame);
        }
        frameCount++;

        // Break the loop on 'q' key press
        if (cv::waitKey(1) == 'q') {
            break;
        }
    }

    cap.release();
    cv::destroyAllWindows();
}

int main() {
    // Start video recording
    startVideoRecording();

    // Allow a moment for the recording to initialize
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // Start capturing frames
    captureFrames();

    return 0;
}
