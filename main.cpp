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
    int brownCount;
    int blackCount;
    int whiteCount;
    double bluePercentage;
    double greenPercentage;
    double yellowPercentage;
    double brownPercentage;
    double blackPercentage;
    double whitePercentage;
    time_t timestamp;
    char filename[50];
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
    Mat hsv;
    cvtColor(image, hsv, COLOR_BGR2HSV);

    // Define color ranges in HSV
    int lower_blue[] = {110, 50, 70};
    int upper_blue[] = {130, 255, 255};
    int lower_green[] = {36, 25, 25};
    int upper_green[] = {70, 255, 255};
    int lower_yellow[] = {20, 100, 100};
    int upper_yellow[] = {30, 255, 255};
    int lower_brown[] = {10, 100, 20}; // Added brown range
    int upper_brown[] = {20, 255, 200}; // Added brown range
    int lower_black[] = {0, 0, 0};
    int upper_black[] = {180, 255, 80};
    int lower_white[] = {0, 0, 200}; // Added white range
    int upper_white[] = {180, 20, 255}; // Added white range

    // Create masks and count colors
    Mat mask_blue, mask_green, mask_yellow, mask_brown, mask_black, mask_white;
    inRange(hsv, Scalar(lower_blue[0], lower_blue[1], lower_blue[2]), Scalar(upper_blue[0], upper_blue[1], upper_blue[2]), mask_blue);
    inRange(hsv, Scalar(lower_green[0], lower_green[1], lower_green[2]), Scalar(upper_green[0], upper_green[1], upper_green[2]), mask_green);
    inRange(hsv, Scalar(lower_yellow[0], lower_yellow[1], lower_yellow[2]), Scalar(upper_yellow[0], upper_yellow[1], upper_yellow[2]), mask_yellow);
    inRange(hsv, Scalar(lower_brown[0], lower_brown[1], lower_brown[2]), Scalar(upper_brown[0], upper_brown[1], upper_brown[2]), mask_brown);
    inRange(hsv, Scalar(lower_black[0], lower_black[1], lower_black[2]), Scalar(upper_black[0], upper_black[1], upper_black[2]), mask_black);
    inRange(hsv, Scalar(lower_white[0], lower_white[1], lower_white[2]), Scalar(upper_white[0], upper_white[1], upper_white[2]), mask_white);

    // Count colors
    data.blueCount = countNonZero(mask_blue);
    data.greenCount = countNonZero(mask_green);
    data.yellowCount = countNonZero(mask_yellow);
    data.brownCount = countNonZero(mask_brown);
    data.blackCount = countNonZero(mask_black);
    data.whiteCount = countNonZero(mask_white);

    // Calculate total pixels in the image
    int totalPixels = image.rows * image.cols;

    // Calculate color percentages
    data.bluePercentage = (static_cast<double>(data.blueCount) / totalPixels) * 100;
    data.greenPercentage = (static_cast<double>(data.greenCount) / totalPixels) * 100;
    data.yellowPercentage = (static_cast<double>(data.yellowCount) / totalPixels) * 100;
    data.brownPercentage = (static_cast<double>(data.brownCount) / totalPixels) * 100;
    data.blackPercentage = (static_cast<double>(data.blackCount) / totalPixels) * 100;
    data.whitePercentage = (static_cast<double>(data.whiteCount) / totalPixels) * 100;
}

// Function to save the frames based on day criteria
void selectDayFrames(std::vector<FrameData>& frameDataList, std::vector<FrameData>& selectedDayFrames, const std::string& dayFolder) {
    // First pass: Find frames with more than 30% blue pixels
    for (const auto& frame : frameDataList) {
        if (frame.bluePercentage > 30) {
            selectedDayFrames.push_back(frame);
        }
    }

    // If we found blue frames, sort them by green percentage in descending order
    if (!selectedDayFrames.empty()) {
        std::sort(selectedDayFrames.begin(), selectedDayFrames.end(),
                  [](const FrameData& a, const FrameData& b) {
                      return a.greenPercentage > b.greenPercentage; // Sort by green percentage
                  });

        // Save selected frames to the day folder
        for (const auto& frame : selectedDayFrames) {
            Mat savedImage = imread(frame.filename); // Read the temporarily saved frame
            std::string dayImagePath = dayFolder + "/frame_" + std::to_string(frame.frameID) + ".jpg";
            imwrite(dayImagePath, savedImage); // Save the image to the day folder
        }
    } else {
        // If no frames meet the criteria, check for unique colors
        for (const auto& frame : frameDataList) {
            if (frame.blueCount + frame.greenCount + frame.yellowCount + frame.brownCount + frame.blackCount > 0) {
                selectedDayFrames.push_back(frame);
            }
        }

        // If still no frames, fallback to blue and white arrangement
        if (selectedDayFrames.empty()) {
            for (const auto& frame : frameDataList) {
                if (frame.bluePercentage > 30) {
                    selectedDayFrames.push_back(frame);
                }
            }

            // Sort selected frames by white percentage in descending order
            std::sort(selectedDayFrames.begin(), selectedDayFrames.end(),
                      [](const FrameData& a, const FrameData& b) {
                          return a.whitePercentage > b.whitePercentage;
                      });
        }
    }
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
    char key;
    const int capture_duration = 30; // Capture for 30 seconds
    const std::string binaryFile = "frame_data.bin"; // Binary file for frame data
    const std::string dayFolder = "day"; // Directory for day frames
    const std::string tempFolder = "temp"; // Directory for temp frames

    // Create necessary directories
    createDirectory(dayFolder);
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
        std::vector<FrameData> frameDataList; // Store frame data

        while (difftime(time(0), start_time) < capture_duration) {
            // Capture frame
            flag = cam.capture(frameData);
            if (!flag) continue;

            // Prepare frame data
            FrameData data;
            data.frameID = frame_count++;
            data.timestamp = time(0);
            snprintf(data.filename, sizeof(data.filename), "%s/frame_%d.jpg", tempFolder.c_str(), data.frameID); // Use temp folder for saving temporarily
            imwrite(data.filename, frameData.getFrame()); // Save the frame temporarily

            // Calculate color intensities and percentages
            calculateColorIntensity(frameData.getFrame(), data);
            // Add frame data to the list
            frameDataList.push_back(data);

            // Display the captured frame
            cv::imshow("libcamera-demo", frameData.getFrame());

            // Wait for key press
            key = cv::waitKey(1);
            if (key == 27) break; // Exit on ESC
        }

        cam.stopCamera();

        std::vector<FrameData> selectedDayFrames;
        selectDayFrames(frameDataList, selectedDayFrames, dayFolder); // Pass the day folder to save frames

        // Save selected day frames and their data
        saveFrameData(selectedDayFrames, binaryFile);
    }

    return 0;
}
