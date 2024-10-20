#include <opencv2/opencv.hpp>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include "LibCamera.h" // Ensure to include your LibCamera header
#include <fstream>
#include <vector>
#include <algorithm>
#include <sys/stat.h>
#include <cstdlib>
#include <cstdio>
#include <jpeglib.h>
#include <math.h>

using namespace cv;

extern "C" void compress_image(const char *input_filename, const char *output_filename, int quality, int downsample_factor) {
    struct jpeg_decompress_struct cinfo;
    struct jpeg_compress_struct cjpeg;
    struct jpeg_error_mgr jerr;

    // Set up the error handling
    cinfo.err = jpeg_std_error(&jerr);
    cjpeg.err = jpeg_std_error(&jerr);

    // Open the input file
    FILE *infile = fopen(input_filename, "rb");
    if (!infile) {
        std::cerr << "Cannot open " << input_filename << std::endl;
        return;
    }

    // Initialize the decompression
    jpeg_create_decompress(&cinfo);
    jpeg_stdio_src(&cinfo, infile);
    jpeg_read_header(&cinfo, TRUE);

    // Start decompression
    jpeg_start_decompress(&cinfo);

    // Set up the compression
    jpeg_create_compress(&cjpeg);
    FILE *outfile = fopen(output_filename, "wb");
    if (!outfile) {
        std::cerr << "Cannot open " << output_filename << std::endl;
        fclose(infile);
        return;
    }
    jpeg_stdio_dest(&cjpeg, outfile);

    // Calculate new dimensions based on downsample factor
    cjpeg.image_width = cinfo.output_width / downsample_factor;
    cjpeg.image_height = cinfo.output_height / downsample_factor;

    cjpeg.input_components = cinfo.output_components;
    cjpeg.in_color_space = cinfo.out_color_space;

    jpeg_set_defaults(&cjpeg);
    jpeg_set_quality(&cjpeg, quality, TRUE); // Set lower quality for more compression

    // Start compression
    jpeg_start_compress(&cjpeg, TRUE);

    // Allocate buffer for one scanline (downsampled)
    JSAMPROW row_pointer[1];

    // Loop through the scanlines
    while (cjpeg.next_scanline < cjpeg.image_height) {
        // Allocate a buffer for one scanline
        row_pointer[0] = (JSAMPROW) malloc(cinfo.output_width * cinfo.output_components / (downsample_factor));
        if (row_pointer[0] == NULL) {
            std::cerr << "Memory allocation failed" << std::endl;
            break; // Exit if allocation fails
        }

        // Read one scanline from the decompressed image
        jpeg_read_scanlines(&cinfo, row_pointer, 1);

        // Skip the next scanline for downsampling (only use every nth scanline)
        for (int i = 0; i < downsample_factor - 1; i++) {
            jpeg_read_scanlines(&cinfo, row_pointer, 1); // Read again to downsample
        }

        // Write the scanline to the compressed image
        jpeg_write_scanlines(&cjpeg, row_pointer, 1);

        // Free the allocated memory for the scanline
        free(row_pointer[0]);
    }

    // Finish compression
    jpeg_finish_compress(&cjpeg);
    fclose(outfile);

    // Finish decompression
    jpeg_finish_decompress(&cinfo);
    fclose(infile);

    // Clean up
    jpeg_destroy_compress(&cjpeg);
    jpeg_destroy_decompress(&cinfo);

    std::cout << "Image compressed successfully to " << output_filename << std::endl;
}


// Struct to hold frame data
struct FrameData {
    int frameID;
    int blueCount;
    int greenCount;
    int yellowCount;
    int blackCount;
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

// Function to calculate color intensity
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

    // Create masks and count colors
    Mat mask_blue, mask_green, mask_yellow, mask_black;
    inRange(hsv, Scalar(lower_blue[0], lower_blue[1], lower_blue[2]), Scalar(upper_blue[0], upper_blue[1], upper_blue[2]), mask_blue);
    inRange(hsv, Scalar(lower_green[0], lower_green[1], lower_green[2]), Scalar(upper_green[0], upper_green[1], upper_green[2]), mask_green);
    inRange(hsv, Scalar(lower_yellow[0], lower_yellow[1], lower_yellow[2]), Scalar(upper_yellow[0], upper_yellow[1], upper_yellow[2]), mask_yellow);
    inRange(hsv, Scalar(lower_black[0], lower_black[1], lower_black[2]), Scalar(upper_black[0], upper_black[1], upper_black[2]), mask_black);

    // Count colors
    data.blueCount = countNonZero(mask_blue);
    data.greenCount = countNonZero(mask_green);
    data.yellowCount = countNonZero(mask_yellow);
    data.blackCount = countNonZero(mask_black);
}

// Function to create a directory if it does not exist
void createDirectory(const std::string& dirName) {
    struct stat st;
    if (stat(dirName.c_str(), &st) != 0) {
        mkdir(dirName.c_str(), 0777); // Create directory
    }
}
void deleteDirectoryIfExists(const std::string& dirName) {
    std::string command = "rm -rf " + dirName; // Unix command to remove directory
    system(command.c_str()); // Execute the command
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
    const std::string dayFolder = "day";
    const std::string nightFolder = "night";
    const std::string tempFolder = "temp";
    const std::string compressFolder = "compress_imgs"; // Folder for compressed images

    deleteDirectoryIfExists(dayFolder);
    deleteDirectoryIfExists(nightFolder);
    deleteDirectoryIfExists(tempFolder);
    deleteDirectoryIfExists(compressFolder);

    // Create necessary directories
    createDirectory(dayFolder);
    createDirectory(nightFolder);
    createDirectory(tempFolder);
    createDirectory(compressFolder); // Create the compressed images directory

    // Create a window for displaying the camera feed
    cv::namedWindow("libcamera-demo", cv::WINDOW_NORMAL);
    cv::resizeWindow("libcamera-demo", width, height); 

    int ret = cam.initCamera();
    cam.configureStill(width, height, formats::RGB888, 1, 0);
    ControlList controls_;
    int64_t frame_time = 1000000 / 10;
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
            
            // Calculate color intensities
            calculateColorIntensity(im, data);
            frameDataList.push_back(data); // Store frame data in a list

            // Save the frame image in the temp folder
            std::string tempFilename = tempFolder + "/" + data.filename;
            cv::imwrite(tempFilename, im, {cv::IMWRITE_JPEG_QUALITY, 10}); // Set quality to 10

            // Compress the saved image
            std::string compressedFilename = compressFolder + "/compressed_" + data.filename;
            compress_image(tempFilename, compressedFilename, 50); // Set quality to 50 for compression

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

        // Calculate the percentage of black pixels
        double blackPixelPercentage = (static_cast<double>(totalBlackCount) / (totalFrames * width * height)) * 100;

        if (blackPixelPercentage < 50.0) { // Daytime condition
            // Save the top 4 highest blue intensity images
            for (int i = 0; i < std::min(4, static_cast<int>(frameDataList.size())); ++i) {
                const FrameData& topFrame = frameDataList[i];
                // Load the original image using the frame ID or filename if available
                Mat image = imread(topFrame.filename);
                if (!image.empty()) {
                    // Save the top images in the day folder
                    std::string newFilename = dayFolder + "/top_blue_frame_" + std::to_string(i + 1) + ".jpg";
                    imwrite(newFilename, image);
                }
            }
        } else { // Nighttime condition
            // Sort frames based on yellow intensity
            std::sort(frameDataList.begin(), frameDataList.end(), [](const FrameData& a, const FrameData& b) {
                return a.yellowCount > b.yellowCount; // Sort in descending order
            });

            // Save the top 4 highest yellow intensity images
            for (int i = 0; i < std::min(4, static_cast<int>(frameDataList.size())); ++i) {
                const FrameData& topFrame = frameDataList[i];
                // Load the original image using the frame ID or filename if available
                Mat image = imread(topFrame.filename);
                if (!image.empty()) {
                    // Save the top images in the day folder
                    std::string newFilename = nightFolder + "/top_yellow_frame_" + std::to_string(i + 1) + ".jpg";
                    imwrite(newFilename, image);
                }
            }
        }

        destroyAllWindows();
        cam.stopCamera();
        videoWriter.release();
    }
    cam.closeCamera();
    return 0;
}
