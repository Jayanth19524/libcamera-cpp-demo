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
    double quality;
    time_t timestamp;
};

double calculateImageQuality(const Mat& image) {
    Mat gray, laplacian;
    cvtColor(image, gray, COLOR_BGR2GRAY);
    Laplacian(gray, laplacian, CV_64F);
    Scalar mean, stddev;
    meanStdDev(laplacian, mean, stddev);
    return stddev[0] * stddev[0];  // Variance of the Laplacian
}

bool isDay(const Mat& image) {
    Mat gray;
    cvtColor(image, gray, COLOR_BGR2GRAY);
    Scalar meanVal = mean(gray);
    return meanVal[0] > 100;  // Adjust threshold based on lighting conditions
}

void updateTopImages(vector<ImageData>& topImages, const Mat& image, double quality, time_t timestamp) {
    if (topImages.size() < 5) {
        topImages.push_back({image, quality, timestamp});
    } else {
        // Replace the lowest quality image if the new one is better
        auto minElement = min_element(topImages.begin(), topImages.end(), 
                                      [](const ImageData& a, const ImageData& b) {
                                          return a.quality < b.quality;
                                      });
        if (quality > minElement->quality) {
            *minElement = {image, quality, timestamp};
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
            double quality = calculateImageQuality(image);

            if (isDay(image)) {
                updateTopImages(topDayImages, image, quality, time(0));
            } else {
                updateTopImages(topNightImages, image, quality, time(0));
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
