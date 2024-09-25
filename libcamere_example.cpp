#include <iomanip>
#include <iostream>
#include <memory>
#include <thread>

#include <libcamera/libcamera.h>

using namespace libcamera;
using namespace std::chrono_literals;

int main()
{
    std::unique_ptr<CameraManager> cm = std::make_unique<CameraManager>();
    cm->start();

    for (auto const &camera : cm->cameras())
    std::cout << camera->id() << std::endl;

    std::unique_ptr<CameraConfiguration> config = camera->generateConfiguration( { StreamRole::Viewfinder } );

    StreamConfiguration &streamConfig = config->at(0);
    std::cout << "Default viewfinder configuration is: " << streamConfig.toString() << std::endl;

    streamConfig.size.width = 640;
    streamConfig.size.height = 480;
     
     config->validate();
std::cout << "Validated viewfinder configuration is: " << streamConfig.toString() << std::endl;
    
    camera->configure(config.get());

    FrameBufferAllocator *allocator = new FrameBufferAllocator(camera);

for (StreamConfiguration &cfg : *config) {
    int ret = allocator->allocate(cfg.stream());
    if (ret < 0) {
        std::cerr << "Can't allocate buffers" << std::endl;
        return -ENOMEM;
    }

    size_t allocated = allocator->buffers(cfg.stream()).size();
    std::cout << "Allocated " << allocated << " buffers for stream" << std::endl;
}

Stream *stream = streamConfig.stream();
const std::vector<std::unique_ptr<FrameBuffer>> &buffers = allocator->buffers(stream);
std::vector<std::unique_ptr<Request>> requests;

for (unsigned int i = 0; i < buffers.size(); ++i) {
    std::unique_ptr<Request> request = camera->createRequest();
    if (!request)
    {
        std::cerr << "Can't create request" << std::endl;
        return -ENOMEM;
    }

    const std::unique_ptr<FrameBuffer> &buffer = buffers[i];
    int ret = request->addBuffer(stream, buffer.get());
    if (ret < 0)
    {#include <iomanip>
#include <iostream>
#include <memory>
#include <thread>
#include <vector>
#include <opencv2/opencv.hpp>  // Include OpenCV for displaying images

#include <libcamera/libcamera.h>

using namespace libcamera;
using namespace std::chrono_literals;

int main()
{
    // Step 1: Initialize CameraManager
    std::unique_ptr<CameraManager> cm = std::make_unique<CameraManager>();
    cm->start();

    // Step 2: List available cameras and configure the first one
    for (const auto &camera : cm->cameras()) {
        std::cout << "Camera ID: " << camera->id() << std::endl;

        // Generate Camera Configuration
        std::unique_ptr<CameraConfiguration> config = camera->generateConfiguration({StreamRole::Viewfinder});
        StreamConfiguration &streamConfig = config->at(0);
        streamConfig.size.width = 640;
        streamConfig.size.height = 480;

        // Validate and configure the camera
        config->validate();
        camera->configure(config.get());

        // Step 6: Allocate buffers
        FrameBufferAllocator allocator(camera);
        for (StreamConfiguration &cfg : *config) {
            int ret = allocator.allocate(cfg.stream());
            if (ret < 0) {
                std::cerr << "Can't allocate buffers" << std::endl;
                return ret;
            }
        }

        // Step 7: Prepare requests
        Stream *stream = streamConfig.stream();
        const auto &buffers = allocator.buffers(stream);
        std::vector<std::unique_ptr<Request>> requests;

        for (const auto &buffer : buffers) {
            std::unique_ptr<Request> request = camera->createRequest();
            if (!request) {
                std::cerr << "Can't create request" << std::endl;
                return -ENOMEM;
            }

            int ret = request->addBuffer(stream, buffer.get());
            if (ret < 0) {
                std::cerr << "Can't set buffer for request" << std::endl;
                return ret;
            }

            requests.push_back(std::move(request));
        }

        // Step 8: Start the camera
        camera->start();

        // Step 9: Queue requests and handle frame capture
        while (true) {
            for (auto &request : requests) {
                camera->queueRequest(request.get());
            }

            // Capture and process frames
            for (const auto &buffer : buffers) {
                // Wait for the request to complete and process the buffer
                camera->start();
                auto *frame = buffer->map();
                if (frame) {
                    // Convert the frame to an OpenCV Mat for display
                    cv::Mat img(streamConfig.size.height, streamConfig.size.width, CV_8UC3, frame->data());
                    cv::imshow("Camera Preview", img);
                    if (cv::waitKey(1) >= 0) break; // Exit on any key press
                }
                buffer->unmap();
            }
        }

        // Step 10: Cleanup
        camera->stop();
        camera->release();
        cm->stop();

        break; // Exit after handling the first camera
    }

    return 0;
}

        std::cerr << "Can't set buffer for request"
              << std::endl;
        return ret;
    }

    requests.push_back(std::move(request));
}

camera->stop();
allocator->free(stream);
delete allocator;
camera->release();
camera.reset();


    return 0;
}