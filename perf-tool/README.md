# Build Instructions
1. Make sure you have CMake installed & updated. To do so on ubuntu, run `sudo apt-get install cmake`.  
2. Create build directory `mkdir build`, otherwise if the directory already exists `cd build` and run `cmake ..`. This will generate the makefile.  
3. Now to build the project, run `make all`. The generated `libagent.so` will be in the `build` directory.