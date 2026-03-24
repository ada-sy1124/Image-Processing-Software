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
// Includes test case registration macros and assertion macros (EXPECT_TRUE, EXPECT_EQ)

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

// 1. Test the boundary case of 0% noise intensity.
// Expected result: the filter should not modify any pixel, and the image should remain in its initial state (all values = 128).
// Failure case: if the code handles 0 incorrectly (e.g., infinite loop, crash, or accidentally introducing noise), the assertion will fail.

TEST(SaltPepperFilter_ZeroAmount) {
    Image img(10, 10, 1);
    // Initialize all pixels to 128
    for(int i = 0; i < 100; i++) img.getData()[i] = 128;

    SaltPepperFilter filter(0); // 0% noise
    filter.apply(img);

    for(int i = 0; i < 100; i++) {
        EXPECT_EQ(img.getData()[i], 128);
    }
}

// 2. Test the noise injection logic under a normal noise level (10%).
// Expected result: the image should contain both pure black pixels (pepper) and pure white pixels (salt),
// while a large portion of the original pixels remain unchanged, and the total number of pixels is preserved.
// Failure case: if the random logic is incorrect (e.g., only white pixels are generated or the whole image becomes black),
// or if out-of-bounds modification occurs causing pixel loss, EXPECT_TRUE or EXPECT_EQ will fail.

TEST(SaltPepperFilter_NormalAmount) {
    Image img(20, 20, 1); // image with 400 pixels
    for(int i = 0; i < 400; i++) img.getData()[i] = 128; // uniform base color 128

    SaltPepperFilter filter(10); // apply 10% noise
    filter.apply(img);

    int countBlack = 0;
    int countWhite = 0;
    int countUnchanged = 0;

    for(int i = 0; i < 400; i++) {
        if (img.getData()[i] == 0) countBlack++;
        else if (img.getData()[i] == 255) countWhite++;
        else if (img.getData()[i] == 128) countUnchanged++;
    }

    // Assert that both black and white noise pixels exist and some original pixels remain
    EXPECT_TRUE(countBlack > 0);
    EXPECT_TRUE(countWhite > 0);
    EXPECT_TRUE(countUnchanged > 0);
    
    // Assert that the total pixel count is preserved
    EXPECT_EQ(countBlack + countWhite + countUnchanged, 400);
}

// 3. Test the extreme case of 100% noise level.
// Expected result: due to heavy noise injection, most pixels should be modified to 0 or 255.
// (Note: because random coordinates may overlap, we do not require 100% of pixels to change).
// Failure case: if multiplication overflows or the `amount` parameter is parsed incorrectly,
// causing insufficient noise injection, the number of changed pixels may not reach the threshold (>50).

TEST(SaltPepperFilter_HundredPercent) {
    Image img(10, 10, 1); 
    for(int i = 0; i < 100; i++) img.getData()[i] = 128; 

    SaltPepperFilter filter(100); // 100% destruction
    filter.apply(img);
    
    int changedPixels = 0;
    for(int i = 0; i < 100; i++) {
        if (img.getData()[i] == 0 || img.getData()[i] == 255) {
            changedPixels++;
        }
    }
    
    // As long as more than half of the pixels are modified, the 100% noise logic works correctly
    EXPECT_TRUE(changedPixels > 50); 
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
            tc.func();
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