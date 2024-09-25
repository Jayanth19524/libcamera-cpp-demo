#include <iostream>
#include <fstream>
#include <vector>
#include <iomanip>

struct FrameData {
    int frameID;
    double bluePercentage;
    double greenPercentage;
    double yellowPercentage;
    double blackPercentage;
    time_t timestamp;
    char filename[50];
};

void readFrameData(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "Error: Unable to open file." << std::endl;
        return;
    }

    std::vector<FrameData> frames;
    FrameData frame;

    while (file.read(reinterpret_cast<char*>(&frame), sizeof(FrameData))) {
        frames.push_back(frame);
    }

    file.close();

    // Display the data
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Frame Data:" << std::endl;
    std::cout << "-------------------------------------------" << std::endl;
    for (const auto& f : frames) {
        std::cout << "Frame ID: " << f.frameID << std::endl;
        std::cout << "Timestamp: " << ctime(&f.timestamp); // Convert to readable format
        std::cout << "Blue Percentage: " << f.bluePercentage << "%" << std::endl;
        std::cout << "Green Percentage: " << f.greenPercentage << "%" << std::endl;
        std::cout << "Yellow Percentage: " << f.yellowPercentage << "%" << std::endl;
        std::cout << "Black Percentage: " << f.blackPercentage << "%" << std::endl;
        std::cout << "Filename: " << f.filename << std::endl;
        std::cout << "-------------------------------------------" << std::endl;
    }
}

int main() {
    std::string filename = "frame_data.bin"; // Specify the path to your binary file
    readFrameData(filename);
    return 0;
}
