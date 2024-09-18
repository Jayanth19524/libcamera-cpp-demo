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

// Function to extract frames from the video based on timestamps
void extractFramesFromVideo(const std::string& videoFile, const std::vector<FrameData>& frames) {
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
            std::string outputFilename = "extracted_frame_" + std::to_string(frameData.frameID) + ".jpg";
            imwrite(outputFilename, frame);
            std::cout << "Extracted frame " << frameData.frameID << " to " << outputFilename << std::endl;
        } else {
            std::cerr << "Failed to extract frame at " << frameNumber << std::endl;
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
    const std::string videoFile = "output_video.mp4";
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

        // Initialize VideoWriter
        VideoWriter videoWriter(videoFile, VideoWriter::fourcc('H', '2', '6', '4'), 30, Size(width, height), true);

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

            // Save the frame image as well
            imwrite(data.filename, im);

            frame_count++;
            cam.returnFrameBuffer(frameData);
        }

        destroyAllWindows();
        cam.stopCamera();
        videoWriter.release();
    }
    cam.closeCamera();

    // Read and sort frames based on timestamps and extract them from the video
    std::vector<FrameData> frames = readAndSortFrames(binaryFile);
    extractFramesFromVideo(videoFile, frames);

    return 0;
}
