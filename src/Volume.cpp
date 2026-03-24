/**
 * /////////////////////////////////////////////////////////
 * BarnesHut
 * /////////////////////////////////////////////////////////
 * @author Akira C T Eisenbeiss  (GitHub: @ada-ace25)
 * @author Ju Lin (GitHub: @ada-jl4025)
 * @author Yichen Liu (GitHub: @ada-yl2425)
 * @author Siyuan Yuan (GitHub: @ada-sy1124)
 * @author Charli Maguire (GitHub: @ada-cm1625)
 * @author Jiacheng Zhao (GitHub: @ada-jz1225)
 * @author Siqi Yao (GitHub: @ada-sy325)
 
 * ---------------------------------------------------------
 
 * @file Volume.cpp
 * @brief Implementation of the Volume class for 3D volumetric data management.
 * @details This file implements the methods for the Volume class, including 
 * multi-channel volumetric data allocation, optimized file sequence loading 
 * (with zero-padding detection and binary search for bounds), voxel access 
 * with bounds clamping, and deep copying.
 */


#include "Volume.h"
#include <stdexcept>
#include <algorithm>
#include <iomanip>
#include "SimpleFilter.h"
#include <iostream>
#include <sstream>
#include <fstream>

// constructors
// Initialize with zeroes to ensure no garbage data exists if methods are called before load()
Volume::Volume() : width(0), height(0), depth(0), channels(0) {}

// Allocating a 3D array creates severe memory fragmentation and causes cache misses. Flattening W x H x D x C into a single 
// contiguous 1D vector is the standard, high-performance way to manage volumetric data.
Volume::Volume(int w, int h, int d, int c)
    : width(w), height(h), depth(d), channels(c),
      data(static_cast<size_t>(w) * h * d * c, 0) {}

Volume::~Volume() {}



// File loading helpers (from teammate's optimized implementation)

// Volumetric sequences often have hundreds of slices. By passing `pad` by reference, we discover the format on the very first 
// slice and reuse it, avoiding 5 failed file-open attempts for every single subsequent slice.
static std::unique_ptr<Image> loadSlice(const std::string& prefix,
                                         int index,
                                         const std::string& extension,
                                         int& pad) {
    int start = (pad >= 0) ? pad : 5;
    int end   = (pad >= 0) ? pad : 0;
    
    // Loop through padding widths (e.g., 00001, 0001, 001, 01, 1)
    for (int p = start; p >= end; p--) {
        std::ostringstream oss;
        if (p > 0)
            oss << prefix << std::setw(p) << std::setfill('0') << index << extension;
        else
            oss << prefix << index << extension;
        
        // Attempt to parse the image data
        auto img = Image::load(oss.str());
        if (img) {
            pad = p; // Lock in the working padding width
            return img;
        }
    }
    return nullptr;
}

// Check if a file exists (without fully loading the image).
// Uses the same padding detection logic as loadSlice. Fully decoding a PNG/JPG using stb_image is computationally 
// expensive. When we are just trying to find how many slices exist in a folder, we only 
// need to ask the OS if the file exists using std::ifstream, which is drastically faster.
static bool sliceExists(const std::string& prefix,
                         int index,
                         const std::string& extension,
                         int& pad) {
    int start = (pad >= 0) ? pad : 5;
    int end   = (pad >= 0) ? pad : 0;
    for (int p = start; p >= end; p--) {
        std::ostringstream oss;
        if (p > 0)
            oss << prefix << std::setw(p) << std::setfill('0') << index << extension;
        else
            oss << prefix << index << extension;
        
        std::ifstream f(oss.str()); // Open file stream
        if (f.good()) {             // Check if file is readable
            pad = p;
            return true;
        }
    }
    return false;
}



// Volume::load — multi-channel with optimized file detection

bool Volume::load(const std::string& prefix,
                  int first,
                  int last,
                  std::string extension) {

    // ensure extension has a leading dot
    if (!extension.empty() && extension[0] != '.') {
        extension = "." + extension;
    }

    bool autoDetect = (first < 0 || last < 0);
    int detectedPad = -1;  // -1 = not yet detected

    // If the user didn't specify exactly how many slices to load, we must auto-detect bounds.
    if (autoDetect) {
        // try starting from 0, if that fails try 1 (common indexing schemes)
        first = 0;
        if (!sliceExists(prefix, 0, extension, detectedPad)) {
            first = 1;
            if (!sliceExists(prefix, 1, extension, detectedPad)) {
                return false; // Neither 0 nor 1 exist, volume cannot be found
            }
        }

        // EXPONENTIAL SEARCH: Instead of checking file 2, then 3, then 4... (O(N) time), 
        // we check 1, 2, 4, 8, 16, 32... until we hit a missing file. 
        // This quickly establishes an upper bound for our sequence.
        int bound = 1;
        while (sliceExists(prefix, first + bound, extension, detectedPad)) {
            bound *= 2;
        }

        // BINARY SEARCH: Now that we know the last file is somewhere between `bound/2` and `bound`,
        // we use a binary search to pinpoint the exact last index in O(log N) time.
        // For a volume of 500 slices, this reduces file checks from 500 down to about 18.
        int lo = first + bound / 2;
        int hi = first + bound;
        while (lo < hi) {
            int mid = lo + (hi - lo) / 2;
            if (sliceExists(prefix, mid, extension, detectedPad)) {
                lo = mid + 1; // It exists, so the end is further right
            } else {
                hi = mid;     // It doesn't exist, so the end is here or to the left
            }
        }
        last = lo - 1; // lo is the first missing file, so lo-1 is the last valid file

        if (last < first) return false;
    }

    int numSlices = last - first + 1;

    // Now actually load the image data into memory
    for (int i = first; i <= last; i++) {

        auto slice = loadSlice(prefix, i, extension, detectedPad);
        if (!slice) {
            std::cerr << "Failed to load slice " << i << std::endl;
            return false;
        }

        int sliceW = slice->getWidth();
        int sliceH = slice->getHeight();
        int sliceC = slice->getChannels();

        if (i == first) {
            // first slice sets the master volume dimensions and channel count
            width    = sliceW;
            height   = sliceH;
            depth    = numSlices;
            channels = sliceC;
            
            // Allocate the entire 3D array memory at once.
            // Using size_t prevents integer overflow on very large volumes 
            data.resize(static_cast<size_t>(width) * height * depth * channels, 0);
        } else {
            // Validate subsequent slices against the master dimensions
            if (sliceW != width || sliceH != height) {
                throw std::runtime_error(
                    "Volume::load: slice " + std::to_string(i) +
                    " has different dimensions than first slice");
            }
            // gracefully handle channel count mismatch
            if (sliceC != channels) {
                // If the volume is grayscale but a slice is RGB, force the slice to grayscale
                if (channels == 1 && sliceC > 1) {
                    GreyscaleFilter grey;
                    grey.apply(*slice);
                } else {
                    throw std::runtime_error(
                        "Volume::load: slice " + std::to_string(i) +
                        " has incompatible channel count (" +
                        std::to_string(sliceC) + " vs " +
                        std::to_string(channels) + ")");
                }
            }
        }

        // copy 2D slice data straight into the correct Z-offset in the 1D volume buffer
        int z = i - first;
        size_t sliceSize = static_cast<size_t>(width) * height * channels;
        size_t sliceOffset = static_cast<size_t>(z) * sliceSize;

        const std::vector<unsigned char>& sliceData = slice->getData();
        for (size_t j = 0; j < sliceSize; j++) {
            data[sliceOffset + j] = sliceData[j];
        }
    }

    return true;
}


// Properties

int Volume::getWidth()    const { return width; }
int Volume::getHeight()   const { return height; }
int Volume::getDepth()    const { return depth; }
int Volume::getChannels() const { return channels; }


// Voxel access — multi-channel

unsigned char Volume::getVoxel(int x, int y, int z, int c) const {
    if (width <= 0 || height <= 0 || depth <= 0)
        throw std::out_of_range("Cannot read from an empty volume");

    // CLAMPING: Protects against out-of-bounds indexing which causes segfaults. 
    // This allows 3D filters (like Gaussian) to safely read "off the edge" of the volume 
    // by mirroring the boundary voxels.
    x = std::clamp(x, 0, width  - 1);
    y = std::clamp(y, 0, height - 1);
    z = std::clamp(z, 0, depth  - 1);
    c = std::clamp(c, 0, channels - 1);

    return data[((static_cast<size_t>(z) * height + y) * width + x) * channels + c];
}

void Volume::setVoxel(int x, int y, int z, int c, unsigned char value) {
    if (x < 0 || x >= width || y < 0 || y >= height || z < 0 || z >= depth)
        throw std::out_of_range("Voxel coordinates out of range");
    if (c < 0 || c >= channels)
        throw std::out_of_range("Channel index out of range");

    data[((static_cast<size_t>(z) * height + y) * width + x) * channels + c] = value;
}

// convenience overload: defaults to channel 0 for purely grayscale logic
void Volume::setVoxel(int x, int y, int z, unsigned char value) {
    setVoxel(x, y, z, 0, value);
}

float Volume::getVoxelAsFloat(int x, int y, int z, int c) const {
    return static_cast<float>(getVoxel(x, y, z, c));
}

void Volume::setVoxelClamped(int x, int y, int z, int c, int value) {
    // Guards against integer overflow/underflow when filters write back mathematical results
    setVoxel(x, y, z, c, static_cast<unsigned char>(std::clamp(value, 0, 255)));
}

void Volume::setVoxelClamped(int x, int y, int z, int value) {
    setVoxelClamped(x, y, z, 0, value);
}



// Raw data access

std::vector<unsigned char>& Volume::getData() { return data; }
const std::vector<unsigned char>& Volume::getData() const { return data; }



// Deep copy

std::unique_ptr<Volume> Volume::clone() const {
    // Provides a safe way to snapshot the volume before destructive operations
    auto copy = std::make_unique<Volume>(width, height, depth, channels);
    copy->data = data; // Relies on std::vector's built-in deep copy assignment operator
    return copy;
}