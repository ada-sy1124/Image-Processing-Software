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
 
 * @file Projection.cpp
 * @brief Implementation of 2D image projections from 3D volumetric data.
 * @details This file implements the methods for projecting a 3D volume (or a specific slab) 
 * into a 2D image. It includes Maximum Intensity Projection (MIP), Minimum Intensity 
 * Projection (MinIP), Average Intensity Projection (AIP), and a histogram-based 
 * Median Intensity Projection.
 */


#include "Projection.h"
#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <vector>

// Helper function to validate and normalize the Z-axis boundaries (the "slab").
// We want to modify the caller's zStart and zEnd variables directly 
// so they can pass in raw user input (like -1 for default) and get back safe, clamped bounds.
void Projection::resolveSlabBounds(int depth, int& zStart, int& zEnd) {
    // A value of -1 is our designated "default" flag meaning "use the full range".
    if (zStart < 0) zStart = 0;
    if (zEnd   < 0) zEnd   = depth - 1;
    
    // Clamp to ensure we never read outside the allocated volume memory, preventing segfaults.
    zStart = std::clamp(zStart, 0, depth - 1);
    zEnd   = std::clamp(zEnd,   0, depth - 1);
    
    // Gracefully handle cases where the user inputs the bounds backward (e.g., end before start).
    if (zStart > zEnd) std::swap(zStart, zEnd);
}

// Maximum intensity projection (MIP)
// Commonly used in medical imaging (like CT/MRI angiography) to visualize 
// bright structures (like blood vessels or bone) by projecting the densest voxel along a ray.
Image Projection::maxIntensity(const Volume& volume, int zStart, int zEnd) {
    int w  = volume.getWidth();
    int h  = volume.getHeight();
    int d  = volume.getDepth();
    int ch = volume.getChannels();
    resolveSlabBounds(d, zStart, zEnd);

    // The output is a 2D image with the same width and height as a single volume slice.
    Image out(w, h, ch);

    // Iterate over channels, then Y (rows), then X (columns).
    for (int c = 0; c < ch; ++c) {
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                // Initialize the max tracker with the first voxel in our requested Z-slab.
                unsigned char maxVal = volume.getVoxel(x, y, zStart, c);
                
                // "Ray cast" through the volume along the Z-axis.
                for (int z = zStart + 1; z <= zEnd; ++z) {
                    unsigned char val = volume.getVoxel(x, y, z, c);
                    if (val > maxVal) maxVal = val; // Keep the brightest voxel
                }
                
                // Write the brightest voxel found along that ray to the 2D output image.
                out.setPixel(x, y, c, maxVal);
            }
        }
    }

    return out;
}

// Minimum intensity projection (MinIP)
// The inverse of MIP. Used to visualize low-density structures, 
// such as the bronchial tree (airways) in lung scans.
Image Projection::minIntensity(const Volume& volume, int zStart, int zEnd) {
    int w  = volume.getWidth();
    int h  = volume.getHeight();
    int d  = volume.getDepth();
    int ch = volume.getChannels();
    resolveSlabBounds(d, zStart, zEnd);

    Image out(w, h, ch);

    for (int c = 0; c < ch; ++c) {
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                // Initialize the min tracker with the first voxel in the slab.
                unsigned char minVal = volume.getVoxel(x, y, zStart, c);
                
                // Ray cast along Z, keeping only the darkest voxel.
                for (int z = zStart + 1; z <= zEnd; ++z) {
                    unsigned char val = volume.getVoxel(x, y, z, c);
                    if (val < minVal) minVal = val;
                }
                out.setPixel(x, y, c, minVal);
            }
        }
    }

    return out;
}

// Average intensity projection (AIP)
Image Projection::averageIntensity(const Volume& volume, int zStart, int zEnd) {
    int w  = volume.getWidth();
    int h  = volume.getHeight();
    int d  = volume.getDepth();
    int ch = volume.getChannels();
    resolveSlabBounds(d, zStart, zEnd);

    int slabDepth = zEnd - zStart + 1;
    if (slabDepth <= 0) throw std::runtime_error("Cannot project empty slab");

    Image out(w, h, ch);
    
    // Pre-compute half of the divisor for fast integer rounding.
    int half = slabDepth / 2;

    for (int c = 0; c < ch; ++c) {
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                int sum = 0;
                // Ray cast along Z, accumulating the sum of all voxels.
                for (int z = zStart; z <= zEnd; ++z) {
                    sum += volume.getVoxel(x, y, z, c);
                }
                // This is a standard integer math trick.
                out.setPixel(x, y, c,
                    static_cast<unsigned char>((sum + half) / slabDepth));
            }
        }
    }

    return out;
}

// Median intensity projection
// builds a 256-bin histogram per pixel across the z slab, 
// then walks each histogram to find the median.
Image Projection::medianIntensity(const Volume& volume, int zStart, int zEnd) {
    int w  = volume.getWidth();
    int h  = volume.getHeight();
    int d  = volume.getDepth();
    int ch = volume.getChannels();
    resolveSlabBounds(d, zStart, zEnd);

    int slabDepth = zEnd - zStart + 1;
    int half = slabDepth / 2; // The index representing the middle element

    Image out(w, h, ch);


    // We allocate exactly one continuous block representing an entire row of 
    // histograms (width * 256 bins). We clear and reuse it for every Y row.
    // 16-bit integers save 50% memory compared to standard 32-bit ints, 
    // and since a volume depth rarely exceeds 65,535 slices, overflow is practically impossible.
    std::vector<uint16_t> hist(static_cast<size_t>(w) * 256, 0);

    for (int c = 0; c < ch; ++c) {
        for (int y = 0; y < h; ++y) {
            // Clear the shared histogram buffer for the start of this new row.
            std::fill(hist.begin(), hist.end(), static_cast<uint16_t>(0));

            // Build the histogram across the entire Z slab for every X in this Y row.
            // Notice Z is the outer loop here to maximize cache locality if data is stored by slice.
            for (int z = zStart; z <= zEnd; ++z) {
                for (int x = 0; x < w; ++x) {
                    unsigned char val = volume.getVoxel(x, y, z, c);
                    // Offset by `x * 256` to find the start of the specific histogram for pixel 'x'.
                    hist[static_cast<size_t>(x) * 256 + val]++;
                }
            }

            // Now that the histograms for this row are populated, walk each pixel's 
            // histogram to find the median intensity.
            for (int x = 0; x < w; ++x) {
                int count = 0;
                size_t base = static_cast<size_t>(x) * 256;
                for (int v = 0; v < 256; ++v) {
                    count += hist[base + v];
                    // Once we've accumulated more than half the total voxels in the ray, 
                    // we've found our median.
                    if (count > half) {
                        out.setPixel(x, y, c, static_cast<unsigned char>(v));
                        break;
                    }
                }
            }
        }
    }

    return out;
}