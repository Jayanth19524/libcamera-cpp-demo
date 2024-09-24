#include <opencv2/imgproc.hpp>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/videoio.hpp>
#include "LibCamera.h"
#include <fstream>
#include <vector>
#include <algorithm>

using namespace cv;

// Structure to hold frame information
struct FrameData {
    int frameID;
    float blueIntensity;
    float greenIntensity;
    float yellowIntensity;
    float blackIntensity;
    int uniqueColors;
};

// Helper function to calculate pixel intensities
void calculateColorIntensity(const Mat& frame, FrameData& frameData) {
    Mat hsvFrame;
    cvtColor(frame, hsvFrame, COLOR_BGR2HSV);  // Convert from BGR to HSV

    int countBlue = 0, countGreen = 0, countYellow = 0, countBlack = 0;
    std::set<std::tuple<int, int, int>> uniqueColors;

    for (int i = 0; i < hsvFrame.rows; ++i) {
        for (int j = 0; j < hsvFrame.cols; ++j) {
            Vec3b pixel = hsvFrame.at<Vec3b>(i, j);
            int hue = pixel[0];       // Hue value
            int saturation = pixel[1]; // Saturation
            int value = pixel[2];     // Value (Brightness)

            // Count colors based on HSV ranges
            if (hue >= 110 && hue <= 130) { // Blue
                countBlue++;
            } else if (hue >= 36 && hue <= 70) { // Green
                countGreen++;
            } else if (hue >= 20 && hue <= 30) { // Yellow
                countYellow++;
            } else if (value < 50) { // Black
                countBlack++;
            }

            // Store unique colors
            uniqueColors.insert(std::make_tuple(pixel[0], pixel[1], pixel[2]));
        }
    }

    // Normalize the counts to get intensity ratios
    int totalPixels = frame.rows * frame.cols;
    frameData.blueIntensity = static_cast<float>(countBlue) / totalPixels;
    frameData.greenIntensity = static_cast<float>(countGreen) / totalPixels;
    frameData.yellowIntensity = static_cast<float>(countYellow) / totalPixels;
    frameData.blackIntensity = static_cast<float>(countBlack) / totalPixels;
    frameData.uniqueColors = uniqueColors.size();
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

    // Sort by blue intensity
    std::sort(frames.begin(), frames.end(), [](const FrameData& a, const FrameData& b) {
        return a.blueIntensity > b.blueIntensity;
    });

    return frames;
}

// Function to extract frames from the video based on frameID
void extractFramesFromVideo(const std::string& videoFile, const std::vector<FrameData>& frames) {
    VideoCapture cap(videoFile);

    if (!cap.isOpened()) {
        std::cerr << "Error opening video file: " << videoFile << std::endl;
        return;
    }

    for (const auto& frameData : frames) {
        cap.set(CAP_PROP_POS_FRAMES, frameData.frameID);
        Mat frame;
        cap.read(frame);

        if (!frame.empty()) {
            std::string outputFilename = "extracted_frame_" + std::to_string(frameData.frameID) + ".jpg";
            imwrite(outputFilename, frame);
            std::cout << "Extracted frame " << frameData.frameID << " to " << outputFilename << std::endl;
        } else {
            std::cerr << "Failed to extract frame at " << frameData.frameID << std::endl;
        }
    }
}

int main() {
    int frame_count = 0;
    const std::string videoFile = "output_video.mp4";
    const std::string binaryFile = "frame_data.bin";

    LibCamera cam;
    uint32_t width = 1920;
    uint32_t height = 1080;
    uint32_t stride;

    int ret = cam.initCamera();
    cam.configureStill(width, height, formats::RGB888, 1, 0);
    ControlList controls_;
    int64_t frame_time = 1000000 / 30;  // 30 fps
    controls_.set(controls::FrameDurationLimits, libcamera::Span<const int64_t, 2>({ frame_time, frame_time }));
    cam.set(controls_);

    if (!ret) {
        cam.startCamera();
        cam.VideoStream(&width, &height, &stride);

        // Initialize VideoWriter
        cv::VideoWriter videoWriter(videoFile, cv::VideoWriter::fourcc('H', '2', '6', '4'), 30, cv::Size(width, height), true);

        while (frame_count < 300) {  // Capture for 300 frames (approx. 10 seconds at 30 fps)
            LibCameraOutData frameData;
            if (cam.readFrame(&frameData)) {
                Mat im(height, width, CV_8UC3, frameData.imageData, stride);
                imshow("libcamera-demo", im);

                // Write frame to video
                videoWriter.write(im);

                // Record FrameData
                FrameData data;
                data.frameID = frame_count;
                calculateColorIntensity(im, data);
                storeFrameData(data, binaryFile);  // Store frame data in binary file

                frame_count++;
                cam.returnFrameBuffer(frameData);
            }
        }

        destroyAllWindows();
        cam.stopCamera();
        videoWriter.release();
    }

    cam.closeCamera();

    // Read and sort frames based on intensity data from the binary file
    std::vector<FrameData> frames = readAndSortFrames(binaryFile);
    extractFramesFromVideo(videoFile, frames);

    return 0;
}
