#include <opencv2/imgproc.hpp>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <vector>
#include <algorithm>

#include "LibCamera.h"

using namespace cv;

struct FrameInfo {
    Mat image;
    double blueSum;
};

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

    std::vector<FrameInfo> blueFrames; // To store frames and their blue intensities

    if (width > window_width)
    {
        cv::namedWindow("libcamera-demo", cv::WINDOW_NORMAL);
        cv::resizeWindow("libcamera-demo", window_width, window_height);
    }

    int ret = cam.initCamera();
    cam.configureStill(width, height, formats::RGB888, 1, 0);
    ControlList controls_;
    int64_t frame_time = 1000000 / 10;
    // Set frame rate
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
        
        while ((time(0) - start_time) < 30) { // Run camera for 30 seconds
            flag = cam.readFrame(&frameData);
            if (!flag)
                continue;

            Mat im(height, width, CV_8UC3, frameData.imageData, stride);

            // Extract the blue channel and calculate the sum of blue values
            Mat blueChannel;
            std::vector<Mat> channels(3);
            split(im, channels);
            blueChannel = channels[0];
            double blueSum = sum(blueChannel)[0];  // Sum of blue pixels

            // Store the image and its blue sum
            blueFrames.push_back({ im.clone(), blueSum });

            imshow("libcamera-demo", im);
            key = waitKey(1);
            if (key == 'q') {
                break;
            } else if (key == 'f') {
                ControlList controls;
                controls.set(controls::AfMode, controls::AfModeAuto);
                controls.set(controls::AfTrigger, 0);
                cam.set(controls);
            } else if (key == 'a' || key == 'A') {
                lens_position += focus_step;
            } else if (key == 'd' || key == 'D') {
                lens_position -= focus_step;
            }

            if (key == 'a' || key == 'A' || key == 'd' || key == 'D') {
                ControlList controls;
                controls.set(controls::AfMode, controls::AfModeManual);
                controls.set(controls::LensPosition, lens_position);
                cam.set(controls);
            }

            frame_count++;
            if ((time(0) - start_time) >= 1) {
                printf("fps: %d\n", frame_count);
                frame_count = 0;
                start_time = time(0);
            }
            cam.returnFrameBuffer(frameData);
        }

        // Sort frames by blue intensity in descending order
        std::sort(blueFrames.begin(), blueFrames.end(), [](const FrameInfo& a, const FrameInfo& b) {
            return a.blueSum > b.blueSum;
        });

        // Save top 5 frames with the highest blue content as jpg
        for (int i = 0; i < 5 && i < blueFrames.size(); i++) {
            std::string filename = "blue_frame_" + std::to_string(i+1) + ".jpg";
            imwrite(filename, blueFrames[i].image);
            std::cout << "Saved " << filename << " with blue intensity: " << blueFrames[i].blueSum << std::endl;
        }

        destroyAllWindows();
        cam.stopCamera();
    }

    cam.closeCamera();
    return 0;
}
