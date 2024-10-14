#include <iostream>
#include <opencv2/opencv.hpp>
#include <thread>
#include <chrono>
#include <vector>

struct FrameData {
    cv::Mat frame;
    int whitePixels; // A metric for how much white is in the frame
};

// Function to calculate the amount of white pixels in a frame
int calculateWhitePixels(const cv::Mat& frame) {
    int whitePixelCount = 0;
    
    for (int y = 0; y < frame.rows; ++y) {
        for (int x = 0; x < frame.cols; ++x) {
            cv::Vec3b pixel = frame.at<cv::Vec3b>(y, x);
            // White is considered if all RGB components are high (near 255)
            if (pixel[0] > 200 && pixel[1] > 200 && pixel[2] > 200) {
                whitePixelCount++;
            }
        }
    }
    
    return whitePixelCount;
}

int main() {
    // Open the camera feed using OpenCV directly from /dev/video0
    cv::VideoCapture cap("/dev/video0");
    if (!cap.isOpened()) {
        std::cerr << "Error: Could not open the camera feed." << std::endl;
        return -1;
    }

    int frameCount = 0;
    int secondsToCapture = 10;  // Capture for 10 seconds
    int fps = 1;  // Capture 1 frame per second
    int totalFrames = secondsToCapture * fps;

    std::vector<FrameData> topFrames(5);  // Store top 5 frames

    cv::Mat frame;
    while (frameCount < totalFrames) {
        // Capture a frame
        cap >> frame;
        if (frame.empty()) {
            std::cerr << "Error: Could not grab a frame." << std::endl;
            break;
        }

        // Analyze the frame for the number of white pixels (clouds)
        int whitePixelCount = calculateWhitePixels(frame);

        // Determine if this frame should be added to the top 5 frames
        int maxWhiteIndex = -1;
        int maxWhiteValue = -1;
        for (int i = 0; i < topFrames.size(); i++) {
            if (topFrames[i].whitePixels > maxWhiteValue) {
                maxWhiteValue = topFrames[i].whitePixels;
                maxWhiteIndex = i;
            }
        }

        // If the current frame has fewer white pixels, replace the worst frame
        if (whitePixelCount < maxWhiteValue || frameCount < 5) {
            if (frameCount >= 5) {
                // We replace the worst frame
                topFrames[maxWhiteIndex] = {frame.clone(), whitePixelCount};
            } else {
                // Initially populate the top 5 list
                topFrames[frameCount] = {frame.clone(), whitePixelCount};
            }
        }

        // Display the current frame
        cv::imshow("Camera Feed", frame);
        
        // Wait for 1 second before capturing the next frame
        std::this_thread::sleep_for(std::chrono::seconds(1));

        if (cv::waitKey(1) == 'q') {
            break;
        }

        frameCount++;
    }

    // Save the top 5 frames to disk
    for (int i = 0; i < topFrames.size(); ++i) {
        if (!topFrames[i].frame.empty()) {
            std::string filename = "best_frame_" + std::to_string(i) + ".jpg";
            cv::imwrite(filename, topFrames[i].frame);
        }
    }

    // Clean up
    cap.release();
    cv::destroyAllWindows();

    return 0;
}
