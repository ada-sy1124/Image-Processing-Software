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
 
 * @file SliceVolume.cpp
 * @brief Implementation of the SliceVolume class for optimized slice extraction.
 * @details This file implements methods to temporarily rotate and crop a 3D volume 
 * to isolate a specific slab (XZ or YZ planes) based on a kernel size. This optimizes 
 * memory usage and processing time when applying 3D convolution operations prior 
 * to extracting a 2D slice.
 */


#include "SliceVolume.h"
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <memory>
#include <algorithm>
#include <iomanip>
#include <cassert>

// Takes a volume and either rotates it to XZ or YZ orientation, then extracts the specified slice as a 2D image.
// coord is 1-based slice coordinate as per spec, axis is 'X' or 'Y' for the slice plane, 
// kernSize is the size of the kernel to use for rotation (must be odd).

// This helps optimise for when we apply a blur 
// Allows to ignore (kernelsize - 1 ) * height * width for one of the passes. More helpful when the kernel is larger
// Also more memory efficient 
void SliceVolume::rotate(int coord, int kernSize, char plane) {
    if (plane != 'X' && plane != 'Y') {
        throw std::runtime_error("Invalid plane. Must be 'X' or 'Y'.");
    }
    if (kernSize <= 0 || kernSize % 2 == 0) {
        throw std::runtime_error("Invalid kernel size. Must be a positive odd integer.");
    }
    rotatedPlane = plane;

    // Convert 1-based coord to 0-based index
    int idx = coord - 1;

    // Calculate the range of slices based on the coordinate and kernel size
    int firstSlice;
    int lastSlice;
    if (plane == 'X') {
        firstSlice = std::max(idx - kernSize / 2, 0);
        lastSlice = std::min(idx + kernSize / 2, width - 1);
    } else {
        firstSlice = std::max(idx - kernSize / 2, 0);
        lastSlice = std::min(idx + kernSize / 2, height - 1);
    }
    int slabSize = lastSlice - firstSlice + 1;

    // Determine the dimensions of the rotated volume
    int rotatedWidth, rotatedHeight, rotatedDepth;
    if (plane == 'X') {
        // Keep Y and Z as full dimensions; crop only X to slab around coord
        rotatedWidth = slabSize;
        rotatedHeight = height;
        rotatedDepth = depth;
    } else {
        rotatedWidth = width;
        // Keep X and Z as full dimensions; crop only Y to slab around coord
        rotatedHeight = slabSize;
        rotatedDepth = depth;
    }

    const int ch = channels;

    // Create a new data vector for the rotated volume
    std::vector<unsigned char> rotatedData(
        static_cast<size_t>(rotatedWidth) * rotatedHeight * rotatedDepth * ch, 0);

    auto dstIndex = [rotatedWidth, rotatedHeight, ch](int x, int y, int z, int c) -> size_t {
        return (((static_cast<size_t>(z) * rotatedHeight + y) * rotatedWidth + x) * ch) + c;
    };
    auto srcIndex = [this](int x, int y, int z, int c) -> size_t {
        return (((static_cast<size_t>(z) * height + y) * width + x) * channels) + c;
    };

    // Perform the rotation
    if (plane == 'X') {
        for (int z = 0; z < rotatedDepth; ++z) {
            for (int y = 0; y < rotatedHeight; ++y) {
                for (int x = 0; x < rotatedWidth; ++x) {
                    int srcX = firstSlice + x;
                    for (int c = 0; c < ch; ++c) {
                        rotatedData[dstIndex(x, y, z, c)] = data[srcIndex(srcX, y, z, c)];
                    }
                }
            }
        }
    } else {
        for (int z = 0; z < rotatedDepth; ++z) {
            for (int y = 0; y < rotatedHeight; ++y) {
                int srcY = firstSlice + y;
                for (int x = 0; x < rotatedWidth; ++x) {
                    for (int c = 0; c < ch; ++c) {
                        rotatedData[dstIndex(x, y, z, c)] = data[srcIndex(x, srcY, z, c)];
                    }
                }
            }
        }
    }

    // Update the volume dimensions and data
    width = rotatedWidth;
    height = rotatedHeight;
    depth = rotatedDepth;
    data = std::move(rotatedData);
}

// Assumes the volume is rotated to the correct orientation.
// Extracts a 2D slice from the kernel center, matching Slice::sliceYZ/XZ output.
//
// After rotate with plane='X':
//   width=kernSize (orig z), height=orig_height (y), depth=orig_width (x)
//   Kernel center at x = width/2.  Output = height x depth to match sliceYZ(h, d).
//
// After rotate with plane='Y':
//   width=orig_width (x), height=kernSize (orig z), depth=orig_height (y)
//   Kernel center at y = height/2.  Output = width x depth to match sliceXZ(w, d).
Image SliceVolume::slice(int coord) {
    (void)coord;
    if (rotatedPlane == 'X') {
        // YZ slice: kernel in width, output is height(=orig_y) x depth(=orig_x)
        int xCenter = width / 2;
        Image img(height, depth, channels);
        for (int z = 0; z < depth; z++) {
            int outRow = depth - 1 - z;
            for (int y = 0; y < height; y++) {
                for (int c = 0; c < channels; ++c) {
                    img.setPixel(y, outRow, c, getVoxel(xCenter, y, z, c));
                }
            }
        }
        return img;
    } else {
        // XZ slice: kernel in height, output is width(=orig_x) x depth(=orig_y)
        int yCenter = height / 2;
        Image img(width, depth, channels);
        for (int z = 0; z < depth; z++) {
            int outRow = depth - 1 - z;
            for (int x = 0; x < width; x++) {
                for (int c = 0; c < channels; ++c) {
                    img.setPixel(x, outRow, c, getVoxel(x, yCenter, z, c));
                }
            }
        }
        return img;
    }
}