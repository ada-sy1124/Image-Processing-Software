#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "Image.h"
#include "Volume.h"
#include "Projection.h"

#include <iostream>
#include <cmath>
#include <cassert>
#include <sstream>
#include <vector>
#include <algorithm>
#include <stdexcept>

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

// ============================================================
// Helper: greyscale test volume  3x2x4, 1 channel
// ============================================================
static Volume makeTestVolume() {
    Volume v(3, 2, 4);
    unsigned char slices[4][6] = {
        { 10,  20,  30,  40,  50,  60},
        { 70,  80,  90, 100, 110, 120},
        {  5,  15,  25,  35,  45,  55},
        {200, 150, 100,  50,   0,   1},
    };
    auto& data = v.getData();
    for (int z = 0; z < 4; z++)
        for (int i = 0; i < 6; i++)
            data[z * 6 + i] = slices[z][i];
    return v;
}

// ============================================================
// Helper: RGB test volume  2x2x3, 3 channels
// R = x*10, G = y*20, B = z*30  (at each voxel)
// ============================================================
static Volume makeRGBVolume() {
    const int W = 2, H = 2, D = 3, C = 3;
    Volume v(W, H, D, C);
    for (int z = 0; z < D; ++z)
        for (int y = 0; y < H; ++y)
            for (int x = 0; x < W; ++x) {
                v.setVoxel(x, y, z, 0, static_cast<unsigned char>(x * 10));
                v.setVoxel(x, y, z, 1, static_cast<unsigned char>(y * 20));
                v.setVoxel(x, y, z, 2, static_cast<unsigned char>(z * 30));
            }
    return v;
}

// ============================================================
// Helper: RGB volume with distinct per-channel variation
// 2x1x4, 3 channels
// R at (0,0,z): {10, 50, 30, 90}
// G at (0,0,z): {80, 20, 60, 40}
// B at (0,0,z): {5, 100, 55, 200}
// ============================================================
static Volume makeRGBVaryingVolume() {
    Volume v(2, 1, 4, 3);
    // pixel (0,0)
    unsigned char R[] = {10, 50, 30, 90};
    unsigned char G[] = {80, 20, 60, 40};
    unsigned char B[] = {5, 100, 55, 200};
    for (int z = 0; z < 4; ++z) {
        v.setVoxel(0, 0, z, 0, R[z]);
        v.setVoxel(0, 0, z, 1, G[z]);
        v.setVoxel(0, 0, z, 2, B[z]);
        // pixel (1,0): simple constant per channel
        v.setVoxel(1, 0, z, 0, 100);
        v.setVoxel(1, 0, z, 1, 50);
        v.setVoxel(1, 0, z, 2, 25);
    }
    return v;
}

// ================================================================
// GREYSCALE TESTS (original)
// ================================================================

TEST(MIP_full) {
    Volume vol = makeTestVolume();
    Image out = Projection::maxIntensity(vol);
    EXPECT_EQ(out.getWidth(), 3);
    EXPECT_EQ(out.getHeight(), 2);
    EXPECT_EQ(out.getChannels(), 1);
    EXPECT_EQ(out.getPixel(0, 0, 0), 200);
    EXPECT_EQ(out.getPixel(1, 0, 0), 150);
    EXPECT_EQ(out.getPixel(2, 0, 0), 100);
    EXPECT_EQ(out.getPixel(0, 1, 0), 100);
    EXPECT_EQ(out.getPixel(1, 1, 0), 110);
    EXPECT_EQ(out.getPixel(2, 1, 0), 120);
}

TEST(MinIP_full) {
    Volume vol = makeTestVolume();
    Image out = Projection::minIntensity(vol);
    EXPECT_EQ(out.getPixel(0, 0, 0), 5);
    EXPECT_EQ(out.getPixel(1, 0, 0), 15);
    EXPECT_EQ(out.getPixel(2, 0, 0), 25);
    EXPECT_EQ(out.getPixel(0, 1, 0), 35);
    EXPECT_EQ(out.getPixel(1, 1, 0), 0);
    EXPECT_EQ(out.getPixel(2, 1, 0), 1);
}

TEST(AIP_full) {
    Volume vol = makeTestVolume();
    Image out = Projection::averageIntensity(vol);
    EXPECT_EQ(out.getPixel(0, 0, 0), 71);
    EXPECT_EQ(out.getPixel(1, 0, 0), 66);
    EXPECT_EQ(out.getPixel(2, 0, 0), 61);
    EXPECT_EQ(out.getPixel(0, 1, 0), 56);
    EXPECT_EQ(out.getPixel(1, 1, 0), 51);
    EXPECT_EQ(out.getPixel(2, 1, 0), 59);
}

TEST(MedianIP_full) {
    Volume vol = makeTestVolume();
    Image out = Projection::medianIntensity(vol);
    EXPECT_EQ(out.getPixel(0, 0, 0), 70);
    EXPECT_EQ(out.getPixel(1, 0, 0), 80);
    EXPECT_EQ(out.getPixel(2, 0, 0), 90);
    EXPECT_EQ(out.getPixel(0, 1, 0), 50);
    EXPECT_EQ(out.getPixel(1, 1, 0), 50);
    EXPECT_EQ(out.getPixel(2, 1, 0), 60);
}

// ====== Thin-slab tests (greyscale) ======

TEST(MIP_slab_z1_z2) {
    Volume vol = makeTestVolume();
    Image out = Projection::maxIntensity(vol, 1, 2);
    EXPECT_EQ(out.getPixel(0, 0, 0), 70);
    EXPECT_EQ(out.getPixel(1, 0, 0), 80);
    EXPECT_EQ(out.getPixel(2, 0, 0), 90);
    EXPECT_EQ(out.getPixel(0, 1, 0), 100);
    EXPECT_EQ(out.getPixel(1, 1, 0), 110);
    EXPECT_EQ(out.getPixel(2, 1, 0), 120);
}

TEST(MinIP_slab_z0_z1) {
    Volume vol = makeTestVolume();
    Image out = Projection::minIntensity(vol, 0, 1);
    EXPECT_EQ(out.getPixel(0, 0, 0), 10);
    EXPECT_EQ(out.getPixel(1, 0, 0), 20);
    EXPECT_EQ(out.getPixel(2, 0, 0), 30);
    EXPECT_EQ(out.getPixel(0, 1, 0), 40);
    EXPECT_EQ(out.getPixel(1, 1, 0), 50);
    EXPECT_EQ(out.getPixel(2, 1, 0), 60);
}

TEST(AIP_slab_z2_z3) {
    Volume vol = makeTestVolume();
    Image out = Projection::averageIntensity(vol, 2, 3);
    EXPECT_EQ(out.getPixel(0, 0, 0), 103);
    EXPECT_EQ(out.getPixel(1, 0, 0), 83);
    EXPECT_EQ(out.getPixel(2, 0, 0), 63);
    EXPECT_EQ(out.getPixel(0, 1, 0), 43);
    EXPECT_EQ(out.getPixel(1, 1, 0), 23);
    EXPECT_EQ(out.getPixel(2, 1, 0), 28);
}

TEST(MedianIP_slab_z0_z2) {
    Volume vol = makeTestVolume();
    Image out = Projection::medianIntensity(vol, 0, 2);
    EXPECT_EQ(out.getPixel(0, 0, 0), 10);
    EXPECT_EQ(out.getPixel(1, 0, 0), 20);
    EXPECT_EQ(out.getPixel(2, 0, 0), 30);
    EXPECT_EQ(out.getPixel(0, 1, 0), 40);
    EXPECT_EQ(out.getPixel(1, 1, 0), 50);
    EXPECT_EQ(out.getPixel(2, 1, 0), 60);
}

TEST(Slab_single_slice) {
    Volume vol = makeTestVolume();
    Image out = Projection::maxIntensity(vol, 2, 2);
    EXPECT_EQ(out.getPixel(0, 0, 0), 5);
    EXPECT_EQ(out.getPixel(1, 0, 0), 15);
    EXPECT_EQ(out.getPixel(2, 0, 0), 25);
    EXPECT_EQ(out.getPixel(0, 1, 0), 35);
    EXPECT_EQ(out.getPixel(1, 1, 0), 45);
    EXPECT_EQ(out.getPixel(2, 1, 0), 55);
}

TEST(Slab_defaults_full_volume) {
    Volume vol = makeTestVolume();
    Image full = Projection::maxIntensity(vol);
    Image defaultSlab = Projection::maxIntensity(vol, -1, -1);
    for (int y = 0; y < 2; y++)
        for (int x = 0; x < 3; x++)
            EXPECT_EQ(full.getPixel(x, y, 0), defaultSlab.getPixel(x, y, 0));
}

// ================================================================
// RGB TESTS
// ================================================================

// 1. MIP RGB — output dimensions and channel count
TEST(MIP_RGB_Dimensions) {
    Volume vol = makeRGBVolume();  // 2x2x3, 3ch
    Image out = Projection::maxIntensity(vol);
    EXPECT_EQ(out.getWidth(), 2);
    EXPECT_EQ(out.getHeight(), 2);
    EXPECT_EQ(out.getChannels(), 3);
}

// 2. MIP RGB — per-channel max values
// R = x*10: constant across z → max = x*10
// G = y*20: constant across z → max = y*20
// B = z*30: varies {0,30,60} → max = 60
TEST(MIP_RGB_PixelValues) {
    Volume vol = makeRGBVolume();
    Image out = Projection::maxIntensity(vol);

    for (int y = 0; y < 2; ++y) {
        for (int x = 0; x < 2; ++x) {
            EXPECT_EQ(out.getPixel(x, y, 0), static_cast<unsigned char>(x * 10));  // R max
            EXPECT_EQ(out.getPixel(x, y, 1), static_cast<unsigned char>(y * 20));  // G max
            EXPECT_EQ(out.getPixel(x, y, 2), 60);  // B max = 2*30
        }
    }
}

// 3. MinIP RGB — per-channel min values
// R = x*10: constant → min = x*10
// G = y*20: constant → min = y*20
// B = z*30: varies {0,30,60} → min = 0
TEST(MinIP_RGB_PixelValues) {
    Volume vol = makeRGBVolume();
    Image out = Projection::minIntensity(vol);

    for (int y = 0; y < 2; ++y) {
        for (int x = 0; x < 2; ++x) {
            EXPECT_EQ(out.getPixel(x, y, 0), static_cast<unsigned char>(x * 10));
            EXPECT_EQ(out.getPixel(x, y, 1), static_cast<unsigned char>(y * 20));
            EXPECT_EQ(out.getPixel(x, y, 2), 0);  // B min = 0*30
        }
    }
}

// 4. AIP RGB — per-channel average
// R = x*10: constant → avg = x*10
// G = y*20: constant → avg = y*20
// B = z*30: {0,30,60} → avg = 90/3 = 30
TEST(AIP_RGB_PixelValues) {
    Volume vol = makeRGBVolume();
    Image out = Projection::averageIntensity(vol);

    for (int y = 0; y < 2; ++y) {
        for (int x = 0; x < 2; ++x) {
            EXPECT_EQ(out.getPixel(x, y, 0), static_cast<unsigned char>(x * 10));
            EXPECT_EQ(out.getPixel(x, y, 1), static_cast<unsigned char>(y * 20));
            EXPECT_EQ(out.getPixel(x, y, 2), 30);  // B avg
        }
    }
}

// 5. MedianIP RGB — per-channel median
// B = z*30: {0,30,60}, d=3, half=1 → median = 30
TEST(MedianIP_RGB_PixelValues) {
    Volume vol = makeRGBVolume();
    Image out = Projection::medianIntensity(vol);

    for (int y = 0; y < 2; ++y) {
        for (int x = 0; x < 2; ++x) {
            EXPECT_EQ(out.getPixel(x, y, 0), static_cast<unsigned char>(x * 10));
            EXPECT_EQ(out.getPixel(x, y, 1), static_cast<unsigned char>(y * 20));
            EXPECT_EQ(out.getPixel(x, y, 2), 30);  // B median
        }
    }
}

// 6. MIP RGB — channels are independent (different max per channel)
// pixel (0,0): R={10,50,30,90} G={80,20,60,40} B={5,100,55,200}
// max: R=90, G=80, B=200
TEST(MIP_RGB_ChannelIndependence) {
    Volume vol = makeRGBVaryingVolume();
    Image out = Projection::maxIntensity(vol);

    EXPECT_EQ(out.getPixel(0, 0, 0), 90);   // R max
    EXPECT_EQ(out.getPixel(0, 0, 1), 80);   // G max
    EXPECT_EQ(out.getPixel(0, 0, 2), 200);  // B max

    // pixel (1,0): constant {100,50,25} across all z
    EXPECT_EQ(out.getPixel(1, 0, 0), 100);
    EXPECT_EQ(out.getPixel(1, 0, 1), 50);
    EXPECT_EQ(out.getPixel(1, 0, 2), 25);
}

// 7. MinIP RGB — channels are independent
// pixel (0,0): R min=10, G min=20, B min=5
TEST(MinIP_RGB_ChannelIndependence) {
    Volume vol = makeRGBVaryingVolume();
    Image out = Projection::minIntensity(vol);

    EXPECT_EQ(out.getPixel(0, 0, 0), 10);  // R min
    EXPECT_EQ(out.getPixel(0, 0, 1), 20);  // G min
    EXPECT_EQ(out.getPixel(0, 0, 2), 5);   // B min
}

// 8. AIP RGB — channels are independent
// pixel (0,0): R avg=(10+50+30+90)/4=180/4=45
//              G avg=(80+20+60+40)/4=200/4=50
//              B avg=(5+100+55+200)/4=360/4=90
TEST(AIP_RGB_ChannelIndependence) {
    Volume vol = makeRGBVaryingVolume();
    Image out = Projection::averageIntensity(vol);

    EXPECT_EQ(out.getPixel(0, 0, 0), 45);  // R avg
    EXPECT_EQ(out.getPixel(0, 0, 1), 50);  // G avg
    EXPECT_EQ(out.getPixel(0, 0, 2), 90);  // B avg
}

// 9. MedianIP RGB — channels are independent
// pixel (0,0): d=4, half=2
// R sorted: {10,30,50,90} → median at count>2 = 50
// G sorted: {20,40,60,80} → median = 60
// B sorted: {5,55,100,200} → median = 100
TEST(MedianIP_RGB_ChannelIndependence) {
    Volume vol = makeRGBVaryingVolume();
    Image out = Projection::medianIntensity(vol);

    EXPECT_EQ(out.getPixel(0, 0, 0), 50);   // R median
    EXPECT_EQ(out.getPixel(0, 0, 1), 60);   // G median
    EXPECT_EQ(out.getPixel(0, 0, 2), 100);  // B median
}

// 10. Thin slab on RGB volume
// makeRGBVolume: B = z*30, slab z=0..1 → B values {0,30}
// MIP B = 30, MinIP B = 0, AIP B = 15, Median B (d=2,half=1) = 30
TEST(MIP_RGB_Slab) {
    Volume vol = makeRGBVolume();
    Image out = Projection::maxIntensity(vol, 0, 1);
    EXPECT_EQ(out.getChannels(), 3);
    EXPECT_EQ(out.getPixel(0, 0, 2), 30);  // B max of {0,30}
}

TEST(MinIP_RGB_Slab) {
    Volume vol = makeRGBVolume();
    Image out = Projection::minIntensity(vol, 0, 1);
    EXPECT_EQ(out.getPixel(0, 0, 2), 0);   // B min of {0,30}
}

TEST(AIP_RGB_Slab) {
    Volume vol = makeRGBVolume();
    Image out = Projection::averageIntensity(vol, 0, 1);
    EXPECT_EQ(out.getPixel(0, 0, 2), 15);  // B avg of {0,30}
}

TEST(MedianIP_RGB_Slab) {
    Volume vol = makeRGBVolume();
    Image out = Projection::medianIntensity(vol, 0, 1);
    // d=2, half=1 → first value where count>1 = 30
    EXPECT_EQ(out.getPixel(0, 0, 2), 30);
}

// ====== Test runner ======

int main() {
    std::cout << "Running " << testCases().size() << " Projection unit tests...\n\n";

    for (auto& tc : testCases()) {
        try {
            tc.func();
            g_passed++;
            std::cout << "  PASS  " << tc.name << "\n";
        } catch (const std::exception& e) {
            g_failed++;
            std::cout << "  FAIL  " << tc.name << ": " << e.what() << "\n";
        }
    }

    std::cout << "\n========================================\n"
              << "Results: " << g_passed << " passed, " << g_failed << " failed\n"
              << "========================================\n";
    return g_failed == 0 ? 0 : 1;
}
