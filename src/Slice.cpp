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
 
 * @file Slice.cpp
 * @brief Implementation of 2D slice extraction from 3D volumetric data.
 * @details This file implements the methods to extract orthogonal 2D slices 
 * (e.g., YZ or XZ planes) from a 3D Volume object, handling coordinate 
 * conversion (1-based to 0-based) and orientation mapping.
 */


#include "Slice.h"
#include <stdexcept>
#include <algorithm>

/*
Internal helper: extract a single slice at 0-based fixedIdx

pivot = 'X' (YZ plane, fix x):
    horizontal axis = y,  vertical axis = z
    output width  = vol.getHeight()  (y dimension)
    output height = vol.getDepth()   (z dimension)

pivot = 'Y' (XZ plane, fix y):
    horizontal axis = x,  vertical axis = z
    output width  = vol.getWidth()   (x dimension)
    output height = vol.getDepth()   (z dimension)

In both cases:
    lower-left (1,1) -> 0-based (0,0)
    z is flipped: output row 0 (top of image) = highest z
*/

Image Slice::extract(const Volume& vol, int fixedIdx, char pivot) {
    int d = vol.getDepth();       // z — always the vertical axis
    int ch = vol.getChannels();   // number of colour channels

    // horizontal dimension depends on pivot
    int hSize;  // horizontal size of output image
    if (pivot == 'X') {
        hSize = vol.getHeight();  // y is horizontal
    } else {
        hSize = vol.getWidth();   // x is horizontal
    }

    Image img(hSize, d, ch);

    for (int c = 0; c < ch; ++c) {
        for (int z = 0; z < d; ++z) {
            // flip z: output row 0 = top = highest z
            int outRow = d - 1 - z;
            for (int h = 0; h < hSize; ++h) {
                unsigned char val;
                if (pivot == 'X') {
                    // fix x = fixedIdx, horizontal = y = h
                    val = vol.getVoxel(fixedIdx, h, z, c);
                } else {
                    // fix y = fixedIdx, horizontal = x = h
                    val = vol.getVoxel(h, fixedIdx, z, c);
                }
                img.setPixel(h, outRow, c, val);
            }
        }
    }

    return img;
}

// Single slice (1-based coordinate as per spec)
Image Slice::slice(const Volume& vol, int coord, char pivot) {
    int idx = coord - 1;  // convert 1-based to 0-based

    // validate pivot and check range
    int maxDim;
    std::string dimName;

    if (pivot == 'X' || pivot == 'x') {
        pivot = 'X';  // normalise
        maxDim = vol.getWidth();
        dimName = "x";
    } else if (pivot == 'Y' || pivot == 'y') {
        pivot = 'Y';
        maxDim = vol.getHeight();
        dimName = "y";
    } else {
        throw std::invalid_argument(
            "Slice::slice: pivot must be 'X' or 'Y', got '" +
            std::string(1, pivot) + "'");
    }

    if (idx < 0 || idx >= maxDim) {
        throw std::out_of_range(
            "Slice::slice: " + dimName + " coordinate " +
            std::to_string(coord) +
            " out of range [1, " + std::to_string(maxDim) + "]");
    }

    return extract(vol, idx, pivot);
}
