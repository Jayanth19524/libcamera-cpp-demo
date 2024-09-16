#include "LibCamera.h"
#include <vector>
#include <algorithm>
#include <ctime>
#include <iostream>
#include <fstream>
#include <string>

using namespace std;

struct ImageData {
    uint8_t* imageData;
    double clarity;
    time_t timestamp;
    uint32_t width;
    uint32_t height;
};

// Function to calculate image clarity based on raw RGB values
double calculateImageClarity(uint8_t* imageData, uint32_t width, uint32_t height, bool isDay) {
    double blue_percentage = 0.0, green_percentage = 0.0, brown_percentage = 0.0;
    double black_percentage = 0.0, yellow_percentage = 0.0;

    for (uint32_t i = 0; i < height; i++) {
        for (uint32_t j = 0; j < width; j++) {
            int index = (i * width + j) * 3; // Assuming 3 channels (RGB)
            uint8_t r = imageData[index];
            uint8_t g = imageData[index + 1];
            uint8_t b = imageData[index + 2];

            // Check RGB values for colors
            if (b > 100 && b < 140 && g > 50 && r < 140) {
                blue_percentage += 1.0;
            } else if (g > 35 && g < 85 && r < 85) {
                green_percentage += 1.0;
            } else if (r > 10 && r < 20) {
                brown_percentage += 1.0;
            } else if (r < 50 && g < 50 && b < 50) {
                black_percentage += 1.0;
            } else if (r > 20 && r < 30) {
                yellow_percentage += 1.0;
            }
        }
    }

    double total_pixels = width * height;
    blue_percentage = (blue_percentage / total_pixels) * 100;
    green_percentage = (green_percentage / total_pixels) * 100;
    brown_percentage = (brown_percentage / total_pixels) * 100;
    black_percentage = (black_percentage / total_pixels) * 100;
    yellow_percentage = (yellow_percentage / total_pixels) * 100;

    if (isDay) {
        return blue_percentage + green_percentage + brown_percentage;
    } else {
        return yellow_percentage + black_percentage;
    }
}

// Function to check if the image is a day or night image
bool isDay(uint8_t* imageData, uint32_t width, uint32_t height) {
    double blue_percentage = 0.0, green_percentage = 0.0, brown_percentage = 0.0;
    double black_percentage = 0.0, yellow_percentage = 0.0;

    for (uint32_t i = 0; i < height; i++) {
        for (uint32_t j = 0; j < width; j++) {
            int index = (i * width + j) * 3;
            uint8_t r = imageData[index];
            uint8_t g = imageData[index + 1];
            uint8_t b = imageData[index + 2];

            if (b > 100 && b < 140 && g > 50 && r < 140) {
                blue_percentage += 1.0;
            } else if (g > 35 && g < 85 && r < 85) {
                green_percentage += 1.0;
            } else if (r > 10 && r < 20) {
                brown_percentage += 1.0;
            } else if (r < 50 && g < 50 && b < 50) {
                black_percentage += 1.0;
            } else if (r > 20 && r < 30) {
                yellow_percentage += 1.0;
            }
        }
    }

    double total_pixels = width * height;
    blue_percentage = (blue_percentage / total_pixels) * 100;
    green_percentage = (green_percentage / total_pixels) * 100;
    brown_percentage = (brown_percentage / total_pixels) * 100;
    black_percentage = (black_percentage / total_pixels) * 100;
    yellow_percentage = (yellow_percentage / total_pixels) * 100;

    return (blue_percentage + green_percentage + brown_percentage) > (black_percentage + yellow_percentage);
}

void updateTopImages(vector<ImageData>& topImages, uint8_t* imageData, double clarity, time_t timestamp, uint32_t width, uint32_t height) {
    if (topImages.size() < 5) {
        topImages.push_back({imageData, clarity, timestamp, width, height});
    } else {
        auto minElement = min_element(topImages.begin(), topImages.end(), 
                                      [](const ImageData& a, const ImageData& b) {
                                          return a.clarity < b.clarity;
                                      });
        if (clarity > minElement->clarity) {
            *minElement = {imageData, clarity, timestamp, width, height};
        }
    }
}

// Save the raw image data to a file
void saveImage(const ImageData& imgData, const string& filename) {
    ofstream file(filename, ios::binary);
    file.write(reinterpret_cast<const char*>(imgData.imageData), imgData.width * imgData.height * 3);
    file.close();
}

int main() {
    time_t start_time = time(0);
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

            bool day = isDay(frameData.imageData, width, height);
            double clarity = calculateImageClarity(frameData.imageData, width, height, day);

            if (day) {
                updateTopImages(topDayImages, frameData.imageData, clarity, time(0), width, height);
            } else {
                updateTopImages(topNightImages, frameData.imageData, clarity, time(0), width, height);
            }

            key = getchar();
            if (key == 'q') break;

            cam.returnFrameBuffer(frameData);
        }

        cam.stopCamera();
    }

    cam.closeCamera();

    // Saving top images after 90 minutes
    for (int i = 0; i < topDayImages.size(); ++i) {
        string filename = "day_image_" + to_string(i) + ".raw";
        saveImage(topDayImages[i], filename);
    }

    for (int i = 0; i < topNightImages.size(); ++i) {
        string filename = "night_image_" + to_string(i) + ".raw";
        saveImage(topNightImages[i], filename);
    }

    return 0;
}
