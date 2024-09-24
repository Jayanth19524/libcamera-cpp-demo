#include <opencv2/opencv.hpp>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include "LibCamera.h" // Ensure to include your LibCamera header
#include <fstream>
#include <vector>
#include <algorithm>

using namespace cv;

// Struct to hold frame data
struct FrameData {
    int frameID;
    int blueCount;
    int greenCount;
    int yellowCount;
    int blackCount;
    int uniqueColorCount;
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

    // Create a window for displaying the camera feed
    if (width > window_width) {
        cv::namedWindow("libcamera-demo", cv::WINDOW_NORMAL);
        cv::resizeWindow("libcamera-demo", window_width, window_height);
    } 

    int ret = cam.initCamera();
    cam.configureStill(width, height, formats::RGB888, 1, 0);
    ControlList controls_;
    int64_t frame_time = 1000000 / 10;
    controls_.set(controls::FrameDurationLimits, libcamera::Span<const int64_t, 2>({ frame_time, frame_time }));
    controls_.set(controls::Brightness, 0.5);
    controls_.set(controls::Contrast, 1.5);
    controls_.set(controls::ExposureTime, 20000);
    cam.set(controls_);

    if (!ret) {
        std::vector<FrameData> frameDataList;
        const int topFramesCount = 5;

        cam.startCamera();
        cam.VideoStream(&width, &height, &stride);
        while (true) {
            LibcameraOutData frameData;
            bool flag = cam.readFrame(&frameData);
            if (!flag) continue;

            Mat im(height, width, CV_8UC3, frameData.imageData, stride);
            Mat hsv;
            cvtColor(im, hsv, COLOR_BGR2HSV);

            // Define color ranges in HSV
            int lower_blue[] = {110, 50, 70};
            int upper_blue[] = {130, 255, 255};
            int lower_green[] = {36, 25, 25};
            int upper_green[] = {70, 255, 255};
            int lower_yellow[] = {20, 100, 100};
            int upper_yellow[] = {30, 255, 255};
            int black[] = {0, 0, 0};

            // Create masks
            Mat mask_blue, mask_green, mask_yellow, mask_black;
            inRange(hsv, Scalar(lower_blue[0], lower_blue[1], lower_blue[2]), Scalar(upper_blue[0], upper_blue[1], upper_blue[2]), mask_blue);
            inRange(hsv, Scalar(lower_green[0], lower_green[1], lower_green[2]), Scalar(upper_green[0], upper_green[1], upper_green[2]), mask_green);
            inRange(hsv, Scalar(lower_yellow[0], lower_yellow[1], lower_yellow[2]), Scalar(upper_yellow[0], upper_yellow[1], upper_yellow[2]), mask_yellow);
            inRange(hsv, Scalar(black[0], black[1], black[2]), Scalar(black[0], black[1], black[2]), mask_black);

            // Count colors
            int count_blue = countNonZero(mask_blue);
            int count_green = countNonZero(mask_green);
            int count_yellow = countNonZero(mask_yellow);
            int count_black = countNonZero(mask_black);
            int count_unique_colors = static_cast<int>(countNonZero(unique(hsv))); // Count unique colors if necessary

            // Store frame data
            FrameData data = { frame_count, count_blue, count_green, count_yellow, count_black, count_unique_colors };
            frameDataList.push_back(data);

            // Process to keep only top 5 frames based on green count
            if (frameDataList.size() > topFramesCount) {
                std::sort(frameDataList.begin(), frameDataList.end(), [](const FrameData& a, const FrameData& b) {
                    return a.greenCount > b.greenCount; // Change this to your criteria
                });
                frameDataList.resize(topFramesCount); // Keep only top 5
            }

            // Show the frame
            imshow("libcamera-demo", im);
            key = waitKey(1);
            if (key == 'q') break;

            frame_count++;
            if ((time(0) - start_time) >= 1) {
                printf("fps: %d\n", frame_count);
                frame_count = 0;
                start_time = time(0);
            }
            cam.returnFrameBuffer(frameData);
        }

        // Save frame data to a binary file
        saveFrameData(frameDataList, "frame_data.bin");

        destroyAllWindows();
        cam.stopCamera();
    }
    cam.closeCamera();
    return 0;
}
