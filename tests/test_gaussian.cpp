#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

/**
 * test_gaussian.cpp
 * Unit tests for GaussianBlurFilter and related Image functions.
 */

#include "Image.h"
#include "ConvolutionalFilter.h"

#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>
#include <cassert>

// ============================================================
// Simple test framework
// ============================================================

static int g_passed = 0;
static int g_failed = 0;

#define CHECK(cond, msg) do { \
    if (cond) { \
        std::cout << "  [PASS] " << msg << "\n"; \
        ++g_passed; \
    } else { \
        std::cout << "  [FAIL] " << msg << "\n"; \
        ++g_failed; \
    } \
} while(0)

#define CHECK_NEAR(a, b, tol, msg) CHECK(std::abs((double)(a)-(double)(b)) <= (tol), msg)

static void printSection(const std::string& name) {
    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << "TEST: " << name << "\n";
    std::cout << std::string(60, '-') << "\n";
}

// ============================================================
// Helper functions
// ============================================================

static Image makeUniformImage(int width, int height, int channels, unsigned char value) {
    Image img(width, height, channels);
    auto& data = img.getData();
    std::fill(data.begin(), data.end(), value);
    return img;
}

static Image makeTestImage() {
    Image img(3, 3, 1);
    img.setPixel(0, 0, 0, 0);
    img.setPixel(1, 0, 0, 0);
    img.setPixel(2, 0, 0, 0);
    img.setPixel(0, 1, 0, 0);
    img.setPixel(1, 1, 0, 255);
    img.setPixel(2, 1, 0, 0);
    img.setPixel(0, 2, 0, 0);
    img.setPixel(1, 2, 0, 0);
    img.setPixel(2, 2, 0, 0);
    return img;
}

// ============================================================
// TEST 1: Image dimensions preserved after Gaussian blur
// ============================================================
static void testDimensionsPreserved() {
    printSection("Dimensions preserved after Gaussian blur");

    int width = 640, height = 480, channels = 3;
    Image img(width, height, channels);
    GaussianBlurFilter filter(5, 1.0);
    filter.apply(img);

    CHECK(img.getWidth() == width, "Width unchanged");
    CHECK(img.getHeight() == height, "Height unchanged");
    CHECK(img.getChannels() == channels, "Channels unchanged");
}

// ============================================================
// TEST 2: Uniform image remains unchanged after Gaussian blur
// ============================================================
static void testUniformImageUnchanged() {
    printSection("Uniform image remains unchanged after Gaussian blur");

    int width = 100, height = 100, channels = 3;
    unsigned char value = 128;
    Image img = makeUniformImage(width, height, channels, value);
    GaussianBlurFilter filter(5, 1.0);
    filter.apply(img);

    bool allEqual = true;
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            for (int c = 0; c < channels; ++c) {
                if (img.getPixel(x, y, c) != value) {
                    allEqual = false;
                    break;
                }
            }
        }
    }

    CHECK(allEqual, "All pixels remain unchanged");
}

// ============================================================
// TEST 3: Gaussian blur with specific kernel
// ============================================================
static void testSpecificKernel() {
    printSection("Gaussian blur with specific kernel");

    Image img = makeTestImage();
    GaussianBlurFilter filter(3, 1.0);
    filter.apply(img);

    const double tolerance = 1.0;
    CHECK_NEAR(img.getPixel(0, 0, 0), 19, tolerance, "Pixel (0, 0) blurred correctly");
    CHECK_NEAR(img.getPixel(1, 0, 0), 31, tolerance, "Pixel (1, 0) blurred correctly");
    CHECK_NEAR(img.getPixel(2, 0, 0), 19, tolerance, "Pixel (2, 0) blurred correctly");
    CHECK_NEAR(img.getPixel(0, 1, 0), 31, tolerance, "Pixel (0, 1) blurred correctly");
    CHECK_NEAR(img.getPixel(1, 1, 0), 52, tolerance, "Pixel (1, 1) blurred correctly");
    CHECK_NEAR(img.getPixel(2, 1, 0), 31, tolerance, "Pixel (2, 1) blurred correctly");
    CHECK_NEAR(img.getPixel(0, 2, 0), 19, tolerance, "Pixel (0, 2) blurred correctly");
    CHECK_NEAR(img.getPixel(1, 2, 0), 31, tolerance, "Pixel (1, 2) blurred correctly");
    CHECK_NEAR(img.getPixel(2, 2, 0), 19, tolerance, "Pixel (2, 2) blurred correctly");
}

// ============================================================
// Main
// ============================================================
int main() {
    std::cout << "Running Gaussian Blur Unit Tests\n";

    testDimensionsPreserved();
    testUniformImageUnchanged();
    testSpecificKernel();

    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << "RESULTS: " << g_passed << " passed, "
              << g_failed << " failed, "
              << (g_passed + g_failed) << " total\n";

    return (g_failed == 0) ? 0 : 1;
}