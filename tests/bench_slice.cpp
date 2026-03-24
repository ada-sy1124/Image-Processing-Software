/*
 * Benchmark: compare blurring a rotated SliceVolume (2.5D)
 * versus blurring the whole Volume (full 3D), then extracting a slice.
 *
 * For each (volume, filter, plane, coord) combination the function:
 *   1. Creates a SliceVolume copy, rotates it, applies the 3D blur with
 *      the coord hint, and extracts the slice.
 *   2. Clones the original volume, applies the full 3D blur (no coord),
 *      and extracts the matching slice via Slice::sliceXZ / sliceYZ.
 *   3. Compares the two resulting images pixel-by-pixel.
 *   4. Reports timing for both approaches.
 */

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "Image.h"
#include "Volume.h"
#include "Slice.h"
#include "SliceVolume.h"
#include "Filters3D.h"
#include "ConvolutionalFilter.h"

#include <chrono>
#include <iostream>
#include <cassert>
#include <cmath>
#include <string>
#include <memory>

// ---------------------------------------------------------------------------
// Helper: build a small synthetic volume (w x h x d) with a gradient pattern
// ---------------------------------------------------------------------------
static Volume makeSyntheticVolume(int w, int h, int d) {
    Volume vol(w, h, d);
    for (int z = 0; z < d; ++z)
        for (int y = 0; y < h; ++y)
            for (int x = 0; x < w; ++x)
                vol.setVoxel(x, y, z,
                    static_cast<unsigned char>((x + y + z) % 256));
    return vol;
}

// ---------------------------------------------------------------------------
// Helper: copy a Volume into a SliceVolume
// (SliceVolume inherits Volume but has no Volume->SliceVolume constructor,
//  so we copy the raw data.)
// ---------------------------------------------------------------------------
static SliceVolume toSliceVolume(const Volume& src) {
    int w = src.getWidth(), h = src.getHeight(), d = src.getDepth();
    SliceVolume sv;
    // Construct a Volume of the right size, then copy data
    static_cast<Volume&>(sv) = Volume(w, h, d);
    auto& dst = sv.getData();
    const auto& srcData = src.getData();
    dst = srcData;
    return sv;
}

// ---------------------------------------------------------------------------
// Helper: compare two images pixel-by-pixel, return max absolute difference
// ---------------------------------------------------------------------------
static int maxPixelDiff(const Image& a, const Image& b) {
    if (a.getWidth() != b.getWidth() || a.getHeight() != b.getHeight())
        return 999;
    int maxDiff = 0;
    for (int y = 0; y < a.getHeight(); ++y)
        for (int x = 0; x < a.getWidth(); ++x) {
            int diff = std::abs(static_cast<int>(a.getPixel(x, y, 0))
                              - static_cast<int>(b.getPixel(x, y, 0)));
            if (diff > maxDiff) maxDiff = diff;
        }
    return maxDiff;
}

// ---------------------------------------------------------------------------
// Core benchmark function
// ---------------------------------------------------------------------------
enum class FilterType { Gaussian, Median };

void benchSlice(const Volume& volume,
                FilterType filterType,
                int kernelSize,
                char plane,
                int coord)
{
    using Clock = std::chrono::high_resolution_clock;
    std::string filterName = (filterType == FilterType::Gaussian)
                             ? "Gaussian" : "Median";

    std::cout << "--- " << filterName << " k=" << kernelSize
              << "  plane=" << plane << "  coord=" << coord
              << "  vol=" << volume.getWidth() << "x"
              << volume.getHeight() << "x" << volume.getDepth()
              << " ---\n";

    // ---- Approach 1: SliceVolume (rotate + blur with coord + slice) ------
    SliceVolume sv = toSliceVolume(volume);
    sv.rotate(coord, kernelSize, plane);

    auto t0 = Clock::now();
    if (filterType == FilterType::Gaussian) {
        GaussianBlur3DFilter f(kernelSize, 1.0);  // full 3D on the small rotated volume
        f.apply(sv);
    } else {
        MedianBlur3DFilter f(kernelSize);          // full 3D on the small rotated volume
        f.apply(sv);
    }
    auto t1 = Clock::now();

    Image sliceFromSV = sv.slice(coord);

    auto usSlice = std::chrono::duration_cast<std::chrono::microseconds>(
                       t1 - t0).count();

    // ---- Approach 2: Full volume blur then extract slice ------------------
    std::unique_ptr<Volume> fullVol = volume.clone();

    auto t2 = Clock::now();
    if (filterType == FilterType::Gaussian) {
        GaussianBlur3DFilter f(kernelSize, 1.0);   // slice = false (full 3D)
        f.apply(*fullVol);
    } else {
        MedianBlur3DFilter f(kernelSize);           // slice = false
        f.apply(*fullVol);
    }
    auto t3 = Clock::now();

    Image sliceFromFull;
    if (plane == 'X') {
        sliceFromFull = Slice::slice(*fullVol, coord, 'X');
    } else {
        sliceFromFull = Slice::slice(*fullVol, coord, 'Y');
    }

    auto usFull = std::chrono::duration_cast<std::chrono::microseconds>(
                      t3 - t2).count();

    // ---- Compare results --------------------------------------------------
    int diff = maxPixelDiff(sliceFromSV, sliceFromFull);
    std::cout << "  SliceVolume blur : " << usSlice  << " us\n"
              << "  Full Volume blur : " << usFull   << " us\n"
              << "  Max pixel diff   : " << diff     << "\n";
    if (diff == 0)
        std::cout << "  Result: MATCH\n";
    else
        std::cout << "  Result: MISMATCH (max diff = " << diff << ")\n";
    std::cout << std::endl;
}

// ---------------------------------------------------------------------------
// main – run several combinations
// ---------------------------------------------------------------------------
int main() {
    // Create synthetic volumes of different sizes
    Volume small  = makeSyntheticVolume(16, 16, 16);
    Volume medium = makeSyntheticVolume(32, 32, 32);
    Volume large  = makeSyntheticVolume(64, 64, 64);
    Volume xlarge = makeSyntheticVolume(96, 96, 96);
    Volume huge   = makeSyntheticVolume(128, 128, 128);

    struct TestCase {
        const Volume& vol;
        std::string label;
        FilterType filter;
        int kernelSize;
        char plane;
        int coord;
    };

    TestCase cases[] = {
        // Small volume – Gaussian
        { small,  "small",  FilterType::Gaussian, 3, 'X', 8  },
        { small,  "small",  FilterType::Gaussian, 3, 'Y', 8  },
        { small,  "small",  FilterType::Gaussian, 5, 'X', 4  },
        // Small volume – Median
        { small,  "small",  FilterType::Median,   3, 'X', 8  },
        { small,  "small",  FilterType::Median,   3, 'Y', 8  },
        // Medium volume – Gaussian
        { medium, "medium", FilterType::Gaussian, 3, 'X', 16 },
        { medium, "medium", FilterType::Gaussian, 5, 'Y', 16 },
        { medium, "medium", FilterType::Gaussian, 7, 'X', 16 },
        // Medium volume – Median
        { medium, "medium", FilterType::Median,   3, 'X', 16 },
        { medium, "medium", FilterType::Median,   3, 'Y', 16 },
        { medium, "medium", FilterType::Median,   5, 'X', 16 },
        // Large volume – Gaussian
        { large,  "large",  FilterType::Gaussian, 3, 'X', 32 },
        { large,  "large",  FilterType::Gaussian, 3, 'Y', 32 },
        { large,  "large",  FilterType::Gaussian, 7, 'X', 32 },
        { large,  "large",  FilterType::Gaussian, 9, 'Y', 32 },
        // Large volume – Median
        { large,  "large",  FilterType::Median,   3, 'X', 32 },
        { large,  "large",  FilterType::Median,   3, 'Y', 32 },
        { large,  "large",  FilterType::Median,   5, 'X', 32 },
        // XLarge volume – Gaussian
        { xlarge, "xlarge", FilterType::Gaussian, 3, 'X', 48 },
        { xlarge, "xlarge", FilterType::Gaussian, 5, 'Y', 48 },
        { xlarge, "xlarge", FilterType::Gaussian, 7, 'X', 48 },
        // XLarge volume – Median
        { xlarge, "xlarge", FilterType::Median,   3, 'X', 48 },
        // Huge volume – Gaussian
        { huge,   "huge",   FilterType::Gaussian, 3, 'X', 64 },
        { huge,   "huge",   FilterType::Gaussian, 3, 'Y', 64 },
        { huge,   "huge",   FilterType::Gaussian, 5, 'X', 64 },
    };

    for (const auto& tc : cases) {
        benchSlice(tc.vol, tc.filter, tc.kernelSize, tc.plane, tc.coord);
    }

    std::cout << "All benchmarks complete.\n";
    return 0;
}
