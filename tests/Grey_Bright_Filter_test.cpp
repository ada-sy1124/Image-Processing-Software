/*
Unit tests for SimpleFilters: GreyscaleFilter and BrightnessFilter.
Compile via CMake with CTest integration.
 */

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "Image.h"
#include "SimpleFilter.h"

#include <iostream>
#include <cmath>
#include <cstring>
#include <cassert>
#include <filesystem>
#include <sstream>
#include <vector>
#include <algorithm>
#include <numeric>

static int g_passed = 0;
static int g_failed = 0;

#define TEST(name)                                                      \
    static void test_##name();                                          \
    static struct Register_##name {                                     \
        Register_##name() { testCases().push_back({#name, test_##name}); } \
    } reg_##name;                                                       \
    static void test_##name()

#define EXPECT_TRUE(expr)  do { if (!(expr))  { throw std::runtime_error( \
    std::string("EXPECT_TRUE failed: ") + #expr + "  (" + __FILE__ + ":" + std::to_string(__LINE__) + ")"); } } while(0)

#define EXPECT_FALSE(expr) EXPECT_TRUE(!(expr))

#define EXPECT_EQ(a, b)    do { if ((a) != (b)) { \
    std::ostringstream _os; _os << "EXPECT_EQ failed: " << #a << " == " << (a) << ", " << #b << " == " << (b) \
    << "  (" << __FILE__ << ":" << __LINE__ << ")"; throw std::runtime_error(_os.str()); } } while(0)

#define EXPECT_NEAR(a, b, eps) do { if (std::abs((a) - (b)) > (eps)) { \
    std::ostringstream _os; _os << "EXPECT_NEAR failed: " << #a << " == " << (a) << ", " << #b << " == " << (b) \
    << ", eps=" << (eps) << "  (" << __FILE__ << ":" << __LINE__ << ")"; throw std::runtime_error(_os.str()); } } while(0)

#define EXPECT_THROW(expr, exType) do { bool _caught = false; \
    try { expr; } catch (const exType&) { _caught = true; } \
    if (!_caught) { throw std::runtime_error( \
        std::string("EXPECT_THROW failed: ") + #expr + " did not throw " + #exType \
        + "  (" + __FILE__ + ":" + std::to_string(__LINE__) + ")"); } } while(0)

struct TestEntry { const char* name; void (*func)(); };
static std::vector<TestEntry>& testCases() {
    static std::vector<TestEntry> v;
    return v;
}

// Helper: locate project-root Images/ directory
static std::string imagesDir() {
    for (auto& candidate : {"Images", "../Images", "../../Images"}) {
        if (std::filesystem::is_directory(candidate)) return candidate;
    }
    return "Images";  // fallback
}

static std::string imgPath(const std::string& filename) {
    return imagesDir() + "/" + filename;
}

//  1. GREYSCALE FILTER TESTS

// 1.1 Greyscale on a known RGB pixel (luminance formula)
TEST(Greyscale_KnownRGBPixel) {
    // Create 1x1 RGB image with known colour
    Image img(1, 1, 3);
    img.setPixel(0, 0, 0, 100);  // R
    img.setPixel(0, 0, 1, 150);  // G
    img.setPixel(0, 0, 2, 200);  // B

    GreyscaleFilter filter;
    filter.apply(img);

    // After greyscale, image should be 1-channel
    EXPECT_EQ(img.getChannels(), 1);
    EXPECT_EQ(static_cast<int>(img.getData().size()), 1);

    // Standard luminance: 0.2126*R + 0.7152*G + 0.0722*B
    int expected = static_cast<int>(0.2126 * 100 + 0.7152 * 150 + 0.0722 * 200);
    EXPECT_NEAR(static_cast<int>(img.getPixel(0, 0, 0)), expected, 2);
}

// 1.2 Greyscale on pure white pixel
TEST(Greyscale_PureWhite) {
    Image img(1, 1, 3);
    img.setPixel(0, 0, 0, 255);
    img.setPixel(0, 0, 1, 255);
    img.setPixel(0, 0, 2, 255);

    GreyscaleFilter filter;
    filter.apply(img);

    EXPECT_EQ(img.getChannels(), 1);
    EXPECT_NEAR(static_cast<int>(img.getPixel(0, 0, 0)), 255, 1);
}

// 1.3 Greyscale on pure black pixel
TEST(Greyscale_PureBlack) {
    Image img(1, 1, 3);
    img.setPixel(0, 0, 0, 0);
    img.setPixel(0, 0, 1, 0);
    img.setPixel(0, 0, 2, 0);

    GreyscaleFilter filter;
    filter.apply(img);

    EXPECT_EQ(img.getChannels(), 1);
    EXPECT_EQ(img.getPixel(0, 0, 0), 0);
}

// 1.4 Greyscale on pure red — tests weighting
TEST(Greyscale_PureRed) {
    Image img(1, 1, 3);
    img.setPixel(0, 0, 0, 255);
    img.setPixel(0, 0, 1, 0);
    img.setPixel(0, 0, 2, 0);

    GreyscaleFilter filter;
    filter.apply(img);

    EXPECT_EQ(img.getChannels(), 1);
    // 0.2126 * 255 ≈ 54
    int expected = static_cast<int>(0.2126 * 255);
    EXPECT_NEAR(static_cast<int>(img.getPixel(0, 0, 0)), expected, 2);
}

// 1.5 Greyscale on pure green — highest weight
TEST(Greyscale_PureGreen) {
    Image img(1, 1, 3);
    img.setPixel(0, 0, 0, 0);
    img.setPixel(0, 0, 1, 255);
    img.setPixel(0, 0, 2, 0);

    GreyscaleFilter filter;
    filter.apply(img);

    EXPECT_EQ(img.getChannels(), 1);
    // 0.7152 * 255 ≈ 182
    int expected = static_cast<int>(0.7152 * 255);
    EXPECT_NEAR(static_cast<int>(img.getPixel(0, 0, 0)), expected, 2);
}

// 1.6 Greyscale on pure blue — lowest weight
TEST(Greyscale_PureBlue) {
    Image img(1, 1, 3);
    img.setPixel(0, 0, 0, 0);
    img.setPixel(0, 0, 1, 0);
    img.setPixel(0, 0, 2, 255);

    GreyscaleFilter filter;
    filter.apply(img);

    EXPECT_EQ(img.getChannels(), 1);
    // 0.0722 * 255 ≈ 18
    int expected = static_cast<int>(0.0722 * 255);
    EXPECT_NEAR(static_cast<int>(img.getPixel(0, 0, 0)), expected, 2);
}

// 1.7 Greyscale on a multi-pixel image — dimensions preserved
TEST(Greyscale_MultiPixelDimensions) {
    Image img(4, 3, 3);
    // Fill with a gradient
    for (int y = 0; y < 3; ++y)
        for (int x = 0; x < 4; ++x) {
            img.setPixel(x, y, 0, static_cast<unsigned char>(x * 50));
            img.setPixel(x, y, 1, static_cast<unsigned char>(y * 80));
            img.setPixel(x, y, 2, 100);
        }

    GreyscaleFilter filter;
    filter.apply(img);

    EXPECT_EQ(img.getWidth(), 4);
    EXPECT_EQ(img.getHeight(), 3);
    EXPECT_EQ(img.getChannels(), 1);
    EXPECT_EQ(static_cast<int>(img.getData().size()), 4 * 3);
}

// 1.8 Greyscale on uniform grey — value unchanged
TEST(Greyscale_UniformGrey) {
    Image img(2, 2, 3);
    for (int y = 0; y < 2; ++y)
        for (int x = 0; x < 2; ++x) {
            img.setPixel(x, y, 0, 128);
            img.setPixel(x, y, 1, 128);
            img.setPixel(x, y, 2, 128);
        }

    GreyscaleFilter filter;
    filter.apply(img);

    EXPECT_EQ(img.getChannels(), 1);
    for (int y = 0; y < 2; ++y)
        for (int x = 0; x < 2; ++x)
            EXPECT_NEAR(static_cast<int>(img.getPixel(x, y, 0)), 128, 1);
}

// 1.9 Greyscale on already-grayscale image (1-channel)
TEST(Greyscale_AlreadyGrayscale) {
    Image img(3, 3, 1);
    for (int y = 0; y < 3; ++y)
        for (int x = 0; x < 3; ++x)
            img.setPixel(x, y, 0, static_cast<unsigned char>(x * 30 + y * 30));

    auto origData = img.getData();

    GreyscaleFilter filter;
    filter.apply(img);

    // Should remain 1-channel, values unchanged (or at least not crash)
    EXPECT_EQ(img.getChannels(), 1);
    EXPECT_EQ(img.getWidth(), 3);
    EXPECT_EQ(img.getHeight(), 3);
}

// 1.10 Greyscale on real image file
TEST(Greyscale_RealImage) {
    std::string path = imgPath("stinkbug.png");
    if (!std::filesystem::exists(path)) {
        std::cerr << "  [SKIP] " << path << " not found\n";
        return;
    }
    auto imgPtr = Image::load(path);
    EXPECT_TRUE(imgPtr != nullptr);
    Image& img = *imgPtr;
    int origW = img.getWidth();
    int origH = img.getHeight();

    GreyscaleFilter filter;
    filter.apply(img);

    EXPECT_EQ(img.getWidth(), origW);
    EXPECT_EQ(img.getHeight(), origH);
    EXPECT_EQ(img.getChannels(), 1);
    EXPECT_EQ(static_cast<int>(img.getData().size()), origW * origH);
}

//  2. BRIGHTNESS FILTER TESTS

// 2.1 Brightness +50 on known pixel
TEST(Brightness_PositiveValue) {
    Image img(1, 1, 3);
    img.setPixel(0, 0, 0, 100);
    img.setPixel(0, 0, 1, 100);
    img.setPixel(0, 0, 2, 100);

    BrightnessFilter filter(50);
    filter.apply(img);

    EXPECT_EQ(img.getPixel(0, 0, 0), 150);
    EXPECT_EQ(img.getPixel(0, 0, 1), 150);
    EXPECT_EQ(img.getPixel(0, 0, 2), 150);
}

// 2.2 Brightness -50 on known pixel
TEST(Brightness_NegativeValue) {
    Image img(1, 1, 3);
    img.setPixel(0, 0, 0, 100);
    img.setPixel(0, 0, 1, 100);
    img.setPixel(0, 0, 2, 100);

    BrightnessFilter filter(-50);
    filter.apply(img);

    EXPECT_EQ(img.getPixel(0, 0, 0), 50);
    EXPECT_EQ(img.getPixel(0, 0, 1), 50);
    EXPECT_EQ(img.getPixel(0, 0, 2), 50);
}

// 2.3 Brightness clamping — doesn't exceed 255
TEST(Brightness_ClampUpper) {
    Image img(1, 1, 3);
    img.setPixel(0, 0, 0, 200);
    img.setPixel(0, 0, 1, 250);
    img.setPixel(0, 0, 2, 255);

    BrightnessFilter filter(100);
    filter.apply(img);

    EXPECT_EQ(img.getPixel(0, 0, 0), 255);
    EXPECT_EQ(img.getPixel(0, 0, 1), 255);
    EXPECT_EQ(img.getPixel(0, 0, 2), 255);
}

// 2.4 Brightness clamping — doesn't go below 0
TEST(Brightness_ClampLower) {
    Image img(1, 1, 3);
    img.setPixel(0, 0, 0, 30);
    img.setPixel(0, 0, 1, 10);
    img.setPixel(0, 0, 2, 0);

    BrightnessFilter filter(-50);
    filter.apply(img);

    EXPECT_EQ(img.getPixel(0, 0, 0), 0);
    EXPECT_EQ(img.getPixel(0, 0, 1), 0);
    EXPECT_EQ(img.getPixel(0, 0, 2), 0);
}

// 2.5 Brightness 0 — no change
TEST(Brightness_ZeroNoChange) {
    Image img(2, 2, 3);
    img.setPixel(0, 0, 0, 100);
    img.setPixel(0, 0, 1, 150);
    img.setPixel(0, 0, 2, 200);
    img.setPixel(1, 1, 0, 50);
    img.setPixel(1, 1, 1, 75);
    img.setPixel(1, 1, 2, 25);

    BrightnessFilter filter(0);
    filter.apply(img);

    EXPECT_EQ(img.getPixel(0, 0, 0), 100);
    EXPECT_EQ(img.getPixel(0, 0, 1), 150);
    EXPECT_EQ(img.getPixel(0, 0, 2), 200);
    EXPECT_EQ(img.getPixel(1, 1, 0), 50);
    EXPECT_EQ(img.getPixel(1, 1, 1), 75);
    EXPECT_EQ(img.getPixel(1, 1, 2), 25);
}

// 2.6 Brightness on multi-pixel image — all pixels affected
TEST(Brightness_MultiPixel) {
    Image img(3, 3, 3);
    for (int y = 0; y < 3; ++y)
        for (int x = 0; x < 3; ++x)
            for (int c = 0; c < 3; ++c)
                img.setPixel(x, y, c, 100);

    BrightnessFilter filter(30);
    filter.apply(img);

    for (int y = 0; y < 3; ++y)
        for (int x = 0; x < 3; ++x)
            for (int c = 0; c < 3; ++c)
                EXPECT_EQ(img.getPixel(x, y, c), 130);
}

// 2.7 Brightness on grayscale (1-channel) image
TEST(Brightness_Grayscale) {
    Image img(2, 2, 1);
    img.setPixel(0, 0, 0, 100);
    img.setPixel(1, 0, 0, 200);
    img.setPixel(0, 1, 0, 50);
    img.setPixel(1, 1, 0, 0);

    BrightnessFilter filter(25);
    filter.apply(img);

    EXPECT_EQ(img.getPixel(0, 0, 0), 125);
    EXPECT_EQ(img.getPixel(1, 0, 0), 225);
    EXPECT_EQ(img.getPixel(0, 1, 0), 75);
    EXPECT_EQ(img.getPixel(1, 1, 0), 25);
}

// 2.8 Brightness preserves image dimensions and channels
TEST(Brightness_PreservesDimensions) {
    Image img(10, 8, 3);
    BrightnessFilter filter(20);
    filter.apply(img);

    EXPECT_EQ(img.getWidth(), 10);
    EXPECT_EQ(img.getHeight(), 8);
    EXPECT_EQ(img.getChannels(), 3);
    EXPECT_EQ(static_cast<int>(img.getData().size()), 10 * 8 * 3);
}

// 2.9 Brightness with large positive value — all saturate
TEST(Brightness_LargePositive) {
    Image img(2, 2, 3);
    for (int y = 0; y < 2; ++y)
        for (int x = 0; x < 2; ++x)
            for (int c = 0; c < 3; ++c)
                img.setPixel(x, y, c, static_cast<unsigned char>(x * 50 + y * 30 + c * 10));

    BrightnessFilter filter(255);
    filter.apply(img);

    // All pixels should be clamped to 255
    for (int y = 0; y < 2; ++y)
        for (int x = 0; x < 2; ++x)
            for (int c = 0; c < 3; ++c)
                EXPECT_EQ(img.getPixel(x, y, c), 255);
}

// 2.10 Brightness with large negative value — all zero
TEST(Brightness_LargeNegative) {
    Image img(2, 2, 3);
    for (int y = 0; y < 2; ++y)
        for (int x = 0; x < 2; ++x)
            for (int c = 0; c < 3; ++c)
                img.setPixel(x, y, c, static_cast<unsigned char>(x * 50 + y * 30 + c * 10));

    BrightnessFilter filter(-255);
    filter.apply(img);

    // All pixels should be clamped to 0
    for (int y = 0; y < 2; ++y)
        for (int x = 0; x < 2; ++x)
            for (int c = 0; c < 3; ++c)
                EXPECT_EQ(img.getPixel(x, y, c), 0);
}

// 2.11 Brightness auto mode — adjusts to mean 128
TEST(Brightness_AutoMode) {
    // Create a dark image (mean 50)
    Image img(4, 4, 3);
    for (int y = 0; y < 4; ++y)
        for (int x = 0; x < 4; ++x)
            for (int c = 0; c < 3; ++c)
                img.setPixel(x, y, c, 50);

    BrightnessFilter filter;  // auto mode
    filter.apply(img);

    // After auto brightness, the mean should be closer to 128
    double sum = 0;
    const auto& data = img.getData();
    for (auto v : data) sum += v;
    double mean = sum / data.size();

    // The auto mode should have increased brightness
    EXPECT_TRUE(mean > 50);
    // Ideally close to 128, but allow some tolerance
    EXPECT_NEAR(mean, 128.0, 30.0);
}

// 2.12 Brightness auto mode on bright image — should decrease
TEST(Brightness_AutoModeBrightImage) {
    // Create a bright image (mean 220)
    Image img(4, 4, 3);
    for (int y = 0; y < 4; ++y)
        for (int x = 0; x < 4; ++x)
            for (int c = 0; c < 3; ++c)
                img.setPixel(x, y, c, 220);

    BrightnessFilter filter;  // auto mode
    filter.apply(img);

    // After auto brightness, the mean should be closer to 128
    double sum = 0;
    const auto& data = img.getData();
    for (auto v : data) sum += v;
    double mean = sum / data.size();

    // The auto mode should have decreased brightness
    EXPECT_TRUE(mean < 220);
    EXPECT_NEAR(mean, 128.0, 30.0);
}

// 2.13 Brightness on real image file
TEST(Brightness_RealImage) {
    std::string path = imgPath("stinkbug.png");
    if (!std::filesystem::exists(path)) {
        std::cerr << "  [SKIP] " << path << " not found\n";
        return;
    }
    auto imgPtr = Image::load(path);
    EXPECT_TRUE(imgPtr != nullptr);
    Image& img = *imgPtr;

    // Skip if image is not RGB (some filters may reject non-3-channel images)
    if (img.getChannels() != 3) {
        std::cerr << "  [SKIP] stinkbug.png is not RGB (" << img.getChannels() << " channels)\n";
        return;
    }

    int origW = img.getWidth();
    int origH = img.getHeight();

    BrightnessFilter filter(30);
    filter.apply(img);

    EXPECT_EQ(img.getWidth(), origW);
    EXPECT_EQ(img.getHeight(), origH);
    EXPECT_EQ(img.getChannels(), 3);
}

// 2.14 Greyscale then Brightness — filter chaining
TEST(Greyscale_Then_Brightness) {
    Image img(2, 2, 3);
    for (int y = 0; y < 2; ++y)
        for (int x = 0; x < 2; ++x) {
            img.setPixel(x, y, 0, 100);
            img.setPixel(x, y, 1, 100);
            img.setPixel(x, y, 2, 100);
        }

    GreyscaleFilter grey;
    grey.apply(img);
    EXPECT_EQ(img.getChannels(), 1);

    // Grey(100,100,100) = 100 (since uniform)
    unsigned char greyVal = img.getPixel(0, 0, 0);
    EXPECT_NEAR(static_cast<int>(greyVal), 100, 2);

    BrightnessFilter bright(50);
    bright.apply(img);

    // Should be ~150 after +50 brightness
    EXPECT_NEAR(static_cast<int>(img.getPixel(0, 0, 0)),
                static_cast<int>(greyVal) + 50, 2);
}

// main — run all registered tests
int main() {
    std::cout << "Running " << testCases().size()
              << " SimpleFilter unit tests...\n\n";

    for (auto& tc : testCases()) {
        std::cout << "  " << tc.name << " ... ";
        try {
            tc.func();
            std::cout << "PASSED\n";
            ++g_passed;
        } catch (const std::exception& e) {
            std::cout << "FAILED\n    " << e.what() << "\n";
            ++g_failed;
        }
    }

    std::cout << "\n========================================\n"
              << "Results: " << g_passed << " passed, " << g_failed << " failed\n"
              << "========================================\n";

    return g_failed > 0 ? 1 : 0;
}
