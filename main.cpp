#include <opencv2/imgproc.hpp>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include "LibCamera.h"
#include <fstream>
#include <ctime>
#include <vector>
#include <algorithm>

using namespace cv;

// Structure to hold frame information
struct FrameData {
    int frameID;
    time_t timestamp;
    float blueIntensity;
    float greenIntensity;
    float brownIntensity;  // Simplified as a mixture of red and green
    float whiteIntensity;
    float blackIntensity;
    float yellowIntensity;
    char filename[50];  // Filename for the saved image
};

// Helper function to calculate pixel intensities
void calculateColorIntensity(const Mat& frame, FrameData& frameData) {
    int blueCount = 0, greenCount = 0, redCount = 0, totalPixels = frame.rows * frame.cols;

    for (int i = 0; i < frame.rows; ++i) {
        for (int j = 0; j < frame.cols; ++j) {
            Vec3b pixel = frame.at<Vec3b>(i, j);
            int blue = pixel[0];
            int green = pixel[1];
            int red = pixel[2];

            blueCount += blue;
            greenCount += green;
            redCount += red;
        }
    }

    frameData.blueIntensity = static_cast<float>(blueCount) / totalPixels;
    frameData.greenIntensity = static_cast<float>(greenCount) / totalPixels;
    frameData.brownIntensity = (static_cast<float>(redCount + greenCount)) / (2 * totalPixels); // Approximation for brown
    frameData.whiteIntensity = static_cast<float>((blueCount + greenCount + redCount) / 3) / totalPixels;
    frameData.blackIntensity = static_cast<float>(totalPixels - (blueCount + greenCount + redCount)) / totalPixels;
    frameData.yellowIntensity = static_cast<float>((redCount + greenCount)) / (2 * totalPixels);
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

    // Sort by blue intensity (change as needed for other color priorities)
    std::sort(frames.begin(), frames.end(), [](const FrameData& a, const FrameData& b) {
        return a.blueIntensity > b.blueIntensity;
    });

    return frames;
}

// Function to load images from filenames
void loadImagesFromTopFrames(const std::vector<FrameData>& frames, int topN = 5) {
    for (int i = 0; i < std::min(topN, static_cast<int>(frames.size())); ++i) {
        Mat img = imread(frames[i].filename);
        if (!img.empty()) {
            printf("Displaying top frame %d - Blue Intensity: %f, Filename: %s\n", frames[i].frameID, frames[i].blueIntensity, frames[i].filename);
            imshow("Top Frame", img);
            waitKey(0);  // Wait for a key press to show the next image
        } else {
            printf("Failed to load image %s\n", frames[i].filename);
        }
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
    const std::string binaryFile = "frame_data.bin";

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

        while (difftime(time(0), start_time) < 60) {  // Run for 60 seconds
            flag = cam.readFrame(&frameData);
            if (!flag)
                continue;

            Mat im(height, width, CV_8UC3, frameData.imageData, stride);
            imshow("libcamera-demo", im);
            key = waitKey(1);
            if (key == 'q') {
                break;
            }

            FrameData data;
            data.frameID = frame_count;
            data.timestamp = time(0);
            snprintf(data.filename, sizeof(data.filename), "frame_%d.jpg", frame_count);

            calculateColorIntensity(im, data);
            storeFrameData(data, binaryFile);  // Store frame data in binary file

            imwrite(data.filename, im);  // Save frame as image

            frame_count++;
            cam.returnFrameBuffer(frameData);
        }

        destroyAllWindows();
        cam.stopCamera();
    }
    cam.closeCamera();

    // Read, sort frames, and load the top 5 images based on blue intensity
    std::vector<FrameData> frames = readAndSortFrames(binaryFile);
    loadImagesFromTopFrames(frames);

    return 0;
}
