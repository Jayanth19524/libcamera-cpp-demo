#include <opencv2/imgproc.hpp>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/videoio.hpp>
#include "LibCamera.h"
#include <fstream>
#include <ctime>
#include <vector>
#include <algorithm>
#include <set>
#include <filesystem>  // C++17 for directory management

using namespace cv;

// Structure to hold frame information
struct FrameData {
    int frameID;
    time_t timestamp;
    float blueIntensity;
    float greenIntensity;
    float whiteIntensity;
    int uniqueColors;
    char filename[50];  // Filename for the saved image
};

// Helper function to calculate pixel intensities based on HSV
void calculateColorIntensity(const Mat& frame, FrameData& frameData) {
    Mat hsvFrame;
    cvtColor(frame, hsvFrame, COLOR_BGR2HSV);  // Convert from BGR to HSV

    int blueCount = 0, greenCount = 0, whiteCount = 0;
    int totalPixels = frame.rows * frame.cols;
    std::set<Vec3b, std::less<>> uniqueColors;

    for (int i = 0; i < hsvFrame.rows; ++i) {
        for (int j = 0; j < hsvFrame.cols; ++j) {
            Vec3b pixel = hsvFrame.at<Vec3b>(i, j);
            int hue = pixel[0];
            int saturation = pixel[1];
            int value = pixel[2];

            // Track unique colors (excluding white)
            if (saturation > 50 && value < 240) {
                uniqueColors.insert(pixel);
            }

            // Count blue pixels (approx. hue 100-140) and green pixels (approx. hue 35-85)
            if (hue >= 100 && hue <= 140) {
                blueCount++;
            } else if (hue >= 35 && hue <= 85) {
                greenCount++;
            }

            // Count white pixels (high brightness, low saturation)
            if (saturation < 50 && value > 200) {
                whiteCount++;
            }
        }
    }

    frameData.blueIntensity = static_cast<float>(blueCount) / totalPixels;
    frameData.greenIntensity = static_cast<float>(greenCount) / totalPixels;
    frameData.whiteIntensity = static_cast<float>(whiteCount) / totalPixels;
    frameData.uniqueColors = uniqueColors.size();  // Number of unique colors
}

// Function to store FrameData to a binary file
void storeFrameData(const FrameData& frameData, const std::string& filename) {
    std::ofstream file(filename, std::ios::binary | std::ios::app);
    file.write(reinterpret_cast<const char*>(&frameData), sizeof(FrameData));
    file.close();
}

// Function to read and sort FrameData from the binary file
std::vector<FrameData> readAndSortFrames(const std::string& filename) {
    std::vector<FrameData> frames;
    std::ifstream file(filename, std::ios::binary);

    FrameData temp;
    while (file.read(reinterpret_cast<char*>(&temp), sizeof(FrameData))) {
        frames.push_back(temp);
    }
    file.close();

    return frames;
}

// Function to extract frames from the video based on timestamps
void extractFramesFromVideo(const std::string& videoFile, const std::vector<FrameData>& frames, const std::string& outputDir) {
    VideoCapture cap(videoFile);

    if (!cap.isOpened()) {
        std::cerr << "Error opening video file: " << videoFile << std::endl;
        return;
    }

    double fps = cap.get(CAP_PROP_FPS);
    for (const auto& frameData : frames) {
        double timestampInSeconds = difftime(frameData.timestamp, time(0));
        int frameNumber = static_cast<int>(timestampInSeconds * fps);

        cap.set(CAP_PROP_POS_FRAMES, frameNumber);
        Mat frame;
        cap.read(frame);

        if (!frame.empty()) {
            std::string outputFilename = outputDir + "/extracted_frame_" + std::to_string(frameData.frameID) + ".jpg";
            imwrite(outputFilename, frame);
            std::cout << "Extracted frame " << frameData.frameID << " to " << outputFilename << std::endl;
        } else {
            std::cerr << "Failed to extract frame at " << frameNumber << std::endl;
        }
    }
}

// Function to select the top 5 frames based on the specified criteria
std::vector<FrameData> selectFrames(const std::vector<FrameData>& frames) {
    std::vector<FrameData> blueFrames, greenFrames, uniqueColorFrames;

    for (const auto& frame : frames) {
        if (frame.blueIntensity > 0.3f) {
            blueFrames.push_back(frame);
        } else if (frame.greenIntensity > 0.2f) {
            greenFrames.push_back(frame);
        } else {
            uniqueColorFrames.push_back(frame);
        }
    }

    // If blue frames are available, sort them by white intensity
    if (!blueFrames.empty()) {
        std::sort(blueFrames.begin(), blueFrames.end(), [](const FrameData& a, const FrameData& b) {
            return a.whiteIntensity > b.whiteIntensity;
        });
        return std::vector<FrameData>(blueFrames.begin(), blueFrames.begin() + std::min((size_t)5, blueFrames.size()));
    }

    // If no blue frames, but green frames exist, return them
    if (!greenFrames.empty()) {
        return std::vector<FrameData>(greenFrames.begin(), greenFrames.begin() + std::min((size_t)5, greenFrames.size()));
    }

    // As a fallback, return frames sorted by unique colors
    std::sort(uniqueColorFrames.begin(), uniqueColorFrames.end(), [](const FrameData& a, const FrameData& b) {
        return a.uniqueColors > b.uniqueColors;
    });

    return std::vector<FrameData>(uniqueColorFrames.begin(), uniqueColorFrames.begin() + std::min((size_t)5, uniqueColorFrames.size()));
}

int main() {
    time_t start_time = time(0);
    int frame_count = 0;
    float lens_position = 100;
    float focus_step = 50;
    LibCamera cam;
    uint32_t width = 1920;
    uint32_t height = 1080;
    uint32_t stride;
    char key;
    int window_width = 1920;
    int window_height = 1080;
    const std::string videoFile = "output_video.mp4";
    const std::string binaryFile = "frame_data.bin";

    // Directories for storing results
    const std::string dayOutputDir = "day_frames";
    const std::string nightOutputDir = "night_frames";

    // Create directories if they don't exist
    std::filesystem::create_directory(dayOutputDir);
    std::filesystem::create_directory(nightOutputDir);

    if (width > window_width) {
        cv::namedWindow("libcamera-demo", cv::WINDOW_NORMAL);
        cv::resizeWindow("libcamera-demo", window_width, window_height);
    }

    int ret = cam.initCamera();
    cam.configureStill(width, height, formats::RGB888, 1, 0);
    ControlList controls_;
    int64_t frame_time = 1000000 / 30;  // 30 fps
    controls_.set(controls::FrameDurationLimits, libcamera::Span<const int64_t, 2>({ frame_time, frame_time }));
    controls_.set(controls::Brightness, 0.5);
    controls_.set(controls::Contrast, 1.5);
    controls_.set(controls::ExposureTime, 20000);
    cam.set(controls_);

    if (!ret) {
        bool flag;
        LibcameraOutData frameData;
        cam.startCamera();
        cam.VideoStream(&width, &height, &stride);

        // Initialize VideoWriter
        cv::VideoWriter videoWriter(videoFile, cv::VideoWriter::fourcc('H', '2', '6', '4'), 30, cv::Size(width, height), true);

        while (difftime(time(0), start_time) < 10) {  // Run for 10 seconds
            flag = cam.readFrame(&frameData);
            if (!flag)
                continue;

            Mat im(height, width, CV_8UC3, frameData.imageData, stride);
            imshow("libcamera-demo", im);
            key = waitKey(1);
            if (key == 'q') {
                break;
            }

            // Write frame to video
            videoWriter.write(im);

            // Record FrameData with timestamp
            FrameData data;
            data.frameID = frame_count;
            data.timestamp = time(0);
            snprintf(data.filename, sizeof(data.filename), "frame_%d.jpg", frame_count);

            calculateColorIntensity(im, data);
            storeFrameData(data, binaryFile);  // Store frame data in binary file

            frame_count++;
            cam.returnFrameBuffer(frameData);
        }

        destroyAllWindows();
        cam.stopCamera();
        videoWriter.release();
    }
    cam.closeCamera();

    // Read, sort, and select top 5 frames for day and night
    std::vector<FrameData> frames = readAndSortFrames(binaryFile);
    std::vector<FrameData> selectedDayFrames = selectFrames(frames);  // Same criteria for both day and night in this case

    // Extract frames for Day algorithm
    extractFramesFromVideo(videoFile, selectedDayFrames, dayOutputDir);

    // Since no distinct night algorithm was mentioned, assume same criteria for now
    extractFramesFromVideo(videoFile, selectedDayFrames, nightOutputDir);

    std::cout << "Processing complete. Extracted top 5 frames for Day and Night." << std::endl;

    return 0;
}
