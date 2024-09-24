#include <iostream>
#include <fstream>
#include <vector>
#include <ctime>

struct FrameData {
    int frameID;
    int blueCount;
    int greenCount;
    int yellowCount;
    int blackCount;
    time_t timestamp;  // Timestamp for the frame
    char filename[50]; // Filename for saving the image
};

// Function to read frame data from a binary file
std::vector<FrameData> readFrameData(const std::string& filename) {
    std::vector<FrameData> frames;
    std::ifstream file(filename, std::ios::binary);
    if (file.is_open()) {
        while (true) {
            FrameData frame;
            file.read(reinterpret_cast<char*>(&frame), sizeof(FrameData));
            if (file.eof()) break; // Break on end of file
            frames.push_back(frame);
        }
        file.close();
    } else {
        std::cerr << "Error: Unable to open file for reading." << std::endl;
    }
    return frames;
}

int main() {
    const std::string binaryFile = "frame_data.bin"; // Name of the binary file

    // Read the frame data from the binary file
    std::vector<FrameData> frames = readFrameData(binaryFile);

    // Display the frame data
    for (const auto& frame : frames) {
        std::cout << "Frame ID: " << frame.frameID << std::endl;
        std::cout << "Timestamp: " << ctime(&frame.timestamp); // Convert timestamp to human-readable form
        std::cout << "Blue Count: " << frame.blueCount << std::endl;
        std::cout << "Green Count: " << frame.greenCount << std::endl;
        std::cout << "Yellow Count: " << frame.yellowCount << std::endl;
        std::cout << "Black Count: " << frame.blackCount << std::endl;
        std::cout << "Filename: " << frame.filename << std::endl;
        std::cout << "------------------------------------" << std::endl;
    }

    return 0;
}
