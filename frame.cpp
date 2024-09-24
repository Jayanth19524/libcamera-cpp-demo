#include <iostream>
#include <fstream>
#include <vector>

// Struct to hold frame data
struct FrameData {
    int frameID;
    int blueCount;
    int greenCount;
    int yellowCount;
    int blackCount;
};

// Function to read frame data from a binary file
std::vector<FrameData> readFrameData(const std::string& filename) {
    std::vector<FrameData> frames;
    std::ifstream file(filename, std::ios::binary);

    if (file.is_open()) {
        FrameData frame;
        while (file.read(reinterpret_cast<char*>(&frame), sizeof(FrameData))) {
            frames.push_back(frame);
        }
        file.close();
    } else {
        std::cerr << "Error: Unable to open file for reading." << std::endl;
    }

    return frames;
}

// Main function
int main() {
    const std::string filename = "frame_data.bin";
    std::vector<FrameData> frameDataList = readFrameData(filename);

    // Display the frame data
    for (const auto& frame : frameDataList) {
        std::cout << "Frame ID: " << frame.frameID
                  << ", Blue Count: " << frame.blueCount
                  << ", Green Count: " << frame.greenCount
                  << ", Yellow Count: " << frame.yellowCount
                  << ", Black Count: " << frame.blackCount
                  << std::endl;
    }

    return 0;
}

