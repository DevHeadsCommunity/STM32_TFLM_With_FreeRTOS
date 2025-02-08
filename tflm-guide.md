# Guide to Integrate TensorFlow Lite Micro (TFLM) into STM32 with CMake

Implementing TensorFlow Lite Micro on STM32 involves several steps, each detailed below.

---

## Step 1: Building a Library from TensorFlow Lite Source Code

To integrate TensorFlow Lite Micro, you need its source code, which is available on the official TensorFlow Lite Micro GitHub repository. Follow these steps:

1. Clone the repository:
```bash
   git clone https://github.com/tensorflow/tflite-micro.git
```

2. Go to the `tflite-micro` directory 
```bash 
cd tflite-micro
```
3. Compiling the Library 
```bash 
make -f tensorflow/lite/micro/tools/make/Makefile TARGET=cortex_m_generic TARGET_ARCH=cortex-m4 microlite 
```
if your Microcontroller has `FPU` then use `TARGET_ARCH=cortex-m4+fp` also you can change the `TARGET_ARCH` according your requirements 

---

## Step 2: Add The Compiled library to your project with CMake

If you successfully followed those steps you already have a library named `libtensorflow-microlite.a` in your `{tflite_dir}/gen` directory 
now you have to add this into your project with the help of `CMake`
```cmake 
link_directories(${CMAKE_SOURCE_DIR}/path-to-lib-directory) # Link your Library Directory and Make it accessible

target_link_libraries(${PROJECT_NAME} PRIVATE your-lib-dir-path/libtensorflow-microlite.a) # Link TensorFlow Lite Micro library
```
---

## Step 3: Generate the TFLM Tree
Now, you have to add all the necessary `Headers` to your project but for that you have to generate the TFLM tree because tensorflow lite micro
comes with many dependencies which it needs to fetch from the internet and then it will provide you the files which you can include in your project

To generate the TFLM Tree use 
```bash 
python3 {your-tflm-dir}/tensorflow/lite/micro/tools/project_generation/create_tflm_tree.py /your/desired/path/tflm-tree
```

## Step 4: Add all the necessary `Headers` to your project with CMake
```cmake
target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_SOURCE_DIR}/tflm/third_party/flatbuffers/include
    ${CMAKE_SOURCE_DIR}/tflm/third_party/gemmlowp
    ${CMAKE_SOURCE_DIR}/tflm
    ${CMAKE_SOURCE_DIR}/tflm/tensorflow/lite
)
```

**Note: these files are included according to my project setup you have to modify according to your project structure**

## Few Additional Tips
1. Change your filename from `main.c` to `main.cc` or `main.cpp`
2. You may have to use `third_party` tools like `flatbuffers` or `kissfft` then follow step 4. 
3. You can delete all the source files since we already have compiled the `library` according to our MCU Architecture
