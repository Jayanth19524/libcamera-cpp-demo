#include <iostream>
#include <memory>
#include <mutex>
#include <vector>
#include <chrono>
#include <thread>
#include <libcamera/libcamera.h>
#include <libcamera/camera_manager.h>
#include <libcamera/framebuffer_allocator.h>
#include <libcamera/request.h>
#include <libcamera/stream.h>
#include <opencv2/opencv.hpp>  // For image saving

using namespace libcamera;

class LibCamera {
public:
    LibCamera() : camera_(nullptr), allocator_(nullptr), camera_acquired_(false), camera_started_(false) {}
    ~LibCamera() { closeCamera(); }

    int initCamera();
    void configureStill(int width, int height, PixelFormat format, int bufferCount);
    int startCamera();
    void captureFrames(int duration);
    void stopCamera();
    void closeCamera();

private:
    std::unique_ptr<CameraManager> cameraManager_;
    std::shared_ptr<Camera> camera_;
    std::unique_ptr<CameraConfiguration> config_;
    std::unique_ptr<FrameBufferAllocator> allocator_;
    std::vector<std::unique_ptr<Request>> requests_;
    std::mutex camera_mutex_;
    bool camera_acquired_;
    bool camera_started_;
    int frame_count_;
};

int LibCamera::initCamera() {
    cameraManager_ = std::make_unique<CameraManager>();
    if (cameraManager_->start() < 0) {
        std::cerr << "Failed to start camera manager" << std::endl;
        return -1;
    }

    const auto &cameras = cameraManager_->cameras();
    if (cameras.empty()) {
        std::cerr << "No cameras found" << std::endl;
        return -1;
    }

    camera_ = cameras[0];
    if (camera_->acquire() < 0) {
        std::cerr << "Failed to acquire camera" << std::endl;
        return -1;
    }

    camera_acquired_ = true;
    return 0;
}

void LibCamera::configureStill(int width, int height, PixelFormat format, int bufferCount) {
    config_ = camera_->generateConfiguration({StreamRole::StillCapture});
    if (width && height) {
        config_->at(0).size = Size(width, height);
    }
    config_->at(0).pixelFormat = format;
    config_->at(0).bufferCount = bufferCount;

    if (config_->validate() != CameraConfiguration::Valid) {
        throw std::runtime_error("Invalid camera configuration");
    }
}

int LibCamera::startCamera() {
    std::lock_guard<std::mutex> lock(camera_mutex_);
    if (camera_->configure(config_.get()) < 0) {
        std::cerr << "Failed to configure camera" << std::endl;
        return -1;
    }

    allocator_ = std::make_unique<FrameBufferAllocator>(camera_);
    unsigned int numBuffers = allocator_->allocate(config_->at(0).stream()).size();

    for (unsigned int i = 0; i < numBuffers; ++i) {
        std::unique_ptr<Request> request = camera_->createRequest();
        const auto &buffers = allocator_->buffers(config_->at(0).stream());
        request->addBuffer(config_->at(0).stream(), buffers[i].get());
        requests_.push_back(std::move(request));
    }

    if (camera_->start() < 0) {
        std::cerr << "Failed to start camera" << std::endl;
        return -1;
    }

    camera_started_ = true;
    frame_count_ = 0; // Reset frame count
    return 0;
}

void LibCamera::captureFrames(int duration) {
    std::lock_guard<std::mutex> lock(camera_mutex_);
    if (!camera_started_ || requests_.empty()) return;

    auto start_time = std::chrono::steady_clock::now();
    while (std::chrono::steady_clock::now() - start_time < std::chrono::seconds(duration)) {
        Request *request = requests_[frame_count_ % requests_.size()].get();
        camera_->queueRequest(request);
        
        // Wait for the request to complete
        request->wait();

        // Process the frame
        const auto &buffers = request->buffers();
        for (const auto &[stream, buffer] : buffers) {
            const FrameBuffer::Plane &plane = buffer->planes()[0];

            // Convert to OpenCV Mat
            cv::Mat img(cv::Size(1920, 1080), CV_8UC3, static_cast<void *>(plane.memory()), cv::Mat::AUTO_STEP);
            std::string filename = "frame_" + std::to_string(frame_count_) + ".jpg";
            cv::imwrite(filename, img); // Save image
            std::cout << "Saved: " << filename << std::endl;
        }

        frame_count_++;
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Capture frame every 100 ms
    }
}

void LibCamera::stopCamera() {
    std::lock_guard<std::mutex> lock(camera_mutex_);
    if (camera_started_) {
        camera_->stop();
        camera_started_ = false;
    }
}

void LibCamera::closeCamera() {
    if (camera_acquired_) {
        camera_->release();
        camera_acquired_ = false;
    }
    camera_.reset();
    cameraManager_.reset();
}

// Example usage
int main() {
    LibCamera libCamera;
    if (libCamera.initCamera() < 0) return -1;

    libCamera.configureStill(1920, 1080, formats::RGB888, 4);
    if (libCamera.startCamera() < 0) return -1;

    // Capture frames for 30 seconds
    libCamera.captureFrames(30);

    libCamera.stopCamera();
    libCamera.closeCamera();
    return 0;
}
