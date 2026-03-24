/*
 * Unit tests for Slice class.
 * API under test:
 *   Slice::slice(const Volume& vol, int coord, char pivot)
 *     pivot='X' -> YZ plane (fix x), output W=height, H=depth
 *     pivot='Y' -> XZ plane (fix y), output W=width,  H=depth
 *     coord is 1-based
 *     z is flipped: output row 0 (top) = highest z
 */

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "Image.h"
#include "Volume.h"
#include "Slice.h"

#include <iostream>
#include <cmath>
#include <cstring>
#include <cassert>
#include <sstream>
#include <vector>
#include <algorithm>
#include <numeric>
#include <stdexcept>
#include <filesystem>
#include <iomanip>

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

/*
 * Synthetic greyscale volume: 4(W) x 3(H) x 5(D) x 1(ch)
 * Voxel value = x + y*10 + z*100  (unique, fits in [0,242])
 */
static Volume makeSyntheticVolume() {
    const int W = 4, H = 3, D = 5;
    Volume vol(W, H, D, 1);
    for (int z = 0; z < D; ++z)
        for (int y = 0; y < H; ++y)
            for (int x = 0; x < W; ++x)
                vol.setVoxel(x, y, z, 0,
                    static_cast<unsigned char>(x + y * 10 + z * 100));
    return vol;
}

/*
 * Synthetic RGB volume: 4(W) x 3(H) x 5(D) x 3(ch)
 * R = x, G = y*10, B = z*20
 */
static Volume makeRGBVolume() {
    const int W = 4, H = 3, D = 5, C = 3;
    Volume vol(W, H, D, C);
    for (int z = 0; z < D; ++z)
        for (int y = 0; y < H; ++y)
            for (int x = 0; x < W; ++x) {
                vol.setVoxel(x, y, z, 0, static_cast<unsigned char>(x));
                vol.setVoxel(x, y, z, 1, static_cast<unsigned char>(y * 10));
                vol.setVoxel(x, y, z, 2, static_cast<unsigned char>(z * 20));
            }
    return vol;
}

// ================================================================
// 1. XZ PLANE (pivot='Y', fix y) — DIMENSIONS
// ================================================================

TEST(XZPlane_Dimensions) {
    Volume vol = makeSyntheticVolume();  // 4W x 3H x 5D
    Image img = Slice::slice(vol, 2, 'Y');  // fix y=2 (1-based) -> y=1

    EXPECT_EQ(img.getWidth(), vol.getWidth());   // 4
    EXPECT_EQ(img.getHeight(), vol.getDepth());   // 5
    EXPECT_EQ(img.getChannels(), 1);
}

// 2. XZ PLANE — PIXEL VALUES
TEST(XZPlane_PixelValues) {
    Volume vol = makeSyntheticVolume();
    int D = vol.getDepth();

    Image img = Slice::slice(vol, 2, 'Y');  // 0-based y=1

    for (int z = 0; z < D; ++z) {
        int outRow = D - 1 - z;
        for (int x = 0; x < vol.getWidth(); ++x) {
            unsigned char expected = vol.getVoxel(x, 1, z, 0);
            unsigned char actual   = img.getPixel(x, outRow, 0);
            EXPECT_EQ(actual, expected);
        }
    }
}

// ================================================================
// 3. YZ PLANE (pivot='X', fix x) — DIMENSIONS
// ================================================================

TEST(YZPlane_Dimensions) {
    Volume vol = makeSyntheticVolume();
    Image img = Slice::slice(vol, 3, 'X');  // fix x=3 (1-based) -> x=2

    EXPECT_EQ(img.getWidth(), vol.getHeight());   // 3
    EXPECT_EQ(img.getHeight(), vol.getDepth());    // 5
    EXPECT_EQ(img.getChannels(), 1);
}

// 4. YZ PLANE — PIXEL VALUES
TEST(YZPlane_PixelValues) {
    Volume vol = makeSyntheticVolume();
    int D = vol.getDepth();

    Image img = Slice::slice(vol, 3, 'X');  // 0-based x=2

    for (int z = 0; z < D; ++z) {
        int outRow = D - 1 - z;
        for (int y = 0; y < vol.getHeight(); ++y) {
            unsigned char expected = vol.getVoxel(2, y, z, 0);
            unsigned char actual   = img.getPixel(y, outRow, 0);
            EXPECT_EQ(actual, expected);
        }
    }
}

// ================================================================
// 5. GREYSCALE OUTPUT
// ================================================================

TEST(Slice_IsGreyscale) {
    Volume vol = makeSyntheticVolume();
    Image xz = Slice::slice(vol, 1, 'Y');
    Image yz = Slice::slice(vol, 1, 'X');
    EXPECT_EQ(xz.getChannels(), 1);
    EXPECT_EQ(yz.getChannels(), 1);
}

// ================================================================
// 6. BOUNDARY COORDINATES (1 and max)
// ================================================================

TEST(XZPlane_FirstY) {
    Volume vol = makeSyntheticVolume();
    Image img = Slice::slice(vol, 1, 'Y');
    int bottomRow = vol.getDepth() - 1;
    EXPECT_EQ(img.getPixel(0, bottomRow, 0), vol.getVoxel(0, 0, 0, 0));
}

TEST(XZPlane_LastY) {
    Volume vol = makeSyntheticVolume();
    int H = vol.getHeight();
    Image img = Slice::slice(vol, H, 'Y');
    int bottomRow = vol.getDepth() - 1;
    EXPECT_EQ(img.getPixel(0, bottomRow, 0), vol.getVoxel(0, H - 1, 0, 0));
}

TEST(YZPlane_FirstX) {
    Volume vol = makeSyntheticVolume();
    Image img = Slice::slice(vol, 1, 'X');
    int bottomRow = vol.getDepth() - 1;
    EXPECT_EQ(img.getPixel(0, bottomRow, 0), vol.getVoxel(0, 0, 0, 0));
}

TEST(YZPlane_LastX) {
    Volume vol = makeSyntheticVolume();
    int W = vol.getWidth();
    Image img = Slice::slice(vol, W, 'X');
    int bottomRow = vol.getDepth() - 1;
    EXPECT_EQ(img.getPixel(0, bottomRow, 0), vol.getVoxel(W - 1, 0, 0, 0));
}

// ================================================================
// 7. OUT-OF-RANGE COORDINATES — should throw
// ================================================================

TEST(XZPlane_OutOfRange_Zero) {
    Volume vol = makeSyntheticVolume();
    EXPECT_THROW(Slice::slice(vol, 0, 'Y'), std::out_of_range);
}

TEST(XZPlane_OutOfRange_TooLarge) {
    Volume vol = makeSyntheticVolume();
    EXPECT_THROW(Slice::slice(vol, vol.getHeight() + 1, 'Y'), std::out_of_range);
}

TEST(YZPlane_OutOfRange_Zero) {
    Volume vol = makeSyntheticVolume();
    EXPECT_THROW(Slice::slice(vol, 0, 'X'), std::out_of_range);
}

TEST(YZPlane_OutOfRange_TooLarge) {
    Volume vol = makeSyntheticVolume();
    EXPECT_THROW(Slice::slice(vol, vol.getWidth() + 1, 'X'), std::out_of_range);
}

// ================================================================
// 8. INVALID PIVOT — should throw
// ================================================================

TEST(InvalidPivot) {
    Volume vol = makeSyntheticVolume();
    EXPECT_THROW(Slice::slice(vol, 1, 'Z'), std::invalid_argument);
    EXPECT_THROW(Slice::slice(vol, 1, 'A'), std::invalid_argument);
}

// ================================================================
// 9. LOWERCASE PIVOT — should work (normalised internally)
// ================================================================

TEST(LowercasePivot) {
    Volume vol = makeSyntheticVolume();
    Image upper = Slice::slice(vol, 2, 'Y');
    Image lower = Slice::slice(vol, 2, 'y');
    EXPECT_TRUE(upper.getData() == lower.getData());

    Image upperX = Slice::slice(vol, 2, 'X');
    Image lowerX = Slice::slice(vol, 2, 'x');
    EXPECT_TRUE(upperX.getData() == lowerX.getData());
}

// ================================================================
// 10. 1x1x1 VOLUME — minimal edge case
// ================================================================

TEST(Slice_1x1x1_Volume) {
    Volume vol(1, 1, 1, 1);
    vol.setVoxel(0, 0, 0, 0, 42);

    Image xz = Slice::slice(vol, 1, 'Y');
    EXPECT_EQ(xz.getWidth(), 1);
    EXPECT_EQ(xz.getHeight(), 1);
    EXPECT_EQ(xz.getPixel(0, 0, 0), 42);

    Image yz = Slice::slice(vol, 1, 'X');
    EXPECT_EQ(yz.getWidth(), 1);
    EXPECT_EQ(yz.getHeight(), 1);
    EXPECT_EQ(yz.getPixel(0, 0, 0), 42);
}

// ================================================================
// 11-15. RGB VOLUME SLICE TESTS
// ================================================================

TEST(XZPlane_RGB_Dimensions) {
    Volume vol = makeRGBVolume();
    Image img = Slice::slice(vol, 2, 'Y');

    EXPECT_EQ(img.getWidth(), vol.getWidth());
    EXPECT_EQ(img.getHeight(), vol.getDepth());
    EXPECT_EQ(img.getChannels(), 3);
}

TEST(XZPlane_RGB_PixelValues) {
    Volume vol = makeRGBVolume();
    int D = vol.getDepth();
    Image img = Slice::slice(vol, 2, 'Y');

    for (int z = 0; z < D; ++z) {
        int outRow = D - 1 - z;
        for (int x = 0; x < vol.getWidth(); ++x) {
            for (int c = 0; c < 3; ++c) {
                unsigned char expected = vol.getVoxel(x, 1, z, c);
                unsigned char actual   = img.getPixel(x, outRow, c);
                EXPECT_EQ(actual, expected);
            }
        }
    }
}

TEST(YZPlane_RGB_Dimensions) {
    Volume vol = makeRGBVolume();
    Image img = Slice::slice(vol, 3, 'X');

    EXPECT_EQ(img.getWidth(), vol.getHeight());
    EXPECT_EQ(img.getHeight(), vol.getDepth());
    EXPECT_EQ(img.getChannels(), 3);
}

TEST(YZPlane_RGB_PixelValues) {
    Volume vol = makeRGBVolume();
    int D = vol.getDepth();
    Image img = Slice::slice(vol, 3, 'X');

    for (int z = 0; z < D; ++z) {
        int outRow = D - 1 - z;
        for (int y = 0; y < vol.getHeight(); ++y) {
            for (int c = 0; c < 3; ++c) {
                unsigned char expected = vol.getVoxel(2, y, z, c);
                unsigned char actual   = img.getPixel(y, outRow, c);
                EXPECT_EQ(actual, expected);
            }
        }
    }
}

TEST(XZPlane_RGB_ChannelIndependence) {
    Volume vol = makeRGBVolume();
    Image img = Slice::slice(vol, 1, 'Y');  // y=0
    int D = vol.getDepth();

    for (int z = 0; z < D; ++z) {
        int outRow = D - 1 - z;
        for (int x = 0; x < vol.getWidth(); ++x) {
            EXPECT_EQ(img.getPixel(x, outRow, 0), static_cast<unsigned char>(x));
            EXPECT_EQ(img.getPixel(x, outRow, 1), static_cast<unsigned char>(0));
            EXPECT_EQ(img.getPixel(x, outRow, 2), static_cast<unsigned char>(z * 20));
        }
    }
}

// ================================================================
// 16-20. REAL SCANS LOADING + SLICE
// ================================================================

static std::string findScanPrefix() {
    for (auto& base : {"Scans/TestVolume/vol", "../Scans/TestVolume/vol",
                        "../../Scans/TestVolume/vol"}) {
        std::string test0 = std::string(base) + "000.png";
        if (std::filesystem::exists(test0)) return base;
        std::string test1 = std::string(base) + "0.png";
        if (std::filesystem::exists(test1)) return base;
    }
    return "";
}

static std::pair<int, int> findScanRange(const std::string& prefix) {
    int last = 0;
    while (true) {
        std::ostringstream oss;
        oss << prefix << std::setw(3) << std::setfill('0') << (last + 1) << ".png";
        if (std::filesystem::exists(oss.str())) { last++; continue; }
        std::ostringstream oss2;
        oss2 << prefix << (last + 1) << ".png";
        if (std::filesystem::exists(oss2.str())) { last++; continue; }
        break;
    }
    return {0, last};
}

TEST(RealScans_XZSlice) {
    std::string prefix = findScanPrefix();
    if (prefix.empty()) { std::cerr << "  [SKIP] Scans not found\n"; return; }

    auto [first, last] = findScanRange(prefix);
    Volume vol;
    EXPECT_TRUE(vol.load(prefix, first, last, ".png"));

    int midY = vol.getHeight() / 2 + 1;
    Image img = Slice::slice(vol, midY, 'Y');

    EXPECT_EQ(img.getWidth(), vol.getWidth());
    EXPECT_EQ(img.getHeight(), vol.getDepth());
    EXPECT_EQ(img.getChannels(), vol.getChannels());

    bool anyNonZero = false;
    for (size_t i = 0; i < img.getData().size() && !anyNonZero; ++i)
        if (img.getData()[i] != 0) anyNonZero = true;
    EXPECT_TRUE(anyNonZero);
}

TEST(RealScans_YZSlice) {
    std::string prefix = findScanPrefix();
    if (prefix.empty()) { std::cerr << "  [SKIP] Scans not found\n"; return; }

    auto [first, last] = findScanRange(prefix);
    Volume vol;
    EXPECT_TRUE(vol.load(prefix, first, last, ".png"));

    int midX = vol.getWidth() / 2 + 1;
    Image img = Slice::slice(vol, midX, 'X');

    EXPECT_EQ(img.getWidth(), vol.getHeight());
    EXPECT_EQ(img.getHeight(), vol.getDepth());
    EXPECT_EQ(img.getChannels(), vol.getChannels());

    bool anyNonZero = false;
    for (size_t i = 0; i < img.getData().size() && !anyNonZero; ++i)
        if (img.getData()[i] != 0) anyNonZero = true;
    EXPECT_TRUE(anyNonZero);
}

TEST(RealScans_Boundaries) {
    std::string prefix = findScanPrefix();
    if (prefix.empty()) { std::cerr << "  [SKIP] Scans not found\n"; return; }

    auto [first, last] = findScanRange(prefix);
    Volume vol;
    EXPECT_TRUE(vol.load(prefix, first, last, ".png"));

    Image f1 = Slice::slice(vol, 1, 'Y');
    EXPECT_EQ(f1.getWidth(), vol.getWidth());
    Image fN = Slice::slice(vol, vol.getHeight(), 'Y');
    EXPECT_EQ(fN.getWidth(), vol.getWidth());

    Image g1 = Slice::slice(vol, 1, 'X');
    EXPECT_EQ(g1.getWidth(), vol.getHeight());
    Image gN = Slice::slice(vol, vol.getWidth(), 'X');
    EXPECT_EQ(gN.getWidth(), vol.getHeight());
}

TEST(RealScans_ThinSlab) {
    std::string prefix = findScanPrefix();
    if (prefix.empty()) { std::cerr << "  [SKIP] Scans not found\n"; return; }

    auto [first, last] = findScanRange(prefix);
    if (last - first < 4) { std::cerr << "  [SKIP] Too few slices\n"; return; }

    Volume vol;
    EXPECT_TRUE(vol.load(prefix, first + 2, last - 2, ".png"));
    EXPECT_EQ(vol.getDepth(), (last - 2) - (first + 2) + 1);

    Image img = Slice::slice(vol, vol.getHeight() / 2 + 1, 'Y');
    EXPECT_EQ(img.getHeight(), vol.getDepth());
}

TEST(RealScans_DifferentSlices) {
    std::string prefix = findScanPrefix();
    if (prefix.empty()) { std::cerr << "  [SKIP] Scans not found\n"; return; }

    auto [first, last] = findScanRange(prefix);
    Volume vol;
    EXPECT_TRUE(vol.load(prefix, first, last, ".png"));
    if (vol.getHeight() < 2) { std::cerr << "  [SKIP] Volume too small\n"; return; }

    Image s1 = Slice::slice(vol, 1, 'Y');
    Image s2 = Slice::slice(vol, vol.getHeight(), 'Y');
    if (s1.getData() == s2.getData()) {
    std::cerr << "  [SKIP] First and last Y slices are identical (homogeneous volume)\n";
    return;
}
}

// ================================================================
// MAIN
// ================================================================

int main() {
    std::cout << "Running " << testCases().size() << " Slice unit tests...\n\n";

    for (auto& t : testCases()) {
        std::cout << "  " << t.name << " ... ";
        try {
            t.func();
            std::cout << "PASSED\n";
            g_passed++;
        } catch (const std::exception& e) {
            std::cout << "FAILED\n    " << e.what() << "\n";
            g_failed++;
        }
    }

    std::cout << "\n========================================\n"
              << "Results: " << g_passed << " passed, " << g_failed << " failed\n"
              << "========================================\n";

    return g_failed > 0 ? 1 : 0;
}
