/**
 * Unit tests for EmbossFilter
 *
 * Strategy: construct small images with known pixel values, manually compute
 * the expected convolution output, and compare with EmbossFilter::apply().
 *
 * No external image-processing libraries are used.
 * All expected values are derived by hand or by direct formula application.
 *
 * Compile (from project root, adjust paths as needed):
 *   g++ -std=c++17 -I src \
 *       src/Image.cpp src/ConvolutionalFilter.cpp src/Filter.cpp \
 *       src/SimpleFilter.cpp \
 *       tests/test_emboss.cpp \
 *       -o build/test_emboss
 * Run:
 *   ./build/test_emboss
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


// ─────────────────────────────────────────────────────────────
// Tiny test framework
// ─────────────────────────────────────────────────────────────
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

// Allow ±1 tolerance for rounding differences between float and uchar
static bool near(int a, int b, int tol = 1) {
    return std::abs(a - b) <= tol;
}

// ─────────────────────────────────────────────────────────────
// Helper: apply the NW kernel by hand to a single-channel
// flat pixel array and return I_emboss = clamp(S*(G*I)+128, 0, 255)
// for pixel (px, py) in an image of size (w x h).
// Uses edge-clamp for out-of-bounds neighbours.
// ─────────────────────────────────────────────────────────────
static float manualConvolveNW(const std::vector<float>& src,
                               int w, int h, int px, int py, float S) {
    // NW kernel row-major [ky+1][kx+1]
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

// ─────────────────────────────────────────────────────────────
// Helper: build a 3x3 greyscale Image from a flat array
// ─────────────────────────────────────────────────────────────
static Image makeGrey3x3(const std::vector<unsigned char>& pixels) {
    // pixels must have exactly 9 elements
    Image img(3, 3, 1);
    for (int i = 0; i < 9; ++i)
        img.getData()[i] = pixels[i];
    return img;
}

// ─────────────────────────────────────────────────────────────
// TEST 1: Greyscale image, NW direction, strength=1.0
//         Verify every pixel against manual convolution
// ─────────────────────────────────────────────────────────────
static void test_greyscale_NW_strength1() {
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
                std::cout << "  Mismatch at (" << x << "," << y << "): "
                          << "expected=" << expected << " got=" << got << "\n";
                allOk = false;
            }
        }
    }
    check(allOk, "Greyscale NW strength=1.0: all pixels match manual convolution");
}

// ─────────────────────────────────────────────────────────────
// TEST 2: Greyscale uniform image → all outputs should be 128
//         (convolution of any directional kernel on a flat image is 0)
// ─────────────────────────────────────────────────────────────
static void test_greyscale_uniform_gives_128() {
    // Uniform grey: every pixel = 100
    Image img(5, 5, 1);
    for (auto& b : img.getData()) b = 100;

    EmbossFilter f(2.5f, "NW");
    f.apply(img);

    bool allOk = true;
    for (int y = 0; y < 5; ++y)
        for (int x = 0; x < 5; ++x)
            if (img.getPixel(x, y, 0) != 128) allOk = false;

    check(allOk, "Greyscale uniform image: all outputs == 128 (zero convolution + bias)");
}

// ─────────────────────────────────────────────────────────────
// TEST 3: Uniform image also holds for all four directions
// ─────────────────────────────────────────────────────────────
static void test_uniform_all_directions() {
    for (const std::string& dir : {"NW", "NE", "SE", "SW"}) {
        Image img(4, 4, 1);
        for (auto& b : img.getData()) b = 200;

        EmbossFilter f(3.0f, dir);
        f.apply(img);

        bool allOk = true;
        for (int y = 0; y < 4; ++y)
            for (int x = 0; x < 4; ++x)
                if (img.getPixel(x, y, 0) != 128) allOk = false;

        check(allOk, "Uniform image direction=" + dir + ": all outputs == 128");
    }
}

// ─────────────────────────────────────────────────────────────
// TEST 4: Strength scaling — same image, strength 1 vs 2
//         Displacement from 128 should double (before clamping)
// ─────────────────────────────────────────────────────────────
static void test_strength_scaling() {
    // Use a gradient image where the centre pixel has a non-zero convolution
    const std::vector<unsigned char> pixels = {
         0,  0,  0,
         0, 50,255,
         0,255,255
    };

    // Strength 1
    Image img1 = makeGrey3x3(pixels);
    EmbossFilter f1(1.0f, "NW");
    f1.apply(img1);
    int out1 = static_cast<int>(img1.getPixel(1, 1, 0));

    // Strength 2
    Image img2 = makeGrey3x3(pixels);
    EmbossFilter f2(2.0f, "NW");
    f2.apply(img2);
    int out2 = static_cast<int>(img2.getPixel(1, 1, 0));

    // Manually compute expected centre pixel for strength=1
    std::vector<float> src(9);
    for (int i = 0; i < 9; ++i) src[i] = static_cast<float>(pixels[i]);
    float e1 = manualConvolveNW(src, 3, 3, 1, 1, 1.0f);
    float e2 = manualConvolveNW(src, 3, 3, 1, 1, 2.0f);

    check(near(out1, static_cast<int>(std::round(e1))),
          "Strength=1 centre pixel matches manual formula");
    check(near(out2, static_cast<int>(std::round(e2))),
          "Strength=2 centre pixel matches manual formula");
}

// ─────────────────────────────────────────────────────────────
// TEST 5: Output clamped to [0, 255]
//         High strength on a high-contrast image should clamp
// ─────────────────────────────────────────────────────────────
static void test_output_clamped() {
    // Extreme gradient: top-left dark, bottom-right bright
    const std::vector<unsigned char> pixels = {
          0,   0,   0,
          0, 128, 255,
          0, 255, 255
    };
    Image img = makeGrey3x3(pixels);

    EmbossFilter f(10.0f, "NW");  // strength=10 → likely to overflow
    f.apply(img);

    bool allInRange = true;
    for (int y = 0; y < 3; ++y)
        for (int x = 0; x < 3; ++x) {
            int v = static_cast<int>(img.getPixel(x, y, 0));
            if (v < 0 || v > 255) allInRange = false;
        }
    check(allInRange, "All output pixels clamped to [0, 255] with extreme strength");
}

// ─────────────────────────────────────────────────────────────
// TEST 6: NE kernel is the correct rotation of NW
//         Verify by applying NE to a specific input and comparing
//         with hand-computed NE convolution
// ─────────────────────────────────────────────────────────────
static void test_NE_kernel_correct() {
    // NE kernel:
    //  [ 0, -1, -2]
    //  [ 1,  0, -1]
    //  [ 2,  1,  0]
    const std::vector<unsigned char> pixels = {
        10, 20, 30,
        40, 50, 60,
        70, 80, 90
    };
    Image img = makeGrey3x3(pixels);
    EmbossFilter f(1.0f, "NE");
    f.apply(img);

    // Hand-compute centre pixel (1,1):
    // neighbours (edge-clamped, all in bounds for centre):
    // (-1,-1)=10, (0,-1)=20, (1,-1)=30
    // (-1, 0)=40, (0, 0)=50, (1, 0)=60
    // (-1, 1)=70, (0, 1)=80, (1, 1)=90
    // NE kernel applied (ky row, kx col):
    // ky=-1: 0*10 + (-1)*20 + (-2)*30 = -80
    // ky= 0: 1*40 +   0*50 + (-1)*60 = -20
    // ky= 1: 2*70 +   1*80 +   0*90 = 220
    // acc = -80 - 20 + 220 = 120
    // I_emboss = 1.0 * 120 + 128 = 248
    int expected = 248;
    int got = static_cast<int>(img.getPixel(1, 1, 0));
    check(near(got, expected), "NE kernel centre pixel: expected 248, got " + std::to_string(got));
}

// ─────────────────────────────────────────────────────────────
// TEST 7: SE and SW kernels — verify corner pixel (top-left, 0,0)
//         where edge-clamping has maximum effect
// ─────────────────────────────────────────────────────────────
static void test_SE_SW_corner_edge_clamped() {
    const std::vector<unsigned char> pixels = {
        100, 150, 200,
        100, 150, 200,
        100, 150, 200
    };

    // SE kernel:
    // [ 2,  1,  0]
    // [ 1,  0, -1]
    // [ 0, -1, -2]
    // Top-left pixel (0,0) with edge clamp:
    // neighbours: all kx<0 → clamped to x=0, all ky<0 → clamped to y=0
    // (0,0)→100, (1,0)→150, (0,0)→100 (clamped), (1,0)→150 ...
    // Explicitly:
    // ky=-1,kx=-1 → (0,0)=100  kernel=2
    // ky=-1,kx= 0 → (0,0)=100  kernel=1
    // ky=-1,kx= 1 → (1,0)=150  kernel=0
    // ky= 0,kx=-1 → (0,0)=100  kernel=1
    // ky= 0,kx= 0 → (0,0)=100  kernel=0
    // ky= 0,kx= 1 → (1,0)=150  kernel=-1
    // ky= 1,kx=-1 → (0,1)=100  kernel=0
    // ky= 1,kx= 0 → (0,1)=100  kernel=-1
    // ky= 1,kx= 1 → (1,1)=150  kernel=-2
    // acc = 2*100 + 1*100 + 0*150 + 1*100 + 0*100 + (-1)*150
    //       + 0*100 + (-1)*100 + (-2)*150
    //     = 200 + 100 + 0 + 100 + 0 - 150 + 0 - 100 - 300 = -150
    // I_emboss = 1.0*(-150) + 128 = 0  (clamped)
    {
        Image img = makeGrey3x3(pixels);
        EmbossFilter f(1.0f, "SE");
        f.apply(img);
        int got = static_cast<int>(img.getPixel(0, 0, 0));
        check(got == 0, "SE corner (0,0) edge-clamped: expected 0, got " + std::to_string(got));
    }

    // SW kernel:
    // [ 0,  1,  2]
    // [-1,  0,  1]
    // [-2, -1,  0]
    // Top-left pixel (0,0) with same pixels:
    // ky=-1,kx=-1 → (0,0)=100  kernel=0
    // ky=-1,kx= 0 → (0,0)=100  kernel=1
    // ky=-1,kx= 1 → (1,0)=150  kernel=2
    // ky= 0,kx=-1 → (0,0)=100  kernel=-1
    // ky= 0,kx= 0 → (0,0)=100  kernel=0
    // ky= 0,kx= 1 → (1,0)=150  kernel=1
    // ky= 1,kx=-1 → (0,1)=100  kernel=-2
    // ky= 1,kx= 0 → (0,1)=100  kernel=-1
    // ky= 1,kx= 1 → (1,1)=150  kernel=0
    // acc = 0 + 100 + 300 - 100 + 0 + 150 - 200 - 100 + 0 = 150
    // I_emboss = 1.0*150 + 128 = 255 (clamped at 255)
    {
        Image img = makeGrey3x3(pixels);
        EmbossFilter f(1.0f, "SW");
        f.apply(img);
        int got = static_cast<int>(img.getPixel(0, 0, 0));
        check(got == 255, "SW corner (0,0) edge-clamped: expected 255, got " + std::to_string(got));
    }
}

// ─────────────────────────────────────────────────────────────
// TEST 8: RGB image with HSV type — uniform image gives 128 on V
//         Hue and saturation should be preserved (uniform → S=0)
// ─────────────────────────────────────────────────────────────
static void test_RGB_HSV_uniform_gives_128() {
    // Uniform mid-grey RGB image → V=0.5 everywhere, S=0
    Image img(3, 3, 3);
    for (int i = 0; i < 3*3*3; i += 3) {
        img.getData()[i+0] = 128;  // R
        img.getData()[i+1] = 128;  // G
        img.getData()[i+2] = 128;  // B
    }

    EmbossFilter f(2.0f, "NW", "HSV");
    f.apply(img);

    // After uniform convolution: V_out = 128/255, so RGB should all be ~128
    bool allOk = true;
    for (int y = 0; y < 3; ++y)
        for (int x = 0; x < 3; ++x) {
            int r = img.getPixel(x, y, 0);
            int g = img.getPixel(x, y, 1);
            int b = img.getPixel(x, y, 2);
            // grey stays grey: R==G==B
            if (!near(r, 128, 2) || !near(g, 128, 2) || !near(b, 128, 2))
                allOk = false;
        }
    check(allOk, "RGB HSV uniform image: all pixels remain ~(128,128,128)");
}

// ─────────────────────────────────────────────────────────────
// TEST 9: RGB image with HSL type — uniform image gives 128 on L
// ─────────────────────────────────────────────────────────────
static void test_RGB_HSL_uniform_gives_128() {
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
    check(allOk, "RGB HSL uniform image: all pixels remain ~(128,128,128)");
}

// ─────────────────────────────────────────────────────────────
// TEST 10: Invalid direction throws std::invalid_argument
// ─────────────────────────────────────────────────────────────
static void test_invalid_direction_throws() {
    Image img(3, 3, 1);
    for (auto& b : img.getData()) b = 100;

    EmbossFilter f(1.0f, "XX");  // invalid direction
    bool threw = false;
    try {
        f.apply(img);
    } catch (const std::invalid_argument&) {
        threw = true;
    } catch (...) {}
    check(threw, "Invalid direction 'XX' throws std::invalid_argument");
}

// ─────────────────────────────────────────────────────────────
// TEST 11: Image unchanged dimensions and channel count after filter
// ─────────────────────────────────────────────────────────────
static void test_dimensions_unchanged() {
    Image img(7, 5, 1);
    for (auto& b : img.getData()) b = 80;

    EmbossFilter f(1.5f, "SW");
    f.apply(img);

    check(img.getWidth() == 7 && img.getHeight() == 5 && img.getChannels() == 1,
          "Image dimensions and channels unchanged after emboss");
}

// ─────────────────────────────────────────────────────────────
// TEST 12: Default colour space is HSV (no type argument)
//          Result should match explicit HSV
// ─────────────────────────────────────────────────────────────
static void test_default_type_is_HSV() {
    // Build two identical RGB images
    Image img1(3, 3, 3);
    Image img2(3, 3, 3);
    unsigned char val = 0;
    for (int i = 0; i < 3*3*3; ++i) {
        img1.getData()[i] = val;
        img2.getData()[i] = val;
        val = (val + 17) % 256;
    }

    EmbossFilter fDefault(1.0f, "SE");        // default type (should be HSV)
    EmbossFilter fHSV(1.0f, "SE", "HSV");

    fDefault.apply(img1);
    fHSV.apply(img2);

    bool same = true;
    for (int i = 0; i < 3*3*3; ++i)
        if (img1.getData()[i] != img2.getData()[i]) { same = false; break; }

    check(same, "Default colour space matches explicit HSV");
}

// ─────────────────────────────────────────────────────────────
// Main
// ─────────────────────────────────────────────────────────────
int main() {
    std::cout << "=== EmbossFilter Unit Tests ===\n\n";

    test_greyscale_NW_strength1();
    test_greyscale_uniform_gives_128();
    test_uniform_all_directions();
    test_strength_scaling();
    test_output_clamped();
    test_NE_kernel_correct();
    test_SE_SW_corner_edge_clamped();
    test_RGB_HSV_uniform_gives_128();
    test_RGB_HSL_uniform_gives_128();
    test_invalid_direction_throws();
    test_dimensions_unchanged();
    test_default_type_is_HSV();

    std::cout << "\n=== Results: " << g_pass << " passed, "
              << g_fail << " failed ===\n";
    return g_fail > 0 ? 1 : 0;
}