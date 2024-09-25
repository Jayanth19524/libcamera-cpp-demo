#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>
#include <ctime>

// Struct to hold frame data (same as defined in the main program)
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

// Function to read frame data from a binary file
std::vector<FrameData> readFrameData(const std::string& filename) {
    std::vector<FrameData> frames;
    std::ifstream file(filename, std::ios::binary);
    
    if (!file.is_open()) {
        std::cerr << "Error: Unable to open file for reading." << std::endl;
        return frames;
    }

    FrameData data;
    while (file.read(reinterpret_cast<char*>(&data), sizeof(FrameData))) {
        frames.push_back(data);
    }

    file.close();
    return frames;
}

int main() {
    const std::string binaryFile = "frame_data.bin"; // Binary file for frame data

    // Read frame data
    std::vector<FrameData> frameDataList = readFrameData(binaryFile);

    // Print the frame data
    for (const auto& frame : frameDataList) {
        std::cout << "Frame ID: " << frame.frameID << std::endl;
        std::cout << "Timestamp: " << frame.timestamp << std::endl;
        std::cout << "Blue Count: " << frame.blueCount << " (" << frame.bluePercentage << "%)" << std::endl;
        std::cout << "Green Count: " << frame.greenCount << " (" << frame.greenPercentage << "%)" << std::endl;
        std::cout << "Yellow Count: " << frame.yellowCount << " (" << frame.yellowPercentage << "%)" << std::endl;
        std::cout << "Black Count: " << frame.blackCount << " (" << frame.blackPercentage << "%)" << std::endl;
        std::cout << "White Count: " << frame.whiteCount << " (" << frame.whitePercentage << "%)" << std::endl;
        std::cout << "Brown Count: " << frame.brownCount << " (" << frame.brownPercentage << "%)" << std::endl;
        std::cout << "Filename: " << frame.filename << std::endl;
        std::cout << "-----------------------------" << std::endl;
    }

    return 0;
}
