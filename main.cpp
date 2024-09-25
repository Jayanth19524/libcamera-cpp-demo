#include <opencv2/opencv.hpp>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include "LibCamera.h" // Ensure to include your LibCamera header
#include <fstream>
#include <vector>
#include <algorithm>
#include <sys/stat.h>

using namespace cv;

// Struct to hold frame data
struct FrameData {
    int frameID;
    int blueCount;
    int greenCount;
    int yellowCount;
    int blackCount;
    int whiteCount;
    int brownCount;
    double bluePercentage;
    double greenPercentage;
    double yellowPercentage;
    double blackPercentage;
    double whitePercentage;
    double brownPercentage;
    time_t timestamp;
    char filename[50];
};

// Function to save frame data to a binary file
void saveFrameData(const std::vector<FrameData>& frames, const std::string& filename) {
    std::ofstream file(filename, std::ios::binary);
    if (file.is_open()) {
        for (const auto& frame : frames) {
            file.write(reinterpret_cast<const char*>(&frame), sizeof(FrameData));
        }
        file.close();
    } else {
        std::cerr << "Error: Unable to open file for writing." << std::endl;
    }
}

// Function to calculate color intensity and percentages
void calculateColorIntensity(const Mat& image, FrameData& data) {
    // Convert to HSV for color analysis
    Mat hsv;
    cvtColor(image, hsv, COLOR_BGR2HSV);

    // Define color ranges in HSV
    int lower_blue[] = {110, 50, 70};
    int upper_blue[] = {130, 255, 255};
    int lower_green[] = {36, 25, 25};
    int upper_green[] = {70, 255, 255};
    int lower_yellow[] = {20, 100, 100};
    int upper_yellow[] = {30, 255, 255};
    int lower_black[] = {0, 0, 0};
    int upper_black[] = {180, 255, 80};
    int lower_white[] = {0, 0, 200};
    int upper_white[] = {180, 30, 255};
    int lower_brown[] = {10, 100, 20};
    int upper_brown[] = {20, 255, 200};

    // Create masks and count colors
    Mat mask_blue, mask_green, mask_yellow, mask_black, mask_white, mask_brown;
    inRange(hsv, Scalar(lower_blue[0], lower_blue[1], lower_blue[2]), Scalar(upper_blue[0], upper_blue[1], upper_blue[2]), mask_blue);
    inRange(hsv, Scalar(lower_green[0], lower_green[1], lower_green[2]), Scalar(upper_green[0], upper_green[1], upper_green[2]), mask_green);
    inRange(hsv, Scalar(lower_yellow[0], lower_yellow[1], lower_yellow[2]), Scalar(upper_yellow[0], upper_yellow[1], upper_yellow[2]), mask_yellow);
    inRange(hsv, Scalar(lower_black[0], lower_black[1], lower_black[2]), Scalar(upper_black[0], upper_black[1], upper_black[2]), mask_black);
    inRange(hsv, Scalar(lower_white[0], lower_white[1], lower_white[2]), Scalar(upper_white[0], upper_white[1], upper_white[2]), mask_white);
    inRange(hsv, Scalar(lower_brown[0], lower_brown[1], lower_brown[2]), Scalar(upper_brown[0], upper_brown[1], upper_brown[2]), mask_brown);

    // Count colors
    data.blueCount = countNonZero(mask_blue);
    data.greenCount = countNonZero(mask_green);
    data.yellowCount = countNonZero(mask_yellow);
    data.blackCount = countNonZero(mask_black);
    data.whiteCount = countNonZero(mask_white);
    data.brownCount = countNonZero(mask_brown);

    // Calculate total pixels in the image
    int totalPixels = image.rows * image.cols;

    // Calculate color percentages
    data.bluePercentage = (static_cast<double>(data.blueCount) / totalPixels) * 100;
    data.greenPercentage = (static_cast<double>(data.greenCount) / totalPixels) * 100;
    data.yellowPercentage = (static_cast<double>(data.yellowCount) / totalPixels) * 100;
    data.blackPercentage = (static_cast<double>(data.blackCount) / totalPixels) * 100;
    data.whitePercentage = (static_cast<double>(data.whiteCount) / totalPixels) * 100;
    data.brownPercentage = (static_cast<double>(data.brownCount) / totalPixels) * 100;
}

// Function to create a directory if it does not exist
void createDirectory(const std::string& dirName) {
    struct stat st;
    if (stat(dirName.c_str(), &st) != 0) {
        mkdir(dirName.c_str(), 0777);
    }
}

// Function to handle day scenario frame selection
std::vector<FrameData> selectDayFrames(const std::vector<FrameData>& frames) {
    std::vector<FrameData> selectedFrames;

    // First priority: Select frames with more than 30% blue pixels
    for (const auto& frame : frames) {
        if (frame.bluePercentage > 30) {
            selectedFrames.push_back(frame);
        }
    }

    // If blue frames are found, also consider green frames for priority
    if (!selectedFrames.empty()) {
        for (const auto& frame : frames) {
            if (frame.greenPercentage > 30 && std::find(selectedFrames.begin(), selectedFrames.end(), frame) == selectedFrames.end()) {
                selectedFrames.push_back(frame);
            }
        }
    }

    // Fallback 1: Select frames with unique colors (high diversity)
    if (selectedFrames.empty()) {
        for (const auto& frame : frames) {
            double uniqueColorPercentage = frame.bluePercentage + frame.greenPercentage + frame.yellowPercentage + frame.brownPercentage;
            if (uniqueColorPercentage > 20) {
                selectedFrames.push_back(frame);
            }
        }
    }

    // Fallback 2: Sort by white percentage if blue percentage is above 30
    if (selectedFrames.empty()) {
        for (const auto& frame : frames) {
            if (frame.bluePercentage > 30) {
                selectedFrames.push_back(frame);
            }
        }
        std::sort(selectedFrames.begin(), selectedFrames.end(), [](const FrameData& a, const FrameData& b) {
            return a.whitePercentage > b.whitePercentage;
        });
    }

    return selectedFrames;
}

// Function to handle night scenario frame selection
std::vector<FrameData> selectNightFrames(const std::vector<FrameData>& frames) {
    std::vector<FrameData> selectedFrames;

    // First priority: Select frames with more than 30% yellow and black pixels
    for (const auto& frame : frames) {
        if (frame.yellowPercentage > 30 || frame.blackPercentage > 30) {
            selectedFrames.push_back(frame);
        }
    }

    // Fallback 1: Select frames with unique colors (high diversity)
    if (selectedFrames.empty()) {
        for (const auto& frame : frames) {
            double uniqueColorPercentage = frame.bluePercentage + frame.greenPercentage + frame.yellowPercentage + frame.brownPercentage;
            if (uniqueColorPercentage > 20) {
                selectedFrames.push_back(frame);
            }
        }
    }

    // Fallback 2: Sort by blue percentage if yellow or black percentage is high
    if (selectedFrames.empty()) {
        for (const auto& frame : frames) {
            if (frame.yellowPercentage > 30 || frame.blackPercentage > 30) {
                selectedFrames.push_back(frame);
            }
        }
        std::sort(selectedFrames.begin(), selectedFrames.end(), [](const FrameData& a, const FrameData& b) {
            return a.bluePercentage > b.bluePercentage;
        });
    }

    return selectedFrames;
}

int main() {
    time_t start_time = time(0);
    int frame_count = 0;
    LibCamera cam;
    uint32_t width = 1920;
    uint32_t height = 1080;
    uint32_t stride;
    char key;
    const int capture_duration = 30;
    const std::string videoFile = "output_video.mp4";
    const std::string binaryFile = "frame_data.bin";
    const std::string dayFolder = "day";
    const std::string nightFolder = "night";
    const std::string tempFolder = "temp";

    // Create necessary directories
    createDirectory(dayFolder);
    createDirectory(nightFolder);
    createDirectory(tempFolder);

    cv::namedWindow("libcamera-demo", cv::WINDOW_NORMAL);
    cv::resizeWindow("libcamera-demo", width, height);

    int ret = cam.initCamera();
    cam.configureStill(width, height, formats::RGB888, 1, 0);
    ControlList controls_;
    int64_t frame_time = 1000000 / 60;
    controls_.set(controls::FrameDurationLimits, libcamera::Span<const int64_t, 2>({ frame_time, frame_time }));

    cam.startCamera();

    std::vector<FrameData> frameDataList;

    while (time(0) - start_time < capture_duration) {
        LibcameraOutData outdata = cam.captureBuffer();
        Mat image(Size(width, height), CV_8UC3, (void*)outdata.imageData);
        Mat outputImage;
        resize(image, outputImage, Size(width, height));

        // Show the output image
        imshow("libcamera-demo", outputImage);

        // Save the frame to a file
        std::string filePath = tempFolder + "/frame_" + std::to_string(frame_count) + ".jpg";
        imwrite(filePath, outputImage);

        // Prepare FrameData and calculate color intensity
        FrameData data;
        data.frameID = frame_count;
        data.timestamp = time(0);
        snprintf(data.filename, sizeof(data.filename), "frame_%d.jpg", frame_count);
        calculateColorIntensity(outputImage, data);

        frameDataList.push_back(data);

        frame_count++;
    }

    cam.stopCamera();

    // Save frame data to binary file
    saveFrameData(frameDataList, binaryFile);

    // Select top frames for day scenario
    std::vector<FrameData> dayFrames = selectDayFrames(frameDataList);
    for (size_t i = 0; i < std::min<size_t>(5, dayFrames.size()); i++) {
        const FrameData& frame = dayFrames[i];
        std::string srcPath = tempFolder + "/" + frame.filename;
        std::string destPath = dayFolder + "/" + frame.filename;
        rename(srcPath.c_str(), destPath.c_str()); // Move the file to the day directory
    }

    // Select top frames for night scenario
    std::vector<FrameData> nightFrames = selectNightFrames(frameDataList);
    for (size_t i = 0; i < std::min<size_t>(5, nightFrames.size()); i++) {
        const FrameData& frame = nightFrames[i];
        std::string srcPath = tempFolder + "/" + frame.filename;
        std::string destPath = nightFolder + "/" + frame.filename;
        rename(srcPath.c_str(), destPath.c_str()); // Move the file to the night directory
    }

    return 0;
}
