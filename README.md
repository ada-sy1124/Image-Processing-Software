# APImageFilters
Advanced Programming Image Filters Project

## About This Project

APImageFilters is a C++ command-line tool for the Advanced Programming group assignment on image filters, projections, and slices. It processes both 2D images and 3D image stacks, and follows the brief's core `Image`, `Volume`, `Filter`, `Projection`, and `Slice` structure.

It currently supports:

- 2D image processing: greyscale, brightness, histogram equalisation, thresholding, salt-and-pepper noise, box/gaussian/median blur, Laplacian sharpening, edge detection, and embossing.
- 3D volume processing: gaussian and median blur, z-axis orthographic projections (`MIP`, `MinIP`, `meanAIP`, `medianAIP`), and `XZ` / `YZ` slice extraction.
- Thin-slab style workflows by loading a restricted slice range with `--first` / `--last` before applying a slice or projection.

## Project Structure

Key files and directories:

```text
advanced-programming-group-barneshut/
├── CMakeLists.txt                  # Build configuration and test targets
├── README.md
├── command_line_options.md         # CLI usage reference
├── cmdtests.cmake                  # CTest command-line integration tests
├── .github/
│   └── workflows/
│       └── build-and-options-check.yml
├── src/
│   ├── main.cpp                    # CLI parsing and dispatch
│   ├── Image.h / Image.cpp         # Concrete image type + RGB/HSL/HSV structs
│   ├── Volume.h / Volume.cpp       # 3D voxel container
│   ├── SliceVolume.h / SliceVolume.cpp  # Helper for efficient blur+slice workflows
│   ├── Filter.h / Filter.cpp       # 2D filter base class
│   ├── SimpleFilter.h / SimpleFilter.cpp
│   ├── ConvolutionalFilter.h / ConvolutionalFilter.cpp
│   ├── Filters3D.h / Filters3D.cpp # 3D filter hierarchy
│   ├── Projection.h / Projection.cpp  # Z-axis volume projections
│   ├── Slice.h / Slice.cpp         # XZ / YZ slice extraction
│   ├── stb_image.h
│   └── stb_image_write.h
├── tests/
│   ├── Image_test.cpp              # Image load/save/pixel tests
│   ├── Volume_test.cpp             # Volume load/voxel tests
│   ├── Grey_Bright_Filter_test.cpp # Greyscale and brightness tests
│   ├── test_histogram.cpp          # Histogram equalisation tests
│   ├── threshold_test.cpp          # Threshold tests
│   ├── salt_pepper_test.cpp        # Salt-and-pepper tests
│   ├── test_emboss.cpp             # Emboss tests
│   ├── Filter3D_test.cpp           # 3D filter tests
│   ├── Projection_test.cpp         # Projection tests
│   ├── Slice_test.cpp              # Slice extraction tests
│   ├── SliceVolumeTest.cpp         # SliceVolume tests
│   ├── bench_gaussian.cpp          # Gaussian blur benchmark
│   ├── bench_projection.cpp        # Projection benchmark
│   ├── bench_slice.cpp             # Slice benchmark
│   └── bench_full.cpp              # End-to-end benchmark
├── Images/                         # Sample 2D test images
└── Scans/                          # Sample 3D volume datasets
```

## Class Structure

```text
Image                          (concrete 2D image container with colour-space conversions)
  Structs: RGBPixel, HSLPixel, HSVPixel

Volume                         (3D voxel container, loads image slice stacks)
└── SliceVolume                (extends Volume with rotation/cropping for optimised blur+slice)

Filter (abstract)
├── SimpleFilter (abstract)
│   ├── GreyscaleFilter
│   ├── BrightnessFilter
│   ├── EqualizeHistogram
│   ├── ThresholdFilter
│   └── SaltPepperFilter
└── ConvolutionalFilter (abstract)
    ├── BoxBlurFilter
    ├── GaussianBlurFilter
    ├── MedianBlurFilter
    ├── SharpenFilter
    ├── SobelFilter
    ├── PrewittFilter
    ├── ScharrFilter
    ├── RobertsCrossFilter
    └── EmbossFilter

Filter3D (abstract)
├── GaussianBlur3DFilter
└── MedianBlur3DFilter

Projection                     (static utility: z-axis MIP, MinIP, AIP, and median projections)

Slice                          (static utility: XZ and YZ slice extraction)
```

## Build and Test

Clone the repository:

```bash
git clone https://github.com/ese-ada-lovelace-2025/advanced-programming-group-barneshut.git
cd advanced-programming-group-barneshut
```

Install CMake and a C++ compiler if needed.

macOS (Homebrew):

```bash
brew install cmake gcc
```

Linux / WSL:

```bash
sudo apt install build-essential cmake gdb
```

Configure the project.

macOS with Homebrew GCC:

```bash
CC=gcc-15 CXX=g++-15 cmake -S . -B build
```

If Homebrew installs a different GCC version suffix, replace `15` with the version available on your machine.

Linux / WSL:

```bash
cmake -S . -B build
```

Build the project:

```bash
cmake --build build
```

Run the tests:

```bash
ctest --test-dir build --output-on-failure
```

## Usage

The program is driven entirely by command-line options. The full option reference lives in [command_line_options.md](command_line_options.md).

Each command starts with either `-i <input_image>` for 2D processing or `-d <data_volume>` for 3D processing, and ends with an output image path.

Example commands:

```bash
./build/APImageFilters -i Images/small.png -g output.png
./build/APImageFilters -d Scans/TestVolume/vol -p MIP output.png
./build/APImageFilters -d Scans/TestVolume/vol --first 4 --last 28 -s XZ 16 output.png
```
