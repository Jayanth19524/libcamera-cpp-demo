          
```
sudo apt install -y libopencv-dev
sudo apt install -y cmake

g++ -std=c++17 -o opencvimwrite opencvnolib.cpp `pkg-config --cflags --libs opencv4 libcamera`
opencvnolib.cpp: In constructor ‘FrameCapture::FrameCapture()’:
opencvnolib.cpp:24:20: error: no matching function for call to ‘libcamera::FrameBufferAllocator::FrameBufferAllocator()’
   24 |     FrameCapture() {
      |                    ^
In file included from /usr/include/libcamera/libcamera/libcamera.h:19,
                 from opencvnolib.cpp:1:
/usr/include/libcamera/libcamera/framebuffer_allocator.h:25:2: note: candidate: ‘libcamera::FrameBufferAllocator::FrameBufferAllocator(std::shared_ptr<libcamera::Camera>)’
   25 |  FrameBufferAllocator(std::shared_ptr<Camera> camera);
      |  ^~~~~~~~~~~~~~~~~~~~
/usr/include/libcamera/libcamera/framebuffer_allocator.h:25:2: note:   candidate expects 1 argument, 0 provided
opencvnolib.cpp:26:40: error: ‘create’ is not a member of ‘libcamera::CameraManager’
   26 |         cameraManager = CameraManager::create();
      |                                        ^~~~~~
opencvnolib.cpp:35:27: error: cannot convert ‘__gnu_cxx::__alloc_traits<std::allocator<std::shared_ptr<libcamera::Camera> >, std::shared_ptr<libcamera::Camera> >::value_type’ {aka ‘std::shared_ptr<libcamera::Camera>’} to ‘libcamera::Camera*’ in assignment
   35 |         camera = cameras[0];
      |                           ^
opencvnolib.cpp: In member function ‘void FrameCapture::configureCamera()’:
opencvnolib.cpp:84:42: error: ‘class libcamera::CameraConfiguration’ has no member named ‘streams’
   84 |         allocator.allocate(cameraConfig->streams());
      |                                          ^~~~~~~
opencvnolib.cpp: In member function ‘void FrameCapture::captureFrame(int)’:
opencvnolib.cpp:94:33: error: ‘class libcamera::FrameBufferAllocator’ has no member named ‘get’
   94 |         auto buffer = allocator.get(cameraConfig->streams().at(0)->stream()).front();
      |                                 ^~~
opencvnolib.cpp:94:51: error: ‘class libcamera::CameraConfiguration’ has no member named ‘streams’
   94 |         auto buffer = allocator.get(cameraConfig->streams().at(0)->stream()).front();
      |                                                   ^~~~~~~
opencvnolib.cpp:95:42: error: ‘class libcamera::CameraConfiguration’ has no member named ‘streams’
   95 |         request->addBuffer(cameraConfig->streams().at(0)->stream(), buffer);
      |                                          ^~~~~~~
opencvnolib.cpp:96:30: error: cannot convert ‘std::unique_ptr<libcamera::Request>’ to ‘libcamera::Request*’
   96 |         camera->queueRequest(request);
      |                              ^~~~~~~
      |                              |
      |                              std::unique_ptr<libcamera::Request>
In file included from /usr/include/libcamera/libcamera/libcamera.h:11,
                 from opencvnolib.cpp:1:
/usr/include/libcamera/libcamera/camera.h:122:28: note:   initializing argument 1 of ‘int libcamera::Camera::queueRequest(libcamera::Request*)’
  122 |  int queueRequest(Request *request);
      |                   ~~~~~~~~~^~~~~~~
opencvnolib.cpp:100:41: error: ‘class libcamera::Camera’ has no member named ‘waitForRequest’
  100 |         auto completedRequest = camera->waitForRequest();
      |                                         ^~~~~~~~~~~~~~
opencvnolib.cpp:103:52: error: ‘RequestCompleted’ is not a member of ‘libcamera::Request’
  103 |         if (completedRequest->status() == Request::RequestCompleted) {
      |                                                    ^~~~~~~~~~~~~~~~
opencvnolib.cpp:104:68: error: ‘const value_type’ {aka ‘const struct libcamera::FrameBuffer::Plane’} has no member named ‘mem’
  104 |             auto yuvBuffer = request->buffers().at(0)->planes()[0].mem();
      |                                                                    ^~~
opencvnolib.cpp:105:40: error: ‘class libcamera::CameraConfiguration’ has no member named ‘streams’
  105 |             auto width = cameraConfig->streams().at(0)->configuration().size.width;
      |                                        ^~~~~~~
opencvnolib.cpp:106:41: error: ‘class libcamera::CameraConfiguration’ has no member named ‘streams’
  106 |             auto height = cameraConfig->streams().at(0)->configuration().size.height;
```

```
mkdir build
cd build 
cmake ..
make -j4
g++ -o opencvimwrite opencvimwrite.cpp -I/usr/include/opencv4 -I/usr/include/opencv -L/usr/lib -lopencv_core -lopencv_imgcodecs -lopencv_highgui -lopencv_imgproc
```
