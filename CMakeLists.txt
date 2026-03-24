cmake_minimum_required(VERSION 3.21)

# Project name and version
project(APImageFilters VERSION 1.0)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/src)

# Define the header files location
file(GLOB_RECURSE HEADER_FILES ${CMAKE_SOURCE_DIR}/src/*.h)

# Add the executable
# Note, you will need to uncomment different files here as you start to add them to your project
add_executable(APImageFilters
    src/main.cpp
    src/Filter.cpp
    src/SimpleFilter.cpp
    src/Image.cpp
    src/ConvolutionalFilter.cpp
    src/Filters3D.cpp
    src/Projection.cpp
    src/Slice.cpp
    src/SliceVolume.cpp
    src/Volume.cpp
    ${HEADER_FILES}
)

# Enable testing
include(CTest)
enable_testing()

# Include the tests
include(${CMAKE_SOURCE_DIR}/cmdtests.cmake)


# Image unit tests
add_executable(Image_test
    tests/Image_test.cpp
    src/Image.cpp
)
target_include_directories(Image_test PRIVATE ${CMAKE_SOURCE_DIR}/src)
add_test(NAME Image_test COMMAND Image_test WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})


# Histogram unit test executable
add_executable(test_histogram
    tests/test_histogram.cpp
    src/Image.cpp
    src/SimpleFilter.cpp
)
target_include_directories(test_histogram PRIVATE ${CMAKE_SOURCE_DIR}/src)
add_test(NAME test_histogram COMMAND test_histogram WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})




# Grey_Bright_Filter unit test executable
add_executable(Grey_Bright_Filter_test
    tests/Grey_Bright_Filter_test.cpp
    src/Image.cpp
    src/SimpleFilter.cpp
)
target_include_directories(Grey_Bright_Filter_test PRIVATE ${CMAKE_SOURCE_DIR}/src)
add_test(NAME Grey_Bright_Filter_test COMMAND Grey_Bright_Filter_test WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})



# Threshold test unit test executable
add_executable(threshold_test
    tests/threshold_test.cpp
    src/Image.cpp
    src/SimpleFilter.cpp
)
target_include_directories(threshold_test PRIVATE ${CMAKE_SOURCE_DIR}/src)
add_test(NAME threshold_test COMMAND threshold_test WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})


# Salt and Pepper test unit test executable
add_executable(salt_pepper_test
    tests/salt_pepper_test.cpp
    src/Image.cpp
    src/SimpleFilter.cpp
)
target_include_directories(salt_pepper_test PRIVATE ${CMAKE_SOURCE_DIR}/src)
add_test(NAME salt_pepper_test COMMAND salt_pepper_test WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})


# Emboss test unit test executable
add_executable(test_emboss
    tests/test_emboss.cpp
    src/Image.cpp
    src/Filter.cpp
    src/Volume.cpp
    src/SimpleFilter.cpp
    src/ConvolutionalFilter.cpp
)

target_include_directories(test_emboss PRIVATE ${CMAKE_SOURCE_DIR}/src)
add_test(NAME test_emboss COMMAND test_emboss WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})


# Slice unit test executable
add_executable(Slice_test
    tests/Slice_test.cpp
    src/Image.cpp
    src/Volume.cpp
    src/SliceVolume.cpp
    src/Slice.cpp
    src/Filter.cpp
    src/SimpleFilter.cpp
    src/ConvolutionalFilter.cpp
)

target_include_directories(Slice_test PRIVATE ${CMAKE_SOURCE_DIR}/src)
add_test(NAME Slice_test COMMAND Slice_test WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

# Volume unit test executable
add_executable(Volume_test
    tests/Volume_test.cpp
    src/Image.cpp
    src/Volume.cpp
    src/Filter.cpp
    src/SimpleFilter.cpp
)
target_include_directories(Volume_test PRIVATE ${CMAKE_SOURCE_DIR}/src)
add_test(NAME Volume_test COMMAND Volume_test WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
 
# Gaussian blur benchmark
add_executable(bench_gaussian
    tests/bench_gaussian.cpp
    src/Image.cpp
    src/Filter.cpp
    src/SimpleFilter.cpp
    src/ConvolutionalFilter.cpp
    src/Volume.cpp
)
target_include_directories(bench_gaussian PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_compile_options(bench_gaussian PRIVATE -O3)
 
add_executable(SliceVolumeTest
    tests/SliceVolumeTest.cpp
    src/SliceVolume.cpp
    src/Image.cpp
    src/Volume.cpp
    src/Filter.cpp
    src/SimpleFilter.cpp
)
target_include_directories(SliceVolumeTest PRIVATE ${CMAKE_SOURCE_DIR}/src)
add_test(NAME SliceVolumeTest COMMAND SliceVolumeTest WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

# Projection benchmark
add_executable(bench_projection
    tests/bench_projection.cpp
    src/Image.cpp
    src/Volume.cpp
    src/Projection.cpp
    src/SimpleFilter.cpp
    src/Filter.cpp
)
target_include_directories(bench_projection PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_compile_options(bench_projection PRIVATE -O3)


# Projection unit tests
add_executable(Projection_test
    tests/Projection_test.cpp
    src/Image.cpp
    src/Volume.cpp
    src/Projection.cpp
    src/SimpleFilter.cpp
    src/Filter.cpp
)
target_include_directories(Projection_test PRIVATE ${CMAKE_SOURCE_DIR}/src)
add_test(NAME Projection_test COMMAND Projection_test WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

add_executable(bench_slice
    tests/bench_slice.cpp
    src/Image.cpp
    src/Volume.cpp
    src/SliceVolume.cpp
    src/Slice.cpp
    src/Filters3D.cpp
    src/SimpleFilter.cpp
    src/ConvolutionalFilter.cpp
    src/Filter.cpp
)
target_include_directories(bench_slice PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_compile_options(bench_slice PRIVATE -O3)
# Filter3D unit tests
add_executable(Filter3D_test
    tests/Filter3D_test.cpp
    src/Image.cpp
    src/Volume.cpp
    src/SimpleFilter.cpp
    src/Filters3D.cpp
)
target_include_directories(Filter3D_test PRIVATE ${CMAKE_SOURCE_DIR}/src)
add_test(NAME Filter3D_test COMMAND Filter3D_test WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

# Full comprehensive benchmark
add_executable(bench_full
    tests/bench_full.cpp
    src/Image.cpp
    src/Volume.cpp
    src/Filter.cpp
    src/SimpleFilter.cpp
    src/ConvolutionalFilter.cpp
    src/Filters3D.cpp
    src/Projection.cpp
    src/Slice.cpp
)
target_include_directories(bench_full PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_compile_options(bench_full PRIVATE -O3)

# Comprehensive Convolutional Filters test executable
add_executable(test_convolutional_filters
    tests/Conv_filters_tests.cpp
    src/Image.cpp
    src/Filter.cpp
    src/Volume.cpp
    src/SimpleFilter.cpp
    src/ConvolutionalFilter.cpp
)

target_include_directories(test_convolutional_filters PRIVATE ${CMAKE_SOURCE_DIR}/src)
add_test(NAME test_convolutional_filters COMMAND test_convolutional_filters WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})


