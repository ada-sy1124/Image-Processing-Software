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
#include <sstream>
#include <vector>
#include <cstdlib>


// Tiny test harness
// Includes macros for test case registration and assertion macros (EXPECT_TRUE, EXPECT_EQ)

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

#define EXPECT_EQ(a, b)    do { if ((a) != (b)) { \
    std::ostringstream _os; _os << "EXPECT_EQ failed: " << #a << " == " << (a) << ", " << #b << " == " << (b) \
    << "  (" << __FILE__ << ":" << __LINE__ << ")"; throw std::runtime_error(_os.str()); } } while(0)

struct TestEntry { const char* name; void (*func)(); };
static std::vector<TestEntry>& testCases() {
    static std::vector<TestEntry> v;
    return v;
}


// Test cases


// 1. Test the threshold filter on a single-channel grayscale image.
// Expected result: pixels with intensity below the threshold (128) become pure black (0),
// while pixels greater than or equal to the threshold become pure white (255).
// Failure case: if the filter's `applyGrayscale` logic is incorrect, or if the pixel comparison
// condition is implemented wrongly (e.g., missing the equality case), the test will fail.

TEST(ThresholdFilter_Grayscale) {
    Image img(2, 1, 1); // Create a 2x1 single-channel image
    img.setPixel(0, 0, 0, 100); // Set a dark pixel
    img.setPixel(1, 0, 0, 200); // Set a bright pixel

    ThresholdFilter filter(128); 
    filter.apply(img);

    EXPECT_EQ(img.getPixel(0, 0, 0), 0);
    EXPECT_EQ(img.getPixel(1, 0, 0), 255);
}


// 2. Test the threshold filter on a color image using HSV mode.
// Expected result: the filter should evaluate pixels based on the V (value/brightness)
// channel in the HSV color space. Pixels meeting the threshold condition will have all
// three RGB channels set to 255; otherwise they will be set to 0.
// Failure case: if the RGB-to-HSV conversion formula is incorrect, or if the filter
// does not correctly modify all three RGB channels, the test will fail.

TEST(ThresholdFilter_RGB_HSV) {
    Image img(2, 1, 3); // Create a 3-channel RGB image
    
    // Pixel 1: dark red
    img.setPixel(0, 0, 0, 100); img.setPixel(0, 0, 1, 0); img.setPixel(0, 0, 2, 0);
    // Pixel 2: bright red
    img.setPixel(1, 0, 0, 200); img.setPixel(1, 0, 1, 0); img.setPixel(1, 0, 2, 0);

    ThresholdFilter filter(150, "HSV"); // Threshold set to 150
    filter.apply(img);

    // Pixel 1 does not meet the threshold, all channels become black
    EXPECT_EQ(img.getPixel(0, 0, 0), 0);
    EXPECT_EQ(img.getPixel(0, 0, 1), 0);
    EXPECT_EQ(img.getPixel(0, 0, 2), 0);

    // Pixel 2 meets the threshold, all channels become white
    EXPECT_EQ(img.getPixel(1, 0, 0), 255);
    EXPECT_EQ(img.getPixel(1, 0, 1), 255);
    EXPECT_EQ(img.getPixel(1, 0, 2), 255);
}


// 3. Test the threshold filter on a color image using HSL mode.
// Expected result: the program correctly enters the HSL branch and performs
// binarization based on the L (lightness) channel, producing values of either 0 or 255.
// Failure case: if "HSL" passed to the constructor is not matched correctly internally
// (for example due to case-sensitive string comparison), or if the HSL conversion
// algorithm is incorrect, the test will fail.

TEST(ThresholdFilter_RGB_HSL) {
    Image img(2, 1, 3);
    
    // Pixel 1: dark gray
    img.setPixel(0, 0, 0, 50); img.setPixel(0, 0, 1, 50); img.setPixel(0, 0, 2, 50);
    // Pixel 2: light gray
    img.setPixel(1, 0, 0, 200); img.setPixel(1, 0, 1, 200); img.setPixel(1, 0, 2, 200);

    ThresholdFilter filter(128, "HSL");
    filter.apply(img);

    EXPECT_EQ(img.getPixel(0, 0, 0), 0);
    EXPECT_EQ(img.getPixel(1, 0, 0), 255);
}


// 4. Test extreme threshold input boundaries (0 and 256).
// Expected result: when the threshold is 0, all pixels should become white;
// when the threshold is 256 (beyond the maximum pixel value), all pixels should become black.
// Failure case: if the comparison mistakenly uses `>` instead of `>=`, boundary cases
// (such as when threshold equals 0) may be handled incorrectly and cause test failure.
 
TEST(ThresholdFilter_ExtremeValues) {
    Image img(2, 1, 1);
    img.setPixel(0, 0, 0, 10);
    img.setPixel(1, 0, 0, 240);

    // Extreme case 1: threshold = 0, all pixels become white
    ThresholdFilter filterZero(0);
    filterZero.apply(img);
    EXPECT_EQ(img.getPixel(0, 0, 0), 255);
    EXPECT_EQ(img.getPixel(1, 0, 0), 255);

    // Restore the image
    img.setPixel(0, 0, 0, 10);
    img.setPixel(1, 0, 0, 240);

    // Extreme case 2: threshold = 256, all pixels become black
    ThresholdFilter filterMax(256);
    filterMax.apply(img);
    EXPECT_EQ(img.getPixel(0, 0, 0), 0);
    EXPECT_EQ(img.getPixel(1, 0, 0), 0);
}


// Test entry point
// Run all test cases and output statistics.
// Expected result: the terminal prints each test name along with PASSED/FAILED status.
// Return value: returns 0 if all tests pass, otherwise returns 1 (so CMake/CTest can detect failure).
 
int main() {
    std::cout << "Running " << testCases().size() << " Filter unit tests...\n\n";

    for (auto& tc : testCases()) {
        std::cout << "  " << tc.name << " ... ";
        try {
            tc.func(); // Execute the specific test logic
            std::cout << "PASSED\n";
            ++g_passed;
        } catch (const std::exception& e) {
            std::cout << "FAILED\n  " << e.what() << "\n";
            ++g_failed;
        }
    }

    std::cout  << "Results: " << g_passed << " passed, " << g_failed << " failed\n";

    return g_failed > 0 ? 1 : 0;
}