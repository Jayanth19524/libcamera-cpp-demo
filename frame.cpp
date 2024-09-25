#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>

struct FrameData {
    int frameID;
    int blueCount;
    int greenCount;
    int yellowCount;
    int blackCount;
    double bluePercentage;    // Percentage of blue pixels
    double greenPercentage;   // Percentage of green pixels
    double yellowPercentage;  // Percentage of yellow pixels
    double blackPercentage;    // Percentage of black pixels
    time_t timestamp;         // Timestamp for the frame
    char filename[50];        // Filename for saving the image
};

void readFrameData(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error: Unable to open file for reading." << std::endl;
        return;
    }

    std::vector<FrameData> frames;
    FrameData data;

    // Read data until the end of the file
    while (file.read(reinterpret_cast<char*>(&data), sizeof(FrameData))) {
        frames.push_back(data);
    }

    file.close();

    // Display the frame data
    for (const auto& frame : frames) {
        std::cout << "Frame ID: " << frame.frameID << std::endl;
        std::cout << "Timestamp: " << frame.timestamp << std::endl;
        std::cout << "Filename: " << frame.filename << std::endl;
        std::cout << "Blue Count: " << frame.blueCount << ", Blue Percentage: " << frame.bluePercentage << "%" << std::endl;
        std::cout << "Green Count: " << frame.greenCount << ", Green Percentage: " << frame.greenPercentage << "%" << std::endl;
        std::cout << "Yellow Count: " << frame.yellowCount << ", Yellow Percentage: " << frame.yellowPercentage << "%" << std::endl;
        std::cout << "Black Count: " << frame.blackCount << ", Black Percentage: " << frame.blackPercentage << "%" << std::endl;
        std::cout << "---------------------------------------------" << std::endl;
    }
}

int main() {
    const std::string binaryFile = "frame_data.bin"; // Name of the binary file
    readFrameData(binaryFile);
    return 0;
}
