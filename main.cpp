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
    int whiteCount;   // New field for white pixels
    int brownCount;   // New field for brown pixels
    double bluePercentage;    // Percentage of blue pixels
    double greenPercentage;   // Percentage of green pixels
    double yellowPercentage;  // Percentage of yellow pixels
    double blackPercentage;   // Percentage of black pixels
    double whitePercentage;   // Percentage of white pixels
    double brownPercentage;   // Percentage of brown pixels
    time_t timestamp;  // Timestamp for the frame
    char filename[50]; // Filename for saving the image
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
    int lower_white[] = {0, 0, 200};   // New range for white
    int upper_white[] = {180, 30, 255};
    int lower_brown[] = {10, 100, 20}; // New range for brown
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
        mkdir(dirName.c_str(), 0777); // Create directory
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
    const int capture_duration = 30; // Capture for 30 seconds
    const std::string videoFile = "output_video.mp4"; // Output video file
    const std::string binaryFile = "frame_data.bin"; // Binary file for frame data
    const std::string dayFolder = "day"; // Directory for day frames
    const std::string nightFolder = "night"; // Directory for night frames
    const std::string tempFolder = "temp"; // Directory for temp frames

    // Create necessary directories
    createDirectory(dayFolder);
    createDirectory(nightFolder);
    createDirectory(tempFolder); // Create the temp directory

    // Create a window for displaying the camera feed
    cv::namedWindow("libcamera-demo", cv::WINDOW_NORMAL);
    cv::resizeWindow("libcamera-demo", width, height); 

    int ret = cam.initCamera();
    cam.configureStill(width, height, formats::RGB888, 1, 0);
    ControlList controls_;
    int64_t frame_time = 1000000 / 60;
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
        cv::VideoWriter videoWriter(videoFile, cv::VideoWriter::fourcc('H', '2', '6', '4'), 30, cv::Size(width, height), true);
        std::vector<FrameData> frameDataList;

        while (difftime(time(0), start_time) < capture_duration) {  // Run for the defined duration
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
            
            // Calculate color intensities and percentages
            calculateColorIntensity(im, data);
            frameDataList.push_back(data); // Store frame data in a list

            // Save the frame image in the temp directory
            std::string tempFilename = tempFolder + "/" + data.filename; // Use temp folder for filenames
            imwrite(tempFilename, im); // Save the current frame as an image file in temp

            frame_count++;
            cam.returnFrameBuffer(frameData);
        }

        // Save frame data to a binary file
        saveFrameData(frameDataList, binaryFile);

        // Sort frames based on blue intensity
        std::sort(frameDataList.begin(), frameDataList.end(), [](const FrameData& a, const FrameData& b) {
            return a.blueCount > b.blueCount; // Sort in descending order
        });

        // Determine if it's day or night based on black pixels
        const int totalFrames = frameDataList.size();
        int totalBlackCount = 0;

        for (const FrameData& frame : frameDataList) {
            totalBlackCount += frame.blackCount;
        }
        double avgBlackCount = static_cast<double>(totalBlackCount) / totalFrames;

        std::string folderToSave = avgBlackCount > 50000 ? nightFolder : dayFolder;

        // Copy the top 5 frames to the appropriate directory
        for (size_t i = 0; i < std::min<size_t>(5, frameDataList.size()); i++) {
            const FrameData& frame = frameDataList[i];
            std::string srcPath = tempFolder + "/" + frame.filename;
            std::string destPath = folderToSave + "/" + frame.filename;
            rename(srcPath.c_str(), destPath.c_str()); // Move the file to the appropriate directory
        }

        cam.stopCamera();
    }

    return 0;
}
