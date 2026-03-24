#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

/**
 * test_histogram.cpp
 * Unit tests for EqualizeHistogram filter and related Image functions.
 */

#include "Image.h"
#include "SimpleFilter.h"

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
// Helper: compute histogram of a flat pixel buffer
// ============================================================
static std::vector<int> computeHistogram(const std::vector<unsigned char>& data) {
    std::vector<int> hist(256, 0);
    for (auto v : data) hist[v]++;
    return hist;
}

// Helper: compute median without std::sort (selection via partial manual sort)
static double computeMedian(std::vector<unsigned char> v) {
    int n = static_cast<int>(v.size());
    // Simple insertion sort (allowed — we write it ourselves)
    for (int i = 1; i < n; ++i) {
        unsigned char key = v[i];
        int j = i - 1;
        while (j >= 0 && v[j] > key) { v[j+1] = v[j]; --j; }
        v[j+1] = key;
    }
    if (n % 2 == 1) return v[n/2];
    return (v[n/2-1] + v[n/2]) / 2.0;
}

// Helper: extract only the V channel from a 3-ch image in HSV space
static std::vector<float> extractVChannel(const Image& img) {
    auto hsv = img.toHSV();
    std::vector<float> v;
    v.reserve(hsv.size());
    for (auto& px : hsv) v.push_back(px.v);
    return v;
}

// Helper: extract only the L channel from a 3-ch image in HSL space
static std::vector<float> extractLChannel(const Image& img) {
    auto hsl = img.toHSL();
    std::vector<float> l;
    l.reserve(hsl.size());
    for (auto& px : hsl) l.push_back(px.l);
    return l;
}

// ============================================================
// Build a synthetic test image with known pixel values
// ============================================================

/**
 * Creates a W×H RGB image where pixel intensity increases linearly
 * from 0 to 255 across all pixels (left-to-right, top-to-bottom).
 * All three channels are set to the same value (grey ramp).
 */
static Image makeRampImage(int W, int H) {
    Image img(W, H, 3);
    int N = W * H;
    auto& data = img.getData();
    for (int i = 0; i < N; ++i) {
        unsigned char val = static_cast<unsigned char>(i * 255 / (N - 1));
        data[3*i]   = val;
        data[3*i+1] = val;
        data[3*i+2] = val;
    }
    return img;
}

/**
 * Creates a W×H greyscale (1-channel) image where all pixels
 * have values concentrated in [50, 100] — a narrow histogram.
 */
static Image makeNarrowGreyscaleImage(int W, int H) {
    Image img(W, H, 1);
    int N = W * H;
    auto& data = img.getData();
    for (int i = 0; i < N; ++i) {
        data[i] = static_cast<unsigned char>(50 + (i % 51)); // values 50..100
    }
    return img;
}

/**
 * Creates a uniform grey image (all pixels = value).
 */
static Image makeUniformImage(int W, int H, int ch, unsigned char value) {
    Image img(W, H, ch);
    auto& data = img.getData();
    std::fill(data.begin(), data.end(), value);
    return img;
}

// ============================================================
// TEST 1: Image dimensions preserved after equalisation
// ============================================================
static void testDimensionsPreserved() {
    printSection("Dimensions preserved after equalisation");

    int W = 32, H = 24;
    Image imgHSV = makeRampImage(W, H);
    Image imgHSL = makeRampImage(W, H);

    EqualizeHistogram filterHSV("HSV");
    EqualizeHistogram filterHSL("HSL");

    filterHSV.apply(imgHSV);
    filterHSL.apply(imgHSL);

    CHECK(imgHSV.getWidth()    == W,  "HSV: width unchanged");
    CHECK(imgHSV.getHeight()   == H,  "HSV: height unchanged");
    CHECK(imgHSV.getChannels() == 3,  "HSV: channels unchanged (3)");

    CHECK(imgHSL.getWidth()    == W,  "HSL: width unchanged");
    CHECK(imgHSL.getHeight()   == H,  "HSL: height unchanged");
    CHECK(imgHSL.getChannels() == 3,  "HSL: channels unchanged (3)");
}

// ============================================================
// TEST 2: Greyscale equalisation — output spans full 0-255 range
// ============================================================
static void testGreyscaleFullRange() {
    printSection("Greyscale: output spans full 0-255 range");

    // Narrow input: values only in [50, 100]
    Image img = makeNarrowGreyscaleImage(64, 64);

    // Verify input really is narrow
    auto& inData = img.getData();
    unsigned char inMin = *std::min_element(inData.begin(), inData.end());
    unsigned char inMax = *std::max_element(inData.begin(), inData.end());
    CHECK(inMin == 50,  "Input min == 50 (narrow range confirmed)");
    CHECK(inMax == 100, "Input max == 100 (narrow range confirmed)");

    EqualizeHistogram filter("HSV");
    filter.apply(img);

    auto& outData = img.getData();
    unsigned char outMin = *std::min_element(outData.begin(), outData.end());
    unsigned char outMax = *std::max_element(outData.begin(), outData.end());

    CHECK(outMin <= 5,   "Output min <= 5 (stretched to near 0)");
    CHECK(outMax >= 250, "Output max >= 250 (stretched to near 255)");
}

// ============================================================
// TEST 3: Greyscale equalisation — median near 128
// ============================================================
static void testGreyscaleMedian() {
    printSection("Greyscale: output median near 128");

    Image img = makeNarrowGreyscaleImage(128, 128);
    EqualizeHistogram filter("HSV");
    filter.apply(img);

    double med = computeMedian(img.getData());
    std::cout << "  Output median: " << med << "\n";
    CHECK_NEAR(med, 128.0, 30.0, "Median is within 30 of 128");
}

// ============================================================
// TEST 4: RGB HSV equalisation — V channel spans full range
// ============================================================
static void testRGBHSVFullRange() {
    printSection("RGB HSV: V channel spans full range after equalisation");

    Image img = makeRampImage(64, 64);
    EqualizeHistogram filter("HSV");
    filter.apply(img);

    auto vChannel = extractVChannel(img);
    float vMin = *std::min_element(vChannel.begin(), vChannel.end());
    float vMax = *std::max_element(vChannel.begin(), vChannel.end());

    std::cout << "  V min: " << vMin << "  V max: " << vMax << "\n";
    CHECK(vMin <= 0.02f, "V channel min <= 0.02 (near 0)");
    CHECK(vMax >= 0.98f, "V channel max >= 0.98 (near 1)");
}

// ============================================================
// TEST 5: RGB HSL equalisation — L channel spans full range
// ============================================================
static void testRGBHSLFullRange() {
    printSection("RGB HSL: L channel spans full range after equalisation");

    Image img = makeRampImage(64, 64);
    EqualizeHistogram filter("HSL");
    filter.apply(img);

    auto lChannel = extractLChannel(img);
    float lMin = *std::min_element(lChannel.begin(), lChannel.end());
    float lMax = *std::max_element(lChannel.begin(), lChannel.end());

    std::cout << "  L min: " << lMin << "  L max: " << lMax << "\n";
    CHECK(lMin <= 0.02f, "L channel min <= 0.02 (near 0)");
    CHECK(lMax >= 0.98f, "L channel max >= 0.98 (near 1)");
}

// ============================================================
// TEST 6: Hue and saturation are preserved (only V/L changes)
// ============================================================
static void testHueSaturationPreserved() {
    printSection("RGB HSV: hue and saturation preserved");

    // Create a known-colour image: pure red (H=0, S=1, V=0.5)
    // R=127, G=0, B=0
    int W = 16, H = 16;
    Image img(W, H, 3);
    auto& data = img.getData();
    for (int i = 0; i < W*H; ++i) {
        data[3*i]   = static_cast<unsigned char>(50 + i % 50); // vary R slightly
        data[3*i+1] = 0;
        data[3*i+2] = 0;
    }

    // Record hue/saturation before
    auto hsvBefore = img.toHSV();

    EqualizeHistogram filter("HSV");
    filter.apply(img);

    auto hsvAfter = img.toHSV();

    bool hueOk = true, satOk = true;
    for (int i = 0; i < W*H; ++i) {
        if (std::abs(hsvBefore[i].h - hsvAfter[i].h) > 1.0f) hueOk = false;
        if (std::abs(hsvBefore[i].s - hsvAfter[i].s) > 0.02f) satOk = false;
    }
    CHECK(hueOk, "Hue (H) unchanged after HSV equalisation");
    CHECK(satOk, "Saturation (S) unchanged after HSV equalisation");
}

// ============================================================
// TEST 7: Uniform image stays uniform after equalisation
// ============================================================
static void testUniformImageUnchanged() {
    printSection("Uniform image: output is also uniform");

    // All pixels = 128 → after equalisation should still be uniform
    Image img = makeUniformImage(32, 32, 3, 128);
    EqualizeHistogram filter("HSV");
    filter.apply(img);

    auto& data = img.getData();
    unsigned char first = data[0];
    bool allSame = true;
    for (auto v : data) if (v != first) { allSame = false; break; }

    CHECK(allSame, "All pixels remain the same value after equalising uniform image");
}

// ============================================================
// TEST 8: Histogram CDF mapping correctness (manual verification)
// ============================================================
static void testCDFMapping() {
    printSection("Greyscale CDF mapping correctness");

    /**
     * Hand-computed expected output:
     * 4×1 greyscale image with pixels: [0, 85, 170, 255]
     * Histogram: each value appears once → hist[0]=1, hist[85]=1, hist[170]=1, hist[255]=1
     * CDF:  val=0  → cum=1 → map=1*255/4=63
     *       val=85 → cum=2 → map=2*255/4=127
     *       val=170→ cum=3 → map=3*255/4=191
     *       val=255→ cum=4 → map=4*255/4=255
     */
    Image img(4, 1, 1);
    auto& data = img.getData();
    data[0] = 0; data[1] = 85; data[2] = 170; data[3] = 255;

    EqualizeHistogram filter("HSV");
    filter.apply(img);

    std::cout << "  Output pixels: "
              << (int)data[0] << " " << (int)data[1] << " "
              << (int)data[2] << " " << (int)data[3] << "\n";

    CHECK(data[0] == 63,  "Pixel 0: 0   -> 63");
    CHECK(data[1] == 127, "Pixel 1: 85  -> 127");
    CHECK(data[2] == 191, "Pixel 2: 170 -> 191");
    CHECK(data[3] == 255, "Pixel 3: 255 -> 255");
}


// ============================================================
// TEST 10: Pixel count preserved (no pixels lost)
// ============================================================
static void testPixelCountPreserved() {
    printSection("Pixel count preserved after equalisation");

    int W = 50, H = 40;
    Image imgHSV = makeRampImage(W, H);
    Image imgHSL = makeRampImage(W, H);

    EqualizeHistogram filterHSV("HSV");
    EqualizeHistogram filterHSL("HSL");
    filterHSV.apply(imgHSV);
    filterHSL.apply(imgHSL);

    // Total pixels = sum of histogram bins
    auto histHSV = computeHistogram(imgHSV.getData());
    auto histHSL = computeHistogram(imgHSL.getData());

    int totalHSV = 0, totalHSL = 0;
    for (auto v : histHSV) totalHSV += v;
    for (auto v : histHSL) totalHSL += v;

    // Each pixel has 3 channels
    int expected = W * H * 3;
    CHECK(totalHSV == expected, "HSV: total pixel values preserved (" +
          std::to_string(totalHSV) + " == " + std::to_string(expected) + ")");
    CHECK(totalHSL == expected, "HSL: total pixel values preserved (" +
          std::to_string(totalHSL) + " == " + std::to_string(expected) + ")");
}

// ============================================================
// Main
// ============================================================
int main() {
    std::cout << "Running Histogram Equalisation Unit Tests\n";

    testDimensionsPreserved();
    testGreyscaleFullRange();
    testGreyscaleMedian();
    testRGBHSVFullRange();
    testRGBHSLFullRange();
    testHueSaturationPreserved();
    testUniformImageUnchanged();
    testCDFMapping();
    testPixelCountPreserved();

    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << "RESULTS: " << g_passed << " passed, "
              << g_failed << " failed, "
              << (g_passed + g_failed) << " total\n";

    return (g_failed == 0) ? 0 : 1;
}