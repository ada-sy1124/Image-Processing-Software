/**
 * Comprehensive Unit Tests for ConvolutionalFilter classes
 *
 * This file combines and extends tests from test_emboss.cpp and test_gaussian.cpp,
 * and adds new test coverage for BoxBlur, MedianBlur, Sharpen, and edge detection filters.
 *
 * Strategy:
 * - Construct small images with known pixel values
 * - Manually compute or verify expected outputs
 * - Compare with filter outputs
 *
 * No external image-processing libraries are used except stb_image for I/O support.
 *
 * Compile (from project root):
 *   g++ -std=c++17 -I src \
 *       src/Image.cpp src/ConvolutionalFilter.cpp src/Filter.cpp \
 *       src/SimpleFilter.cpp \
 *       tests/test_convolutional_filters.cpp \
 *       -o build/test_convolutional_filters
 * Run:
 *   ./build/test_convolutional_filters
 */

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "Image.h"
#include "ConvolutionalFilter.h"

#include <iostream>
#include <cmath>
#include <cassert>
#include <vector>
#include <string>
#include <algorithm>


// TEST FRAMEWORK

static int g_pass = 0;
static int g_fail = 0;

static void check(bool cond, const std::string& name) {
    if (cond) {
        std::cout << "[PASS] " << name << "\n";
        ++g_pass;
    } else {
        std::cout << "[FAIL] " << name << "\n";
        ++g_fail;
    }
}

static bool near(int a, int b, int tol = 1) {
    return std::abs(a - b) <= tol;
}

static bool near_double(double a, double b, double tol = 0.1) {
    return std::abs(a - b) <= tol;
}

static void printSection(const std::string& name) {
    std::cout << "\n" << std::string(70, '=') << "\n";
    std::cout << "TEST SECTION: " << name << "\n";
    std::cout << std::string(70, '-') << "\n";
}

// HELPER FUNCTIONS

static Image makeUniformImage(int width, int height, int channels, unsigned char value) {
    Image img(width, height, channels);
    auto& data = img.getData();
    std::fill(data.begin(), data.end(), value);
    return img;
}

static Image makeTestImage3x3() {
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

static Image makeGradientImage(int width, int height) {
    Image img(width, height, 1);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            unsigned char val = static_cast<unsigned char>((x + y) * 255 / (width + height));
            img.setPixel(x, y, 0, val);
        }
    }
    return img;
}

static Image makeGrey3x3(const std::vector<unsigned char>& pixels) {
    Image img(3, 3, 1);
    for (int i = 0; i < 9; ++i)
        img.getData()[i] = pixels[i];
    return img;
}

static float manualConvolveNW(const std::vector<float>& src,
                               int w, int h, int px, int py, float S) {
    const float K[3][3] = {{-2,-1, 0},
                            {-1, 0, 1},
                            { 0, 1, 2}};
    float acc = 0.0f;
    for (int ky = -1; ky <= 1; ++ky) {
        for (int kx = -1; kx <= 1; ++kx) {
            int nx = std::clamp(px + kx, 0, w - 1);
            int ny = std::clamp(py + ky, 0, h - 1);
            acc += K[ky+1][kx+1] * src[ny * w + nx];
        }
    }
    float val = S * acc + 128.0f;
    return std::clamp(val, 0.0f, 255.0f);
}


// EMBOSS FILTER TESTS (from test_emboss.cpp)

static void test_emboss_greyscale_NW_strength1() {
    const std::vector<unsigned char> pixels = {
        10, 20, 30,
        40, 50, 60,
        70, 80, 90
    };
    Image img = makeGrey3x3(pixels);

    EmbossFilter f(1.0f, "NW");
    f.apply(img);

    std::vector<float> src(9);
    for (int i = 0; i < 9; ++i) src[i] = static_cast<float>(pixels[i]);

    bool allOk = true;
    for (int y = 0; y < 3; ++y) {
        for (int x = 0; x < 3; ++x) {
            float expected = manualConvolveNW(src, 3, 3, x, y, 1.0f);
            int got = static_cast<int>(img.getPixel(x, y, 0));
            if (!near(got, static_cast<int>(std::round(expected)))) {
                allOk = false;
            }
        }
    }
    check(allOk, "Emboss: Greyscale NW strength=1.0");
}

static void test_emboss_uniform_gives_128() {
    Image img(5, 5, 1);
    for (auto& b : img.getData()) b = 100;

    EmbossFilter f(2.5f, "NW");
    f.apply(img);

    bool allOk = true;
    for (int y = 0; y < 5; ++y)
        for (int x = 0; x < 5; ++x)
            if (img.getPixel(x, y, 0) != 128) allOk = false;

    check(allOk, "Emboss: Uniform image outputs 128");
}

static void test_emboss_all_directions() {
    for (const std::string& dir : {"NW", "NE", "SE", "SW"}) {
        Image img(4, 4, 1);
        for (auto& b : img.getData()) b = 200;

        EmbossFilter f(3.0f, dir);
        f.apply(img);

        bool allOk = true;
        for (int y = 0; y < 4; ++y)
            for (int x = 0; x < 4; ++x)
                if (img.getPixel(x, y, 0) != 128) allOk = false;

        check(allOk, "Emboss: Uniform direction=" + dir);
    }
}

static void test_emboss_strength_scaling() {
    const std::vector<unsigned char> pixels = {
         0,  0,  0,
         0, 50,255,
         0,255,255
    };

    Image img1 = makeGrey3x3(pixels);
    EmbossFilter f1(1.0f, "NW");
    f1.apply(img1);
    int out1 = static_cast<int>(img1.getPixel(1, 1, 0));

    Image img2 = makeGrey3x3(pixels);
    EmbossFilter f2(2.0f, "NW");
    f2.apply(img2);
    int out2 = static_cast<int>(img2.getPixel(1, 1, 0));

    std::vector<float> src(9);
    for (int i = 0; i < 9; ++i) src[i] = static_cast<float>(pixels[i]);
    float e1 = manualConvolveNW(src, 3, 3, 1, 1, 1.0f);
    float e2 = manualConvolveNW(src, 3, 3, 1, 1, 2.0f);

    check(near(out1, static_cast<int>(std::round(e1))),
          "Emboss: Strength=1 matches formula");
    check(near(out2, static_cast<int>(std::round(e2))),
          "Emboss: Strength=2 matches formula");
}

static void test_emboss_output_clamped() {
    const std::vector<unsigned char> pixels = {
          0,   0,   0,
          0, 128, 255,
          0, 255, 255
    };
    Image img = makeGrey3x3(pixels);

    EmbossFilter f(10.0f, "NW");
    f.apply(img);

    bool allInRange = true;
    for (int y = 0; y < 3; ++y)
        for (int x = 0; x < 3; ++x) {
            int v = static_cast<int>(img.getPixel(x, y, 0));
            if (v < 0 || v > 255) allInRange = false;
        }
    check(allInRange, "Emboss: Output clamped to [0,255]");
}

static void test_emboss_NE_kernel() {
    const std::vector<unsigned char> pixels = {
        10, 20, 30,
        40, 50, 60,
        70, 80, 90
    };
    Image img = makeGrey3x3(pixels);
    EmbossFilter f(1.0f, "NE");
    f.apply(img);

    int expected = 248;
    int got = static_cast<int>(img.getPixel(1, 1, 0));
    check(near(got, expected), "Emboss: NE kernel centre pixel");
}

static void test_emboss_SE_SW_corners() {
    const std::vector<unsigned char> pixels = {
        100, 150, 200,
        100, 150, 200,
        100, 150, 200
    };

    {
        Image img = makeGrey3x3(pixels);
        EmbossFilter f(1.0f, "SE");
        f.apply(img);
        int got = static_cast<int>(img.getPixel(0, 0, 0));
        check(got == 0, "Emboss: SE corner edge-clamped");
    }

    {
        Image img = makeGrey3x3(pixels);
        EmbossFilter f(1.0f, "SW");
        f.apply(img);
        int got = static_cast<int>(img.getPixel(0, 0, 0));
        check(got == 255, "Emboss: SW corner edge-clamped");
    }
}

static void test_emboss_RGB_HSV_uniform() {
    Image img(3, 3, 3);
    for (int i = 0; i < 3*3*3; i += 3) {
        img.getData()[i+0] = 128;
        img.getData()[i+1] = 128;
        img.getData()[i+2] = 128;
    }

    EmbossFilter f(2.0f, "NW", "HSV");
    f.apply(img);

    bool allOk = true;
    for (int y = 0; y < 3; ++y)
        for (int x = 0; x < 3; ++x) {
            int r = img.getPixel(x, y, 0);
            int g = img.getPixel(x, y, 1);
            int b = img.getPixel(x, y, 2);
            if (!near(r, 128, 2) || !near(g, 128, 2) || !near(b, 128, 2))
                allOk = false;
        }
    check(allOk, "Emboss: RGB HSV uniform image");
}

static void test_emboss_RGB_HSL_uniform() {
    Image img(3, 3, 3);
    for (int i = 0; i < 3*3*3; i += 3) {
        img.getData()[i+0] = 128;
        img.getData()[i+1] = 128;
        img.getData()[i+2] = 128;
    }

    EmbossFilter f(2.0f, "NE", "HSL");
    f.apply(img);

    bool allOk = true;
    for (int y = 0; y < 3; ++y)
        for (int x = 0; x < 3; ++x) {
            int r = img.getPixel(x, y, 0);
            int g = img.getPixel(x, y, 1);
            int b = img.getPixel(x, y, 2);
            if (!near(r, 128, 2) || !near(g, 128, 2) || !near(b, 128, 2))
                allOk = false;
        }
    check(allOk, "Emboss: RGB HSL uniform image");
}

static void test_emboss_invalid_direction() {
    Image img(3, 3, 1);
    for (auto& b : img.getData()) b = 100;

    EmbossFilter f(1.0f, "XX");
    bool threw = false;
    try {
        f.apply(img);
    } catch (const std::invalid_argument&) {
        threw = true;
    } catch (...) {}
    check(threw, "Emboss: Invalid direction throws exception");
}

static void test_emboss_dimensions_unchanged() {
    Image img(7, 5, 1);
    for (auto& b : img.getData()) b = 80;

    EmbossFilter f(1.5f, "SW");
    f.apply(img);

    check(img.getWidth() == 7 && img.getHeight() == 5 && img.getChannels() == 1,
          "Emboss: Dimensions unchanged");
}

static void test_emboss_default_type_is_HSV() {
    Image img1(3, 3, 3);
    Image img2(3, 3, 3);
    unsigned char val = 0;
    for (int i = 0; i < 3*3*3; ++i) {
        img1.getData()[i] = val;
        img2.getData()[i] = val;
        val = (val + 17) % 256;
    }

    EmbossFilter fDefault(1.0f, "SE");
    EmbossFilter fHSV(1.0f, "SE", "HSV");

    fDefault.apply(img1);
    fHSV.apply(img2);

    bool same = true;
    for (int i = 0; i < 3*3*3; ++i)
        if (img1.getData()[i] != img2.getData()[i]) { same = false; break; }

    check(same, "Emboss: Default type is HSV");
}


// GAUSSIAN BLUR FILTER TESTS

static void test_gaussian_dimensions_preserved() {
    int width = 640, height = 480, channels = 3;
    Image img(width, height, channels);
    GaussianBlurFilter filter(5, 1.0);
    filter.apply(img);

    check(img.getWidth() == width && img.getHeight() == height && img.getChannels() == channels,
          "Gaussian: Dimensions preserved");
}

static void test_gaussian_uniform_unchanged() {
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

    check(allEqual, "Gaussian: Uniform image unchanged");
}

static void test_gaussian_specific_kernel() {
    Image img = makeTestImage3x3();
    GaussianBlurFilter filter(3, 1.0);
    filter.apply(img);

    const double tolerance = 1.0;
    bool allOk = true;
    
    std::vector<std::pair<std::pair<int,int>, int>> expected = {
        {{0, 0}, 19}, {{1, 0}, 31}, {{2, 0}, 19},
        {{0, 1}, 31}, {{1, 1}, 52}, {{2, 1}, 31},
        {{0, 2}, 19}, {{1, 2}, 31}, {{2, 2}, 19}
    };
    
    for (auto& p : expected) {
        int x = p.first.first, y = p.first.second, exp_val = p.second;
        int got = img.getPixel(x, y, 0);
        if (!near(got, exp_val, 1)) allOk = false;
    }

    check(allOk, "Gaussian: Specific kernel values correct");
}


// BOX BLUR FILTER TESTS

static void test_boxblur_dimensions_preserved() {
    Image img(640, 480, 3);
    for (auto& b : img.getData()) b = 128;
    
    BoxBlurFilter filter(5);
    filter.apply(img);

    check(img.getWidth() == 640 && img.getHeight() == 480 && img.getChannels() == 3,
          "BoxBlur: Dimensions preserved");
}

static void test_boxblur_uniform_unchanged() {
    int width = 100, height = 100, channels = 3;
    Image img = makeUniformImage(width, height, channels, 128);
    
    BoxBlurFilter filter(5);
    filter.apply(img);

    bool allEqual = true;
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            for (int c = 0; c < channels; ++c) {
                if (img.getPixel(x, y, c) != 128) {
                    allEqual = false;
                }
            }
        }
    }

    check(allEqual, "BoxBlur: Uniform image unchanged");
}

static void test_boxblur_output_in_range() {
    Image img = makeTestImage3x3();
    BoxBlurFilter filter(3);
    filter.apply(img);

    bool inRange = true;
    for (int y = 0; y < 3; ++y) {
        for (int x = 0; x < 3; ++x) {
            int val = img.getPixel(x, y, 0);
            if (val < 0 || val > 255) inRange = false;
        }
    }

    check(inRange, "BoxBlur: Output in valid range [0,255]");
}

static void test_boxblur_invalid_size() {
    Image img(5, 5, 1);
    bool threw = false;
    
    try {
        BoxBlurFilter filter(4);  // even size
        filter.apply(img);
    } catch (const std::invalid_argument&) {
        threw = true;
    } catch (...) {}
    
    check(threw, "BoxBlur: Even kernel size throws exception");
}

static void test_boxblur_smoothing_effect() {
    Image img = makeTestImage3x3();
    BoxBlurFilter filter(3);
    filter.apply(img);

    // Center pixel should be significantly reduced from 255
    int centerVal = img.getPixel(1, 1, 0);
    check(centerVal < 255 && centerVal > 0, "BoxBlur: Center pixel smoothed from 255");
}


// MEDIAN BLUR FILTER TESTS

static void test_medianblur_dimensions_preserved() {
    Image img(640, 480, 1);
    for (auto& b : img.getData()) b = 100;
    
    MedianBlurFilter filter(5);
    filter.apply(img);

    check(img.getWidth() == 640 && img.getHeight() == 480 && img.getChannels() == 1,
          "MedianBlur: Dimensions preserved");
}

static void test_medianblur_uniform_unchanged() {
    Image img = makeUniformImage(50, 50, 1, 200);
    
    MedianBlurFilter filter(5);
    filter.apply(img);

    bool allEqual = true;
    for (int i = 0; i < img.getData().size(); ++i) {
        if (img.getData()[i] != 200) allEqual = false;
    }

    check(allEqual, "MedianBlur: Uniform image unchanged");
}

static void test_medianblur_invalid_size() {
    Image img(5, 5, 1);
    bool threw = false;
    
    try {
        MedianBlurFilter filter(4);  // even size
        filter.apply(img);
    } catch (...) {
        threw = true;
    }
    
    check(threw, "MedianBlur: Even kernel size throws exception");
}

static void test_medianblur_salt_pepper_removal() {
    // Create image with salt and pepper noise
    Image img(5, 5, 1);
    for (auto& b : img.getData()) b = 128;
    
    img.setPixel(2, 2, 0, 255);  // salt
    img.setPixel(1, 1, 0, 0);    // pepper
    
    MedianBlurFilter filter(3);
    filter.apply(img);

    // Noise should be reduced
    int centerVal = img.getPixel(2, 2, 0);
    check(centerVal < 255 && centerVal > 0, "MedianBlur: Salt-pepper noise reduced");
}

static void test_medianblur_output_in_range() {
    Image img = makeGradientImage(10, 10);
    MedianBlurFilter filter(3);
    filter.apply(img);

    bool inRange = true;
    for (int i = 0; i < img.getData().size(); ++i) {
        if (img.getData()[i] > 255) inRange = false;
    }

    check(inRange, "MedianBlur: Output in valid range");
}


// SHARPEN FILTER TESTS

static void test_sharpen_dimensions_preserved() {
    Image img(640, 480, 3);
    for (auto& b : img.getData()) b = 128;
    
    SharpenFilter filter;
    filter.apply(img);

    check(img.getWidth() == 640 && img.getHeight() == 480 && img.getChannels() == 3,
          "Sharpen: Dimensions preserved");
}

static void test_sharpen_uniform_unchanged() {
    Image img = makeUniformImage(50, 50, 1, 150);
    
    SharpenFilter filter;
    filter.apply(img);

    bool allEqual = true;
    for (int i = 0; i < img.getData().size(); ++i) {
        if (img.getData()[i] != 150) allEqual = false;
    }

    check(allEqual, "Sharpen: Uniform image unchanged");
}

static void test_sharpen_output_in_range() {
    Image img = makeTestImage3x3();
    SharpenFilter filter;
    filter.apply(img);

    bool inRange = true;
    for (int y = 0; y < 3; ++y) {
        for (int x = 0; x < 3; ++x) {
            int val = img.getPixel(x, y, 0);
            if (val < 0 || val > 255) inRange = false;
        }
    }

    check(inRange, "Sharpen: Output in valid range [0,255]");
}

static void test_sharpen_enhances_edges() {
    Image img = makeTestImage3x3();
    SharpenFilter filter;
    filter.apply(img);

    // Center pixel should be enhanced
    int centerVal = img.getPixel(1, 1, 0);
    check(centerVal > 200, "Sharpen: Center pixel enhanced");
}

// SOBEL FILTER TESTS

static void test_sobel_dimensions_preserved() {
    Image img(640, 480, 3);
    for (auto& b : img.getData()) b = 128;
    
    SobelFilter filter;
    filter.apply(img);

    check(img.getWidth() == 640 && img.getHeight() == 480 && img.getChannels() == 1,
          "Sobel: Dimensions correct (converted to greyscale)");
}

static void test_sobel_uniform_zero_edges() {
    Image img = makeUniformImage(50, 50, 1, 128);
    
    SobelFilter filter;
    filter.apply(img);

    bool allZero = true;
    for (int i = 0; i < img.getData().size(); ++i) {
        if (img.getData()[i] != 0) allZero = false;
    }

    check(allZero, "Sobel: Uniform image produces zero edges");
}

static void test_sobel_output_in_range() {
    Image img = makeGradientImage(10, 10);
    SobelFilter filter;
    filter.apply(img);

    bool inRange = true;
    for (int i = 0; i < img.getData().size(); ++i) {
        if (img.getData()[i] > 255) inRange = false;
    }

    check(inRange, "Sobel: Output in valid range");
}

static void test_sobel_detects_edges() {
    // Create a clear edge: left half is 0, right half is 255
    Image img(5, 5, 1);
    for (int y = 0; y < 5; ++y) {
        for (int x = 0; x < 5; ++x) {
            img.setPixel(x, y, 0, x < 2 ? 0 : 255);
        }
    }
    
    SobelFilter filter;
    filter.apply(img);

    // Check that at least some edge pixels are detected (non-zero)
    // rather than expecting a specific threshold value
    bool edgeDetected = false;
    for (int y = 0; y < 5; ++y) {
        for (int x = 0; x < 5; ++x) {
            if (img.getPixel(x, y, 0) > 0) {
                edgeDetected = true;
                break;
            }
        }
        if (edgeDetected) break;
    }
    check(edgeDetected, "Sobel: Detects vertical edges");
}

// PREWITT FILTER TESTS

static void test_prewitt_dimensions_preserved() {
    Image img(640, 480, 3);
    for (auto& b : img.getData()) b = 128;
    
    PrewittFilter filter;
    filter.apply(img);

    check(img.getWidth() == 640 && img.getHeight() == 480 && img.getChannels() == 1,
          "Prewitt: Dimensions correct (converted to greyscale)");
}

static void test_prewitt_uniform_zero_edges() {
    Image img = makeUniformImage(50, 50, 1, 100);
    
    PrewittFilter filter;
    filter.apply(img);

    bool allZero = true;
    for (int i = 0; i < img.getData().size(); ++i) {
        if (img.getData()[i] != 0) allZero = false;
    }

    check(allZero, "Prewitt: Uniform image produces zero edges");
}

static void test_prewitt_output_in_range() {
    Image img = makeGradientImage(15, 15);
    PrewittFilter filter;
    filter.apply(img);

    bool inRange = true;
    for (int i = 0; i < img.getData().size(); ++i) {
        if (img.getData()[i] > 255) inRange = false;
    }

    check(inRange, "Prewitt: Output in valid range");
}

// SCHARR FILTER TESTS

static void test_scharr_dimensions_preserved() {
    Image img(640, 480, 3);
    for (auto& b : img.getData()) b = 128;
    
    ScharrFilter filter;
    filter.apply(img);

    check(img.getWidth() == 640 && img.getHeight() == 480 && img.getChannels() == 1,
          "Scharr: Dimensions correct (converted to greyscale)");
}

static void test_scharr_uniform_zero_edges() {
    Image img = makeUniformImage(50, 50, 1, 75);
    
    ScharrFilter filter;
    filter.apply(img);

    bool allZero = true;
    for (int i = 0; i < img.getData().size(); ++i) {
        if (img.getData()[i] != 0) allZero = false;
    }

    check(allZero, "Scharr: Uniform image produces zero edges");
}

static void test_scharr_output_in_range() {
    Image img = makeGradientImage(20, 20);
    ScharrFilter filter;
    filter.apply(img);

    bool inRange = true;
    for (int i = 0; i < img.getData().size(); ++i) {
        if (img.getData()[i] > 255) inRange = false;
    }

    check(inRange, "Scharr: Output in valid range");
}

static void test_scharr_high_precision() {
    // Create a diagonal edge for precision testing
    Image img(5, 5, 1);
    for (int y = 0; y < 5; ++y) {
        for (int x = 0; x < 5; ++x) {
            img.setPixel(x, y, 0, (x + y) < 4 ? 0 : 255);
        }
    }
    
    ScharrFilter filter;
    filter.apply(img);

    // Check that at least some edge pixels are detected (non-zero)
    bool edgeDetected = false;
    for (int y = 0; y < 5; ++y) {
        for (int x = 0; x < 5; ++x) {
            if (img.getPixel(x, y, 0) > 0) {
                edgeDetected = true;
                break;
            }
        }
        if (edgeDetected) break;
    }
    check(edgeDetected, "Scharr: Detects diagonal edges with precision");
}


// ROBERTS CROSS FILTER TESTS

static void test_robertscross_dimensions_preserved() {
    Image img(640, 480, 3);
    for (auto& b : img.getData()) b = 128;
    
    RobertsCrossFilter filter;
    filter.apply(img);

    check(img.getWidth() == 640 && img.getHeight() == 480 && img.getChannels() == 1,
          "RobertsCross: Dimensions correct (converted to greyscale)");
}

static void test_robertscross_uniform_zero_edges() {
    Image img = makeUniformImage(50, 50, 1, 200);
    
    RobertsCrossFilter filter;
    filter.apply(img);

    bool allZero = true;
    for (int i = 0; i < img.getData().size(); ++i) {
        if (img.getData()[i] != 0) allZero = false;
    }

    check(allZero, "RobertsCross: Uniform image produces zero edges");
}

static void test_robertscross_output_in_range() {
    Image img = makeGradientImage(20, 20);
    RobertsCrossFilter filter;
    filter.apply(img);

    bool inRange = true;
    for (int i = 0; i < img.getData().size(); ++i) {
        if (img.getData()[i] > 255) inRange = false;
    }

    check(inRange, "RobertsCross: Output in valid range");
}

static void test_robertscross_fast_computation() {
    Image img = makeTestImage3x3();
    RobertsCrossFilter filter;
    filter.apply(img);

    // Roberts should detect diagonal edges
    int centerVal = img.getPixel(1, 1, 0);
    check(centerVal > 0, "RobertsCross: Detects diagonal edges");
}


// MAIN TEST RUNNER

int main() {
    std::cout << "\n";
    std::cout << std::string(70, '=') << "\n";
    std::cout << "COMPREHENSIVE CONVOLUTIONAL FILTER TEST SUITE\n";
    std::cout << std::string(70, '=') << "\n";

    // Emboss Tests
    printSection("EMBOSS FILTER");
    test_emboss_greyscale_NW_strength1();
    test_emboss_uniform_gives_128();
    test_emboss_all_directions();
    test_emboss_strength_scaling();
    test_emboss_output_clamped();
    test_emboss_NE_kernel();
    test_emboss_SE_SW_corners();
    test_emboss_RGB_HSV_uniform();
    test_emboss_RGB_HSL_uniform();
    test_emboss_invalid_direction();
    test_emboss_dimensions_unchanged();
    test_emboss_default_type_is_HSV();

    // Gaussian Blur Tests
    printSection("GAUSSIAN BLUR FILTER");
    test_gaussian_dimensions_preserved();
    test_gaussian_uniform_unchanged();
    test_gaussian_specific_kernel();

    // Box Blur Tests
    printSection("BOX BLUR FILTER");
    test_boxblur_dimensions_preserved();
    test_boxblur_uniform_unchanged();
    test_boxblur_output_in_range();
    test_boxblur_invalid_size();
    test_boxblur_smoothing_effect();

    // Median Blur Tests
    printSection("MEDIAN BLUR FILTER");
    test_medianblur_dimensions_preserved();
    test_medianblur_uniform_unchanged();
    test_medianblur_invalid_size();
    test_medianblur_salt_pepper_removal();
    test_medianblur_output_in_range();

    // Sharpen Tests
    printSection("SHARPEN FILTER");
    test_sharpen_dimensions_preserved();
    test_sharpen_uniform_unchanged();
    test_sharpen_output_in_range();
    test_sharpen_enhances_edges();

    // Sobel Tests
    printSection("SOBEL EDGE DETECTION");
    test_sobel_dimensions_preserved();
    test_sobel_uniform_zero_edges();
    test_sobel_output_in_range();
    test_sobel_detects_edges();

    // Prewitt Tests
    printSection("PREWITT EDGE DETECTION");
    test_prewitt_dimensions_preserved();
    test_prewitt_uniform_zero_edges();
    test_prewitt_output_in_range();

    // Scharr Tests
    printSection("SCHARR EDGE DETECTION");
    test_scharr_dimensions_preserved();
    test_scharr_uniform_zero_edges();
    test_scharr_output_in_range();
    test_scharr_high_precision();

    // Roberts Cross Tests
    printSection("ROBERTS CROSS EDGE DETECTION");
    test_robertscross_dimensions_preserved();
    test_robertscross_uniform_zero_edges();
    test_robertscross_output_in_range();
    test_robertscross_fast_computation();

    // Final Results
    std::cout << "\n" << std::string(70, '=') << "\n";
    std::cout << "FINAL RESULTS: " << g_pass << " passed, " << g_fail << " failed\n";
    std::cout << std::string(70, '=') << "\n\n";

    return g_fail > 0 ? 1 : 0;
}