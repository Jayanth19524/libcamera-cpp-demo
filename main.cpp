#include <iostream>
#include <fstream>
#include <vector>
#include <ctime>
#include "LibCamera.h"

using namespace std;

struct ImageData {
    vector<unsigned char> image;
    double clarity;
    time_t timestamp;
};

// Function to save raw image data as a BMP file
void saveBmpImage(const string& filename, const unsigned char* data, int width, int height) {
    int fileSize = 54 + 3 * width * height;
    unsigned char bmpFileHeader[14] = {'B', 'M', 0, 0, 0, 0, 54, 0, 0, 0, 40, 0, 0, 0};
    unsigned char bmpInfoHeader[40] = {40, 0, 0, 0, width, 0, 0, 0, height, 0, 0, 0, 1, 0, 24, 0};

    bmpFileHeader[2] = (fileSize & 0xFF);
    bmpFileHeader[3] = ((fileSize >> 8) & 0xFF);
    bmpFileHeader[4] = ((fileSize >> 16) & 0xFF);
    bmpFileHeader[5] = ((fileSize >> 24) & 0xFF);

    bmpInfoHeader[4] = (width & 0xFF);
    bmpInfoHeader[5] = ((width >> 8) & 0xFF);
    bmpInfoHeader[6] = ((width >> 16) & 0xFF);
    bmpInfoHeader[7] = ((width >> 24) & 0xFF);

    bmpInfoHeader[8] = (height & 0xFF);
    bmpInfoHeader[9] = ((height >> 8) & 0xFF);
    bmpInfoHeader[10] = ((height >> 16) & 0xFF);
    bmpInfoHeader[11] = ((height >> 24) & 0xFF);

    ofstream file(filename, ios::binary);
    file.write(reinterpret_cast<char*>(bmpFileHeader), 14);
    file.write(reinterpret_cast<char*>(bmpInfoHeader), 40);
    file.write(reinterpret_cast<const char*>(data), 3 * width * height);
}

// Function to calculate image clarity (dummy implementation)
double calculateImageClarity(const unsigned char* image, int width, int height, bool isDay) {
    // Implement your clarity calculation here
    return 0.0;  // Placeholder for clarity calculation
}

// Function to check if the image is a day or night image (dummy implementation)
bool isDay(const unsigned char* image, int width, int height) {
    // Implement your day/night detection logic here
    return true;  // Placeholder for day/night detection
}

// Function to update top images based on clarity
void updateTopImages(vector<ImageData>& topImages, const unsigned char* image, double clarity, time_t timestamp, int width, int height) {
    if (topImages.size() < 5) {
        topImages.push_back({vector<unsigned char>(image, image + 3 * width * height), clarity, timestamp});
    } else {
        auto minElement = min_element(topImages.begin(), topImages.end(),
                                      [](const ImageData& a, const ImageData& b) {
                                          return a.clarity < b.clarity;
                                      });
        if (clarity > minElement->clarity) {
            *minElement = {vector<unsigned char>(image, image + 3 * width * height), clarity, timestamp};
        }
    }
}

int main() {
    time_t start_time = time(0);
    int frame_count = 0;
    LibCamera cam;
    uint32_t width = 1920;
    uint32_t height = 1080;
    uint32_t stride;
    char key;

    vector<ImageData> topDayImages;
    vector<ImageData> topNightImages;

    cam.initCamera();
    cam.configureStill(width, height, formats::RGB888, 1, 0);
    ControlList controls_;
    int64_t frame_time = 1000000 / 24;  // 24 FPS
    controls_.set(controls::FrameDurationLimits, libcamera::Span<const int64_t, 2>({ frame_time, frame_time }));
    controls_.set(controls::Brightness, 0.5);
    controls_.set(controls::Contrast, 1.5);
    controls_.set(controls::ExposureTime, 20000);
    cam.set(controls_);

    if (!cam.startCamera()) {
        LibcameraOutData frameData;
        cam.VideoStream(&width, &height, &stride);
        time_t current_time = time(0);
        time_t end_time = current_time + 90 * 60;  // 90 minutes

        while (time(0) < end_time) {
            if (!cam.readFrame(&frameData))
                continue;

            // Convert frame data to raw image format
            unsigned char* imageData = frameData.imageData;
            bool day = isDay(imageData, width, height);
            double clarity = calculateImageClarity(imageData, width, height, day);

            if (day) {
                updateTopImages(topDayImages, imageData, clarity, time(0), width, height);
            } else {
                updateTopImages(topNightImages, imageData, clarity, time(0), width, height);
            }

            // Display image
            // Assuming you have a way to display raw image data in your environment
            // e.g., using a library or custom display function

            cam.returnFrameBuffer(frameData);
        }

        cam.stopCamera();
    }

    cam.closeCamera();

    // Save top images after 90 minutes
    for (int i = 0; i < topDayImages.size(); ++i) {
        string filename = "day_image_" + to_string(i) + ".bmp";
        saveBmpImage(filename, topDayImages[i].image.data(), width, height);
    }

    for (int i = 0; i < topNightImages.size(); ++i) {
        string filename = "night_image_" + to_string(i) + ".bmp";
        saveBmpImage(filename, topNightImages[i].image.data(), width, height);
    }

    return 0;
}
