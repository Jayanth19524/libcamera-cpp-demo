#include <opencv2/imgproc.hpp>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include "LibCamera.h"
#include <vector>
#include <algorithm>
#include <ctime>

using namespace cv;
using namespace std;

struct ImageData {
    Mat image;
    double clarity;
    time_t timestamp;
};

// Function to calculate image clarity based on RGB values
double calculateImageClarity(const Mat& image, bool isDay) {
    Mat hsv;
    cvtColor(image, hsv, COLOR_BGR2HSV);

    Scalar blue_lower(100, 50, 50), blue_upper(140, 255, 255);
    Scalar green_lower(35, 50, 50), green_upper(85, 255, 255);
    Scalar brown_lower(10, 50, 50), brown_upper(20, 255, 255);

    Scalar black_lower(0, 0, 0), black_upper(180, 255, 50);
    Scalar yellow_lower(20, 50, 50), yellow_upper(30, 255, 255);

    Mat blue_mask, green_mask, brown_mask, black_mask, yellow_mask;

    inRange(hsv, blue_lower, blue_upper, blue_mask);
    inRange(hsv, green_lower, green_upper, green_mask);
    inRange(hsv, brown_lower, brown_upper, brown_mask);
    inRange(hsv, black_lower, black_upper, black_mask);
    inRange(hsv, yellow_lower, yellow_upper, yellow_mask);

    double blue_percentage = (countNonZero(blue_mask) / (double)(image.rows * image.cols)) * 100;
    double green_percentage = (countNonZero(green_mask) / (double)(image.rows * image.cols)) * 100;
    double brown_percentage = (countNonZero(brown_mask) / (double)(image.rows * image.cols)) * 100;

    double black_percentage = (countNonZero(black_mask) / (double)(image.rows * image.cols)) * 100;
    double yellow_percentage = (countNonZero(yellow_mask) / (double)(image.rows * image.cols)) * 100;

    // For day: prioritize blue, green, and brown, penalize white (clouds)
    if (isDay) {
        return blue_percentage + green_percentage + brown_percentage;
    }
    // For night: prioritize yellow and black
    else {
        return yellow_percentage + black_percentage;
    }
}

// Function to check if the image is a day or night image
bool isDay(const Mat& image) {
    Mat hsv;
    cvtColor(image, hsv, COLOR_BGR2HSV);

    Scalar blue_lower(100, 50, 50), blue_upper(140, 255, 255);
    Scalar green_lower(35, 50, 50), green_upper(85, 255, 255);
    Scalar brown_lower(10, 50, 50), brown_upper(20, 255, 255);
    Scalar black_lower(0, 0, 0), black_upper(180, 255, 50);
    Scalar yellow_lower(20, 50, 50), yellow_upper(30, 255, 255);

    Mat blue_mask, green_mask, brown_mask, black_mask, yellow_mask;
    inRange(hsv, blue_lower, blue_upper, blue_mask);
    inRange(hsv, green_lower, green_upper, green_mask);
    inRange(hsv, brown_lower, brown_upper, brown_mask);
    inRange(hsv, black_lower, black_upper, black_mask);
    inRange(hsv, yellow_lower, yellow_upper, yellow_mask);

    double blue_percentage = (countNonZero(blue_mask) / (double)(image.rows * image.cols)) * 100;
    double green_percentage = (countNonZero(green_mask) / (double)(image.rows * image.cols)) * 100;
    double brown_percentage = (countNonZero(brown_mask) / (double)(image.rows * image.cols)) * 100;

    double black_percentage = (countNonZero(black_mask) / (double)(image.rows * image.cols)) * 100;
    double yellow_percentage = (countNonZero(yellow_mask) / (double)(image.rows * image.cols)) * 100;

    double day_color_percentage = blue_percentage + green_percentage + brown_percentage;
    double night_color_percentage = black_percentage + yellow_percentage;

    return day_color_percentage > night_color_percentage;
}

void updateTopImages(vector<ImageData>& topImages, const Mat& image, double clarity, time_t timestamp) {
    if (topImages.size() < 5) {
        topImages.push_back({image, clarity, timestamp});
    } else {
        auto minElement = min_element(topImages.begin(), topImages.end(), 
                                      [](const ImageData& a, const ImageData& b) {
                                          return a.clarity < b.clarity;
                                      });
        if (clarity > minElement->clarity) {
            *minElement = {image, clarity, timestamp};
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

            Mat image(height, width, CV_8UC3, frameData.imageData, stride);
            bool day = isDay(image);
            double clarity = calculateImageClarity(image, day);

            if (day) {
                updateTopImages(topDayImages, image, clarity, time(0));
            } else {
                updateTopImages(topNightImages, image, clarity, time(0));
            }

            imshow("libcamera-demo", image);
            key = waitKey(1);
            if (key == 'q') break;

            cam.returnFrameBuffer(frameData);
        }

        cam.stopCamera();
        destroyAllWindows();
    }

    cam.closeCamera();

    // Saving top images after 90 minutes
    for (int i = 0; i < topDayImages.size(); ++i) {
        string filename = "day_image_" + to_string(i) + ".jpg";
        imwrite(filename, topDayImages[i].image);
    }

    for (int i = 0; i < topNightImages.size(); ++i) {
        string filename = "night_image_" + to_string(i) + ".jpg";
        imwrite(filename, topNightImages[i].image);
    }

    return 0;
}
