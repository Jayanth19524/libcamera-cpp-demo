#include <iostream>
#include <fstream>
#include <vector>
#include <ctime>
#include <iomanip>

struct FrameData {
    int frameID;
    time_t timestamp;
    float blueIntensity;
    float greenIntensity;
    float brownIntensity;
    float whiteIntensity;
    float blackIntensity;
    float yellowIntensity;
    char filename[50];
};

void printFrameData(const FrameData& frameData) {
    std::cout << "Frame ID: " << frameData.frameID << std::endl;
    std::cout << "Timestamp: " << std::put_time(std::localtime(&frameData.timestamp), "%Y-%m-%d %H:%M:%S") << std::endl;
    std::cout << "Blue Intensity: " << frameData.blueIntensity << std::endl;
    std::cout << "Green Intensity: " << frameData.greenIntensity << std::endl;
    std::cout << "Brown Intensity: " << frameData.brownIntensity << std::endl;
    std::cout << "White Intensity: " << frameData.whiteIntensity << std::endl;
    std::cout << "Black Intensity: " << frameData.blackIntensity << std::endl;
    std::cout << "Yellow Intensity: " << frameData.yellowIntensity << std::endl;
    std::cout << "Filename: " << frameData.filename << std::endl;
    std::cout << "------------------------------" << std::endl;
}

int main() {
    std::string filename = "frame_data.bin";
    std::ifstream file(filename, std::ios::binary);

    if (!file) {
        std::cerr << "Error opening file: " << filename << std::endl;
        return 1;
    }

    FrameData frameData;
    while (file.read(reinterpret_cast<char*>(&frameData), sizeof(FrameData))) {
        printFrameData(frameData);
    }

    file.close();
    return 0;
}
