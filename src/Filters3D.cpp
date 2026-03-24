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
 
 * @file Filters3D.cpp
 * @brief Implementation of 3D volumetric image filters.
 * @details This file implements the methods for 3D separable Gaussian blur 
 * and 3D Median blur filters applied to volumetric datasets.
 */


#include <iostream>
#include <cmath>
#include <vector>
#include <memory>
#include <stdexcept>
#include <algorithm>
#include "Filters3D.h"
 
// GaussianBlur3DFilter
// helper function to calculate 1D Gaussian kernel for the given size and sigma
static std::vector<float> calculateGaussianKernel3D(int size, float sigma) {
    if (size <= 0 || size % 2 == 0) {
        throw std::invalid_argument("Gaussian3D: kernel size must be a positive odd integer");
    }
    if (sigma <= 0.0f) {
        throw std::invalid_argument("Gaussian3D: sigma must be positive");
    }

    std::vector<float> kernel(size);
    float sum = 0.0f;
    const int r = size / 2;

    // Build the 1D Gaussian curve. We center it around 0 using `i - r`.
    for (int i = 0; i < size; ++i) {
        int x = i - r;
        kernel[i] = std::exp(-0.5f * (x * x) / (sigma * sigma));
        sum += kernel[i];
    }

    // Normalize the kernel so that all elements sum to 1. 
    // If we don't normalize, applying the filter will artificially brighten or darken the volume.
    for (int i = 0; i < size; ++i) {
        kernel[i] /= sum;
    }

    return kernel;
}

void GaussianBlur3DFilter::apply(Volume& volume) {
    const int w = volume.getWidth();
    const int h = volume.getHeight();
    const int d = volume.getDepth();
    const int ch = volume.getChannels(); // 1 (gray) or 3 (RGB)

    if (w <= 0 || h <= 0 || d <= 0) {
        throw std::runtime_error("GaussianBlur3DFilter: empty volume");
    }
    if (!(ch == 1 || ch == 3)) {
        throw std::invalid_argument("GaussianBlur3DFilter: only 1 or 3 channels are supported");
    }
    if (size <= 0 || size % 2 == 0) {
        throw std::invalid_argument("GaussianBlur3DFilter: kernel size must be a positive odd integer");
    }
    if (stdev <= 0.0) {
        throw std::invalid_argument("GaussianBlur3DFilter: standard deviation must be positive");
    }
    if (size == 1) {
        return; // no blur
    }

    const int r = size / 2;
    const std::vector<float> kernel = calculateGaussianKernel3D(size, static_cast<float>(stdev));

    const size_t voxels = static_cast<size_t>(w) * h * d;
    const size_t N = voxels * static_cast<size_t>(ch);

    // A true 3D convolution with a size K kernel takes O(K^3) operations per voxel.
    // By separating it into three 1D passes (X, then Y, then Z), we reduce this to O(3K) per voxel.
    // This requires intermediate buffers (tempX, tempY) to hold the float precision results between passes.
    std::vector<float> tempX(N, 0.0f);
    std::vector<float> tempY(N, 0.0f);
    std::vector<unsigned char> out(N, 0);

    // Lambda to calculate flat 1D array index from 3D coordinates.
    // It encapsulates the repetitive indexing math to prevent typos and keep the inner loops clean.
    // interleaved layout: [v0.R, v0.G, v0.B, v1.R, v1.G, v1.B, ...]
    auto idx = [w, h, ch](int x, int y, int z, int c) -> size_t {
        return (((static_cast<size_t>(z) * h + y) * w + x) * ch) + static_cast<size_t>(c);
    };

    // Snapshot source data so we have a clean read-only reference
    const std::vector<unsigned char> src = volume.getData();

    // Clamping helper for edge cases
    auto srcAtClamped = [&](int x, int y, int z, int c) -> float {
        x = std::clamp(x, 0, w - 1);
        y = std::clamp(y, 0, h - 1);
        z = std::clamp(z, 0, d - 1);
        return static_cast<float>(src[idx(x, y, z, c)]);
    };

    // 1) Convolve along X axis
    for (int z = 0; z < d; ++z) {
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                for (int c = 0; c < ch; ++c) {
                    float sum = 0.0f;
                    // Branching inside the innermost loop is expensive.
                    // By checking if we are safely inside the volume bounds (`x >= r && x < w - r`), 
                    // we can do fast, direct memory access for 95%+ of the volume and only use the 
                    // slower clamping logic on the outermost edges.
                    if (x >= r && x < w - r) {
                        // interior = direct array access, contiguous in X (stride = ch)
                        size_t start = idx(x - r, y, z, c);
                        for (int k = 0; k < size; ++k) {
                            sum += static_cast<float>(src[start + static_cast<size_t>(k) * ch]) * kernel[k];
                        }
                    } else {
                        // border = handle edge clamping
                        for (int k = -r; k <= r; ++k) {
                            sum += srcAtClamped(x + k, y, z, c) * kernel[k + r];
                        }
                    }
                    tempX[idx(x, y, z, c)] = sum;
                }
            }
        }
    }

    auto tempXAtClamped = [&](int x, int y, int z, int c) -> float {
        x = std::clamp(x, 0, w - 1);
        y = std::clamp(y, 0, h - 1);
        z = std::clamp(z, 0, d - 1);
        return tempX[idx(x, y, z, c)];
    };

    // 2) Convolve along Y axis (reading from tempX results)
    for (int z = 0; z < d; ++z) {
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                for (int c = 0; c < ch; ++c) {
                    float sum = 0.0f;
                    if (y >= r && y < h - r) {
                        // move between rows by jumping width * channels
                        size_t base = idx(x, y, z, c);
                        for (int k = -r; k <= r; ++k) {
                            sum += tempX[base + static_cast<ptrdiff_t>(k) * w * ch] * kernel[k + r];
                        }
                    } else {
                        for (int k = -r; k <= r; ++k) {
                            sum += tempXAtClamped(x, y + k, z, c) * kernel[k + r];
                        }
                    }
                    tempY[idx(x, y, z, c)] = sum;
                }
            }
        }
    }

    auto tempYAtClamped = [&](int x, int y, int z, int c) -> float {
        x = std::clamp(x, 0, w - 1);
        y = std::clamp(y, 0, h - 1);
        z = std::clamp(z, 0, d - 1);
        return tempY[idx(x, y, z, c)];
    };

    // 3) Convolve along Z axis (reading from tempY results), then clamp back to 8-bit [0,255]
    for (int z = 0; z < d; ++z) {
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                for (int c = 0; c < ch; ++c) {
                    float sum = 0.0f;
                    if (z >= r && z < d - r) {
                        // move between slices by jumping width * height * channels
                        size_t base = idx(x, y, z, c);
                        for (int k = -r; k <= r; ++k) {
                            sum += tempY[base + static_cast<ptrdiff_t>(k) * w * h * ch] * kernel[k + r];
                        }
                    } else {
                        for (int k = -r; k <= r; ++k) {
                            sum += tempYAtClamped(x, y, z + k, c) * kernel[k + r];
                        }
                    }

                    // Convert float back to unsigned char, clamping to ensure valid color ranges
                    int v = static_cast<int>(std::round(sum));
                    v = std::clamp(v, 0, 255);
                    out[idx(x, y, z, c)] = static_cast<unsigned char>(v);
                }
            }
        }
    }

    // Move semantics transfer ownership without an expensive deep copy
    volume.getData() = std::move(out);
}

// MedianBlur3DFilter
MedianBlur3DFilter::MedianBlur3DFilter(int size) : size(size) {
    if (size <= 0) {
        throw std::invalid_argument("MedianBlur3DFilter: kernel size must be > 0");
    }
    if (size % 2 == 0) {
        throw std::invalid_argument("MedianBlur3DFilter: kernel size must be odd (3,5,7,...)");
    }
}

void MedianBlur3DFilter::apply(Volume& volume) {
    const int w = volume.getWidth();
    const int h = volume.getHeight();
    const int d = volume.getDepth();
    const int ch = volume.getChannels(); // 1 (gray) or 3 (RGB)

    if (w <= 0 || h <= 0 || d <= 0) {
        throw std::runtime_error("MedianBlur3DFilter: empty volume");
    }
    if (!(ch == 1 || ch == 3)) {
        throw std::invalid_argument("MedianBlur3DFilter: only 1 or 3 channels are supported");
    }

    const int radius = size / 2;
    const int total = size * size * size;  // fixed 3D neighborhood size
    const int medianPos = total / 2;       // Index of the median element

    // Median filtering requires reading the original unedited neighborhood.
    // If we wrote the median values directly into the source volume as we loop, 
    // the next voxel over would read already-blurred data, corrupting the result.
    std::unique_ptr<Volume> src = volume.clone();
    const std::vector<unsigned char>& srcData = src->getData();

    // interleaved layout: [v0.R, v0.G, v0.B, v1.R, v1.G, v1.B, ...]
    auto idx = [w, h, ch](int x, int y, int z, int c) -> size_t {
        return (((static_cast<size_t>(z) * h + y) * w + x) * ch) + static_cast<size_t>(c);
    };

    auto getVoxelClamped = [&](int x, int y, int z, int c) -> unsigned char {
        x = std::clamp(x, 0, w - 1);
        y = std::clamp(y, 0, h - 1);
        z = std::clamp(z, 0, d - 1);
        return srcData[idx(x, y, z, c)];
    };

    for (int z = 0; z < d; ++z) {
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                // Again, separate interior vs border logic to speed up memory access
                const bool interior = (x >= radius && x < w - radius &&
                                       y >= radius && y < h - radius &&
                                       z >= radius && z < d - radius);

                // process each channel independently (R/G/B or single gray)
                for (int c = 0; c < ch; ++c) {
                    
                    // Sorting a 3D neighborhood array (e.g., 5x5x5 = 125 elements) 
                    // for every single voxel is extremely slow. Since our pixel values are bounded 
                    // to exactly 256 possible values (0-255), we can use a bucketed histogram.
                    // Counting occurrences and scanning 256 bins is much faster and O(1) regarding kernel size.
                    int hist[256] = {0};

                    if (interior) {
                        // fast path: direct array access, no bounds checks
                        for (int kz = -radius; kz <= radius; ++kz) {
                            for (int ky = -radius; ky <= radius; ++ky) {
                                size_t start = idx(x - radius, y + ky, z + kz, c);
                                for (int kx = 0; kx < size; ++kx) {
                                    unsigned char val = srcData[start + static_cast<size_t>(kx) * ch];
                                    hist[val]++; // Increment bucket for this intensity
                                }
                            }
                        }
                    } else {
                        // border path: clamp coordinates
                        for (int kz = -radius; kz <= radius; ++kz) {
                            for (int ky = -radius; ky <= radius; ++ky) {
                                for (int kx = -radius; kx <= radius; ++kx) {
                                    unsigned char val = getVoxelClamped(x + kx, y + ky, z + kz, c);
                                    hist[val]++; // Increment bucket for this intensity
                                }
                            }
                        }
                    }

                    // scan histogram from 0 to 255 to find the median intensity
                    // We stop when our cumulative count passes the halfway point of total neighborhood pixels
                    int cumulative = 0;
                    unsigned char median = 0;
                    for (int b = 0; b < 256; ++b) {
                        cumulative += hist[b];
                        if (cumulative > medianPos) {
                            median = static_cast<unsigned char>(b);
                            break;
                        }
                    }

                    // write channel result directly to the original volume
                    volume.getData()[idx(x, y, z, c)] = median;
                }
            }
        }
    }
}