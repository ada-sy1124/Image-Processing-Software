/*
Unit tests for the Volume class
Compile via CMake with CTest integration
Expected directory layout (relative to project root):
    Scans/TestVolume/vol1000.png ... vol10XX.png   (real 3D scan slices)
 */

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "Volume.h"
#include "Image.h"

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

// Helper: locate project-root Scans/ directory.
static std::string scansDir() {
    for (auto& candidate : {"Scans", "../Scans", "../../Scans"}) {
        if (std::filesystem::is_directory(candidate)) return candidate;
    }
    return "Scans";  // fallback
}

// Helper: create a small set of test PNG slices on disk, returns prefix
// Creates N slices of size WxH with 1 channel (greyscale).
static std::string createTestSlices(int w, int h, int numSlices,
                                     const std::string& prefix = "test_vol_slice") {
    std::string dir = "test_volume_tmp";
    std::filesystem::create_directories(dir);

    for (int z = 0; z < numSlices; z++) {
        Image img(w, h, 1);
        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w; x++) {
                // fill with a pattern dependent on x, y, z
                unsigned char val = static_cast<unsigned char>(
                    (x + y * w + z * w * h) % 256);
                img.setPixel(x, y, 0, val);
            }
        }
        // save with zero-padded name: prefix000.png, prefix001.png, ...
        std::ostringstream oss;
        oss << dir << "/" << prefix
            << std::setw(3) << std::setfill('0') << z << ".png";
        img.save(oss.str());
    }

    return dir + "/" + prefix;
}

// Helper: create test slices with a given start index (non-zero-padded)
static std::string createTestSlicesFrom(int w, int h, int firstIdx, int lastIdx,
                                         const std::string& prefix = "vol") {
    std::string dir = "test_volume_tmp2";
    std::filesystem::create_directories(dir);

    for (int z = firstIdx; z <= lastIdx; z++) {
        Image img(w, h, 1);
        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w; x++) {
                unsigned char val = static_cast<unsigned char>(
                    (x + y + z) % 256);
                img.setPixel(x, y, 0, val);
            }
        }
        // save without zero-padding: vol1000.png, vol1001.png, ...
        std::ostringstream oss;
        oss << dir << "/" << prefix << z << ".png";
        img.save(oss.str());
    }

    return dir + "/" + prefix;
}

// Helper: cleanup temp directories
static void cleanupTestSlices() {
    std::filesystem::remove_all("test_volume_tmp");
    std::filesystem::remove_all("test_volume_tmp2");
}

// ========================================================================
// CONSTRUCTOR TESTS
// ========================================================================

// 1. Default constructor
TEST(DefaultConstructor) {
    Volume vol;
    EXPECT_EQ(vol.getWidth(), 0);
    EXPECT_EQ(vol.getHeight(), 0);
    EXPECT_EQ(vol.getDepth(), 0);
    EXPECT_EQ(vol.getChannels(), 0);
    EXPECT_TRUE(vol.getData().empty());
}

// 2. Parameterised constructor
TEST(ParameterisedConstructor) {
    Volume vol(16, 8, 4, 3);
    EXPECT_EQ(vol.getWidth(), 16);
    EXPECT_EQ(vol.getHeight(), 8);
    EXPECT_EQ(vol.getDepth(), 4);
    EXPECT_EQ(vol.getChannels(), 3);
    EXPECT_EQ(static_cast<int>(vol.getData().size()), 16 * 8 * 4 * 3);

    // All voxels should be zero-initialised
    for (auto v : vol.getData()) {
        EXPECT_EQ(v, 0);
    }
}

// 3. Parameterised constructor — single channel (default)
TEST(ParameterisedConstructorSingleChannel) {
    Volume vol(10, 10, 5);
    EXPECT_EQ(vol.getChannels(), 1);
    EXPECT_EQ(static_cast<int>(vol.getData().size()), 10 * 10 * 5);
}

// ========================================================================
// VOXEL ACCESS TESTS
// ========================================================================

// 4. Basic get/set voxel
TEST(VoxelGetSet) {
    Volume vol(8, 8, 8, 1);
    vol.setVoxel(3, 4, 5, 0, 42);
    EXPECT_EQ(vol.getVoxel(3, 4, 5, 0), 42);

    // convenience overload (channel 0)
    vol.setVoxel(1, 2, 3, static_cast<unsigned char>(99));
    EXPECT_EQ(vol.getVoxel(1, 2, 3), 99);
}

// 5. Multi-channel voxel access
TEST(VoxelMultiChannel) {
    Volume vol(4, 4, 4, 3);
    vol.setVoxel(2, 1, 3, 0, 100); // R
    vol.setVoxel(2, 1, 3, 1, 150); // G
    vol.setVoxel(2, 1, 3, 2, 200); // B

    EXPECT_EQ(vol.getVoxel(2, 1, 3, 0), 100);
    EXPECT_EQ(vol.getVoxel(2, 1, 3, 1), 150);
    EXPECT_EQ(vol.getVoxel(2, 1, 3, 2), 200);
}

// 6. Voxel out-of-range — setVoxel throws
TEST(SetVoxelOutOfRange) {
    Volume vol(4, 4, 4, 1);
    EXPECT_THROW(vol.setVoxel(-1, 0, 0, 0, 0), std::out_of_range);
    EXPECT_THROW(vol.setVoxel(4, 0, 0, 0, 0), std::out_of_range);
    EXPECT_THROW(vol.setVoxel(0, -1, 0, 0, 0), std::out_of_range);
    EXPECT_THROW(vol.setVoxel(0, 4, 0, 0, 0), std::out_of_range);
    EXPECT_THROW(vol.setVoxel(0, 0, -1, 0, 0), std::out_of_range);
    EXPECT_THROW(vol.setVoxel(0, 0, 4, 0, 0), std::out_of_range);
}

// 7. Voxel channel out-of-range — setVoxel throws
TEST(SetVoxelChannelOutOfRange) {
    Volume vol(4, 4, 4, 1);
    EXPECT_THROW(vol.setVoxel(0, 0, 0, -1, 0), std::out_of_range);
    EXPECT_THROW(vol.setVoxel(0, 0, 0, 1, 0), std::out_of_range);
}

// 8. getVoxel clamps coordinates (edge-clamping)
TEST(GetVoxelClamping) {
    Volume vol(4, 4, 4, 1);
    vol.setVoxel(0, 0, 0, static_cast<unsigned char>(10));
    vol.setVoxel(3, 3, 3, static_cast<unsigned char>(20));

    // Negative coords should clamp to 0
    EXPECT_EQ(vol.getVoxel(-5, -5, -5), 10);
    // Over-boundary coords should clamp to max
    EXPECT_EQ(vol.getVoxel(100, 100, 100), 20);
}

// 9. getVoxel on empty volume throws
TEST(GetVoxelEmptyVolume) {
    Volume vol;
    EXPECT_THROW(vol.getVoxel(0, 0, 0), std::out_of_range);
}

// 10. getVoxelAsFloat
TEST(GetVoxelAsFloat) {
    Volume vol(4, 4, 4, 1);
    vol.setVoxel(1, 1, 1, static_cast<unsigned char>(200));
    float val = vol.getVoxelAsFloat(1, 1, 1);
    EXPECT_NEAR(val, 200.0f, 0.001f);
}

// 11. setVoxelClamped — clamps values to [0, 255]
TEST(SetVoxelClamped) {
    Volume vol(4, 4, 4, 1);

    vol.setVoxelClamped(0, 0, 0, 300);   // should clamp to 255
    EXPECT_EQ(vol.getVoxel(0, 0, 0), 255);

    vol.setVoxelClamped(1, 0, 0, -50);   // should clamp to 0
    EXPECT_EQ(vol.getVoxel(1, 0, 0), 0);

    vol.setVoxelClamped(2, 0, 0, 128);   // normal value
    EXPECT_EQ(vol.getVoxel(2, 0, 0), 128);
}

// 12. setVoxelClamped — multi-channel
TEST(SetVoxelClampedMultiChannel) {
    Volume vol(4, 4, 4, 3);
    vol.setVoxelClamped(0, 0, 0, 0, 500);
    vol.setVoxelClamped(0, 0, 0, 1, -100);
    vol.setVoxelClamped(0, 0, 0, 2, 100);

    EXPECT_EQ(vol.getVoxel(0, 0, 0, 0), 255);
    EXPECT_EQ(vol.getVoxel(0, 0, 0, 1), 0);
    EXPECT_EQ(vol.getVoxel(0, 0, 0, 2), 100);
}

// ========================================================================
// DATA LAYOUT AND RAW ACCESS TESTS
// ========================================================================

// 13. Data layout verification
TEST(DataLayout) {
    // Verify the documented layout: data[((z * height + y) * width + x) * channels + c]
    int w = 3, h = 4, d = 5, c = 2;
    Volume vol(w, h, d, c);

    // Set a specific voxel via setVoxel
    vol.setVoxel(2, 3, 4, 1, 77);

    // Verify directly in raw data
    size_t expectedIdx = ((static_cast<size_t>(4) * h + 3) * w + 2) * c + 1;
    EXPECT_EQ(vol.getData()[expectedIdx], 77);
}

// 14. Raw data access (non-const)
TEST(RawDataAccess) {
    Volume vol(4, 4, 4, 1);
    std::vector<unsigned char>& data = vol.getData();

    data[0] = 42;
    EXPECT_EQ(vol.getVoxel(0, 0, 0), 42);
}

// ========================================================================
// CLONE TESTS
// ========================================================================

// 15. Clone produces independent deep copy
TEST(Clone) {
    Volume vol(4, 4, 4, 1);
    vol.setVoxel(1, 2, 3, static_cast<unsigned char>(88));

    auto copy = vol.clone();

    EXPECT_EQ(copy->getWidth(), 4);
    EXPECT_EQ(copy->getHeight(), 4);
    EXPECT_EQ(copy->getDepth(), 4);
    EXPECT_EQ(copy->getChannels(), 1);
    EXPECT_EQ(copy->getVoxel(1, 2, 3), 88);

    // Modify original, clone should be unaffected
    vol.setVoxel(1, 2, 3, static_cast<unsigned char>(0));
    EXPECT_EQ(copy->getVoxel(1, 2, 3), 88);
}

// 16. Clone multi-channel volume
TEST(CloneMultiChannel) {
    Volume vol(2, 2, 2, 3);
    vol.setVoxel(0, 0, 0, 0, 10);
    vol.setVoxel(0, 0, 0, 1, 20);
    vol.setVoxel(0, 0, 0, 2, 30);

    auto copy = vol.clone();
    EXPECT_EQ(copy->getVoxel(0, 0, 0, 0), 10);
    EXPECT_EQ(copy->getVoxel(0, 0, 0, 1), 20);
    EXPECT_EQ(copy->getVoxel(0, 0, 0, 2), 30);
    EXPECT_EQ(copy->getChannels(), 3);
}

// ========================================================================
// LOAD FROM SLICES TESTS (synthetic slices)
// ========================================================================

// 17. Load from synthetic zero-padded slices (auto-detect)
TEST(LoadSyntheticAutoDetect) {
    int w = 8, h = 8, numSlices = 4;
    std::string prefix = createTestSlices(w, h, numSlices);

    Volume vol;
    bool ok = vol.load(prefix, -1, -1, ".png");
    EXPECT_TRUE(ok);
    EXPECT_EQ(vol.getWidth(), w);
    EXPECT_EQ(vol.getHeight(), h);
    EXPECT_EQ(vol.getDepth(), numSlices);
    EXPECT_EQ(vol.getChannels(), 1);

    // Verify some voxel values match the pattern we wrote
    for (int z = 0; z < numSlices; z++) {
        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w; x++) {
                unsigned char expected = static_cast<unsigned char>(
                    (x + y * w + z * w * h) % 256);
                EXPECT_EQ(vol.getVoxel(x, y, z), expected);
            }
        }
    }

    cleanupTestSlices();
}

// 18. Load with explicit first/last range
TEST(LoadExplicitRange) {
    int w = 4, h = 4, numSlices = 6;
    std::string prefix = createTestSlices(w, h, numSlices);

    Volume vol;
    // Load only slices 1-3
    bool ok = vol.load(prefix, 1, 3, ".png");
    EXPECT_TRUE(ok);
    EXPECT_EQ(vol.getWidth(), w);
    EXPECT_EQ(vol.getHeight(), h);
    EXPECT_EQ(vol.getDepth(), 3); // slices 1, 2, 3

    cleanupTestSlices();
}

// 19. Load from non-zero-padded filenames (e.g., vol1000.png)
TEST(LoadNonZeroPadded) {
    int w = 4, h = 4;
    int firstIdx = 1000, lastIdx = 1003;
    std::string prefix = createTestSlicesFrom(w, h, firstIdx, lastIdx);

    Volume vol;
    bool ok = vol.load(prefix, firstIdx, lastIdx, ".png");
    EXPECT_TRUE(ok);
    EXPECT_EQ(vol.getWidth(), w);
    EXPECT_EQ(vol.getHeight(), h);
    EXPECT_EQ(vol.getDepth(), 4); // 1000, 1001, 1002, 1003

    // Verify voxel values match pattern
    for (int z = 0; z < 4; z++) {
        int origZ = firstIdx + z;
        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w; x++) {
                unsigned char expected = static_cast<unsigned char>(
                    (x + y + origZ) % 256);
                EXPECT_EQ(vol.getVoxel(x, y, z), expected);
            }
        }
    }

    cleanupTestSlices();
}

// 20. Load from non-existent prefix fails
TEST(LoadNonExistentPrefix) {
    Volume vol;
    bool ok = vol.load("this_path_does_not_exist/vol", -1, -1, ".png");
    EXPECT_FALSE(ok);
}

// 21. Load single slice
TEST(LoadSingleSlice) {
    int w = 4, h = 4;
    std::string prefix = createTestSlices(w, h, 1);

    Volume vol;
    bool ok = vol.load(prefix, 0, 0, ".png");
    EXPECT_TRUE(ok);
    EXPECT_EQ(vol.getWidth(), w);
    EXPECT_EQ(vol.getHeight(), h);
    EXPECT_EQ(vol.getDepth(), 1);

    cleanupTestSlices();
}

// ========================================================================
// LOAD FROM REAL SCANS (if available)
// ========================================================================

// 22. Load real 3D scan data from Scans/ directory
TEST(LoadRealScans) {
    // Try to find the Scans directory and auto-detect files
    std::string scanPrefix;
    bool found = false;

    // Try several common paths for the scans
    for (auto& base : {"Scans/TestVolume/vol", "../Scans/TestVolume/vol",
                        "../../Scans/TestVolume/vol",
                        "Scans/confuciusornis/vol", "../Scans/confuciusornis/vol",
                        "../../Scans/confuciusornis/vol"}) {
        // Check if first slice (vol1000.png) exists
        std::string testPath = std::string(base) + "1000.png";
        if (std::filesystem::exists(testPath)) {
            scanPrefix = base;
            found = true;
            break;
        }
    }

    if (!found) {
        std::cerr << "  [SKIP] Real scan data not found in Scans/\n";
        return;
    }

    // Find the range of slices
    int first = 1000, last = 1000;
    while (true) {
        std::ostringstream oss;
        oss << scanPrefix << (last + 1) << ".png";
        if (!std::filesystem::exists(oss.str())) break;
        last++;
    }

    std::cout << "(loading slices " << first << "-" << last << ") ";

    Volume vol;
    bool ok = vol.load(scanPrefix, first, last, ".png");
    EXPECT_TRUE(ok);
    EXPECT_TRUE(vol.getWidth() > 0);
    EXPECT_TRUE(vol.getHeight() > 0);
    EXPECT_EQ(vol.getDepth(), last - first + 1);
    EXPECT_TRUE(vol.getChannels() == 1 || vol.getChannels() == 3);

    // Data buffer size should match dimensions
    size_t expectedSize = static_cast<size_t>(vol.getWidth()) *
                          vol.getHeight() * vol.getDepth() * vol.getChannels();
    EXPECT_EQ(vol.getData().size(), expectedSize);

    // Voxels should not all be zero (real scan data)
    bool anyNonZero = false;
    for (size_t i = 0; i < vol.getData().size() && !anyNonZero; i++) {
        if (vol.getData()[i] != 0) anyNonZero = true;
    }
    EXPECT_TRUE(anyNonZero);

    std::cout << "(" << vol.getWidth() << "x" << vol.getHeight()
              << "x" << vol.getDepth() << " ch=" << vol.getChannels() << ") ";
}

// ========================================================================
// PROPERTIES TESTS
// ========================================================================

// 23. Properties after construction
TEST(Properties) {
    Volume vol(32, 64, 16, 3);
    EXPECT_EQ(vol.getWidth(), 32);
    EXPECT_EQ(vol.getHeight(), 64);
    EXPECT_EQ(vol.getDepth(), 16);
    EXPECT_EQ(vol.getChannels(), 3);
}

// ========================================================================
// EDGE CASE TESTS
// ========================================================================

// 24. 1x1x1 volume
TEST(TinyVolume) {
    Volume vol(1, 1, 1, 1);
    EXPECT_EQ(static_cast<int>(vol.getData().size()), 1);
    vol.setVoxel(0, 0, 0, static_cast<unsigned char>(255));
    EXPECT_EQ(vol.getVoxel(0, 0, 0), 255);
}

// 25. Large volume allocation
TEST(LargeVolume) {
    // 128^3 x 1 channel = ~2MB, should be fine
    Volume vol(128, 128, 128, 1);
    EXPECT_EQ(static_cast<int>(vol.getData().size()), 128 * 128 * 128);
    vol.setVoxel(127, 127, 127, static_cast<unsigned char>(42));
    EXPECT_EQ(vol.getVoxel(127, 127, 127), 42);
}

// 26. All voxel values 0-255 roundtrip
TEST(AllVoxelValues) {
    Volume vol(256, 1, 1, 1);
    for (int i = 0; i < 256; i++) {
        vol.setVoxel(i, 0, 0, static_cast<unsigned char>(i));
    }
    for (int i = 0; i < 256; i++) {
        EXPECT_EQ(vol.getVoxel(i, 0, 0), static_cast<unsigned char>(i));
    }
}

// 27. Fill and verify entire volume
TEST(FillEntireVolume) {
    int w = 4, h = 4, d = 4;
    Volume vol(w, h, d, 1);

    // Fill with predictable pattern
    for (int z = 0; z < d; z++)
        for (int y = 0; y < h; y++)
            for (int x = 0; x < w; x++)
                vol.setVoxel(x, y, z, static_cast<unsigned char>((x + y + z) % 256));

    // Verify
    for (int z = 0; z < d; z++)
        for (int y = 0; y < h; y++)
            for (int x = 0; x < w; x++)
                EXPECT_EQ(vol.getVoxel(x, y, z),
                          static_cast<unsigned char>((x + y + z) % 256));
}

// 28. Boundary voxels
TEST(BoundaryVoxels) {
    int w = 5, h = 6, d = 7;
    Volume vol(w, h, d, 1);

    // Set all 8 corners
    vol.setVoxel(0,     0,     0,     static_cast<unsigned char>(1));
    vol.setVoxel(w - 1, 0,     0,     static_cast<unsigned char>(2));
    vol.setVoxel(0,     h - 1, 0,     static_cast<unsigned char>(3));
    vol.setVoxel(w - 1, h - 1, 0,     static_cast<unsigned char>(4));
    vol.setVoxel(0,     0,     d - 1, static_cast<unsigned char>(5));
    vol.setVoxel(w - 1, 0,     d - 1, static_cast<unsigned char>(6));
    vol.setVoxel(0,     h - 1, d - 1, static_cast<unsigned char>(7));
    vol.setVoxel(w - 1, h - 1, d - 1, static_cast<unsigned char>(8));

    EXPECT_EQ(vol.getVoxel(0,     0,     0),     1);
    EXPECT_EQ(vol.getVoxel(w - 1, 0,     0),     2);
    EXPECT_EQ(vol.getVoxel(0,     h - 1, 0),     3);
    EXPECT_EQ(vol.getVoxel(w - 1, h - 1, 0),     4);
    EXPECT_EQ(vol.getVoxel(0,     0,     d - 1), 5);
    EXPECT_EQ(vol.getVoxel(w - 1, 0,     d - 1), 6);
    EXPECT_EQ(vol.getVoxel(0,     h - 1, d - 1), 7);
    EXPECT_EQ(vol.getVoxel(w - 1, h - 1, d - 1), 8);
}

// 29. getVoxel clamping with channels
TEST(GetVoxelClampingChannels) {
    Volume vol(2, 2, 2, 3);
    vol.setVoxel(0, 0, 0, 0, 10);
    vol.setVoxel(0, 0, 0, 1, 20);
    vol.setVoxel(0, 0, 0, 2, 30);

    // Channel clamping
    EXPECT_EQ(vol.getVoxel(0, 0, 0, -1), 10);  // clamp to channel 0
    EXPECT_EQ(vol.getVoxel(0, 0, 0, 99), 30);  // clamp to channel 2
}

// 30. Volume data size matches expectations for various configs
TEST(DataSizeConsistency) {
    struct { int w, h, d, c; } configs[] = {
        {1, 1, 1, 1},
        {10, 20, 30, 1},
        {5, 5, 5, 3},
        {100, 100, 10, 1},
        {2, 3, 4, 4},
    };
    for (auto& cfg : configs) {
        Volume vol(cfg.w, cfg.h, cfg.d, cfg.c);
        size_t expected = static_cast<size_t>(cfg.w) * cfg.h * cfg.d * cfg.c;
        EXPECT_EQ(vol.getData().size(), expected);
    }
}

// ========================================================================
// main — run all registered tests
// ========================================================================
int main() {
    std::cout << "Running " << testCases().size() << " Volume unit tests...\n\n";

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
