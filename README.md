# Libcamera c++ demo


```
sudo apt install -y libopencv-dev
sudo apt install -y cmake
```

```
mkdir build
cd build 
cmake ..
make -j4
g++ -o opencvimwrite opencvimwrite.cpp -I/usr/include/opencv4 -I/usr/include/opencv -L/usr/lib -lopencv_core -lopencv_imgcodecs -lopencv_highgui -lopencv_imgproc
```
