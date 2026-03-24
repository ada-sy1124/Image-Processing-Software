/*
 * Unit tests for GaussianBlur3DFilter and MedianBlur3DFilter.
 * Compile via CMake with CTest integration.
 */

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "Filters3D.h"
#include "Volume.h"

#include <iostream>
#include <cmath>
#include <sstream>
#include <vector>
#include <algorithm>
#include <stdexcept>

static int g_passed = 0;
static int g_failed = 0;

#define TEST(name)                                                         \
    static void test_##name();                                             \
    static struct Register_##name                                          \
    {                                                                      \
        Register_##name() { testCases().push_back({#name, test_##name}); } \
    } reg_##name;                                                          \
    static void test_##name()

#define EXPECT_TRUE(expr)                                                                                               \
    do                                                                                                                  \
    {                                                                                                                   \
        if (!(expr))                                                                                                    \
        {                                                                                                               \
            throw std::runtime_error(                                                                                   \
                std::string("EXPECT_TRUE failed: ") + #expr + "  (" + __FILE__ + ":" + std::to_string(__LINE__) + ")"); \
        }                                                                                                               \
    } while (0)

#define EXPECT_FALSE(expr) EXPECT_TRUE(!(expr))

#define EXPECT_EQ(a, b)                                         \
    do                                                          \
    {                                                           \
        if (!((a) == (b)))                                      \
        {                                                       \
            std::ostringstream _os;                             \
            _os << "EXPECT_EQ failed: " << #a << " == " << #b   \
                << "  (" << __FILE__ << ":" << __LINE__ << ")"; \
            throw std::runtime_error(_os.str());                \
        }                                                       \
    } while (0)

#define EXPECT_NEAR(a, b, eps)                                                                  \
    do                                                                                          \
    {                                                                                           \
        if (std::abs((a) - (b)) > (eps))                                                        \
        {                                                                                       \
            std::ostringstream _os;                                                             \
            _os << "EXPECT_NEAR failed: " << #a << " == " << (a) << ", " << #b << " == " << (b) \
                << ", eps=" << (eps) << "  (" << __FILE__ << ":" << __LINE__ << ")";            \
            throw std::runtime_error(_os.str());                                                \
        }                                                                                       \
    } while (0)

#define EXPECT_THROW(expr, exType)                                                                                                                     \
    do                                                                                                                                                 \
    {                                                                                                                                                  \
        bool _caught = false;                                                                                                                          \
        try                                                                                                                                            \
        {                                                                                                                                              \
            expr;                                                                                                                                      \
        }                                                                                                                                              \
        catch (const exType &)                                                                                                                         \
        {                                                                                                                                              \
            _caught = true;                                                                                                                            \
        }                                                                                                                                              \
        if (!_caught)                                                                                                                                  \
        {                                                                                                                                              \
            throw std::runtime_error(                                                                                                                  \
                std::string("EXPECT_THROW failed: ") + #expr + " did not throw " + #exType + "  (" + __FILE__ + ":" + std::to_string(__LINE__) + ")"); \
        }                                                                                                                                              \
    } while (0)

struct TestEntry
{
    const char *name;
    void (*func)();
};
static std::vector<TestEntry> &testCases()
{
    static std::vector<TestEntry> v;
    return v;
}

// ======================================================================
// GaussianBlur3DFilter tests
// ======================================================================

// 1. Valid construction does not throw
TEST(GaussianConstruct)
{
    GaussianBlur3DFilter f(3, 1.0);
    EXPECT_TRUE(true);
}

// 2. Apply to empty volume throws
TEST(GaussianEmptyVolumeThrows)
{
    Volume vol;
    GaussianBlur3DFilter f(3, 1.0);
    EXPECT_THROW(f.apply(vol), std::runtime_error);
}

// 3. Even kernel size throws on apply
TEST(GaussianEvenKernelSizeThrows)
{
    Volume vol(4, 4, 4, 1);
    GaussianBlur3DFilter f(4, 1.0);
    EXPECT_THROW(f.apply(vol), std::invalid_argument);
}

// 4. Negative sigma throws on apply
TEST(GaussianNegativeSigmaThrows)
{
    Volume vol(4, 4, 4, 1);
    GaussianBlur3DFilter f(3, -1.0);
    EXPECT_THROW(f.apply(vol), std::invalid_argument);
}

// 5. Zero sigma throws on apply
TEST(GaussianZeroSigmaThrows)
{
    Volume vol(4, 4, 4, 1);
    GaussianBlur3DFilter f(3, 0.0);
    EXPECT_THROW(f.apply(vol), std::invalid_argument);
}

// 6. Kernel size == 1 is a no-op (data unchanged)
TEST(GaussianKernelSize1NoOp)
{
    Volume vol(4, 4, 4, 1);
    for (int z = 0; z < 4; z++)
        for (int y = 0; y < 4; y++)
            for (int x = 0; x < 4; x++)
                vol.setVoxel(x, y, z, static_cast<unsigned char>((x + y + z) % 256));

    auto original = vol.getData();
    GaussianBlur3DFilter f(1, 1.0);
    f.apply(vol);
    EXPECT_EQ(vol.getData(), original);
}

// 7. Constant-valued volume stays constant after blur
TEST(GaussianConstantVolume)
{
    const int w = 8, h = 8, d = 8;
    Volume vol(w, h, d, 1);
    for (int z = 0; z < d; z++)
        for (int y = 0; y < h; y++)
            for (int x = 0; x < w; x++)
                vol.setVoxel(x, y, z, static_cast<unsigned char>(128));

    GaussianBlur3DFilter f(3, 1.0);
    f.apply(vol);

    for (int z = 0; z < d; z++)
        for (int y = 0; y < h; y++)
            for (int x = 0; x < w; x++)
                EXPECT_EQ(vol.getVoxel(x, y, z), 128);
}

// 8. Bright point (spike) gets blurred — center value decreases
TEST(GaussianBlursSpike)
{
    const int w = 7, h = 7, d = 7;
    Volume vol(w, h, d, 1);
    vol.setVoxel(3, 3, 3, static_cast<unsigned char>(255));

    GaussianBlur3DFilter f(3, 1.0);
    f.apply(vol);

    // Center should have lost energy
    EXPECT_TRUE(vol.getVoxel(3, 3, 3) < 255);
    // At least one direct neighbor should have gained energy
    EXPECT_TRUE(vol.getVoxel(2, 3, 3) > 0 ||
                vol.getVoxel(3, 2, 3) > 0 ||
                vol.getVoxel(3, 3, 2) > 0);
}

// 9. 3-channel (RGB) volume: constant colors stay constant
TEST(GaussianRGBVolume)
{
    const int w = 6, h = 6, d = 6;
    Volume vol(w, h, d, 3);
    for (int z = 0; z < d; z++)
        for (int y = 0; y < h; y++)
            for (int x = 0; x < w; x++)
            {
                vol.setVoxel(x, y, z, 0, 100);
                vol.setVoxel(x, y, z, 1, 150);
                vol.setVoxel(x, y, z, 2, 200);
            }

    GaussianBlur3DFilter f(3, 1.0);
    f.apply(vol);

    // Constant volume must stay constant after blur
    for (int z = 0; z < d; z++)
        for (int y = 0; y < h; y++)
            for (int x = 0; x < w; x++)
            {
                EXPECT_EQ(vol.getVoxel(x, y, z, 0), 100);
                EXPECT_EQ(vol.getVoxel(x, y, z, 1), 150);
                EXPECT_EQ(vol.getVoxel(x, y, z, 2), 200);
            }
}

// 10. Both channel counts 1 and 3 apply without throwing
TEST(GaussianValidChannels)
{
    bool threw = false;

    Volume vol1(4, 4, 4, 1);
    GaussianBlur3DFilter f1(3, 1.0);
    try
    {
        f1.apply(vol1);
    }
    catch (...)
    {
        threw = true;
    }
    EXPECT_FALSE(threw);

    Volume vol3(4, 4, 4, 3);
    GaussianBlur3DFilter f3(3, 1.0);
    threw = false;
    try
    {
        f3.apply(vol3);
    }
    catch (...)
    {
        threw = true;
    }
    EXPECT_FALSE(threw);
}

// 11. Larger kernel produces more blur (lower center value from identical spike)
TEST(GaussianLargerKernelMoreBlur)
{
    const int w = 9, h = 9, d = 9;

    Volume vol3(w, h, d, 1);
    vol3.setVoxel(w / 2, h / 2, d / 2, static_cast<unsigned char>(255));

    Volume vol7(w, h, d, 1);
    vol7.setVoxel(w / 2, h / 2, d / 2, static_cast<unsigned char>(255));

    GaussianBlur3DFilter f3(3, 1.5);
    GaussianBlur3DFilter f7(7, 1.5);
    f3.apply(vol3);
    f7.apply(vol7);

    int center3 = vol3.getVoxel(w / 2, h / 2, d / 2);
    int center7 = vol7.getVoxel(w / 2, h / 2, d / 2);

    // Larger kernel => more energy spread => lower center
    EXPECT_TRUE(center7 <= center3);
}

// 12. Blur preserves total data size
TEST(GaussianPreservesDataSize)
{
    Volume vol(5, 6, 7, 1);
    size_t sizeBefore = vol.getData().size();
    GaussianBlur3DFilter f(3, 1.0);
    f.apply(vol);
    EXPECT_EQ(vol.getData().size(), sizeBefore);
}

// ======================================================================
// MedianBlur3DFilter tests
// ======================================================================

// 13. Even kernel size throws in constructor
TEST(MedianEvenKernelSizeThrows)
{
    EXPECT_THROW(MedianBlur3DFilter f(4), std::invalid_argument);
}

// 14. Zero kernel size throws in constructor
TEST(MedianZeroKernelSizeThrows)
{
    EXPECT_THROW(MedianBlur3DFilter f(0), std::invalid_argument);
}

// 15. Negative kernel size throws in constructor
TEST(MedianNegativeKernelSizeThrows)
{
    EXPECT_THROW(MedianBlur3DFilter f(-3), std::invalid_argument);
}

// 16. Apply to empty volume throws
TEST(MedianEmptyVolumeThrows)
{
    Volume vol;
    MedianBlur3DFilter f(3);
    EXPECT_THROW(f.apply(vol), std::runtime_error);
}

// 17. Constant volume stays constant after median
TEST(MedianConstantVolume)
{
    const int w = 6, h = 6, d = 6;
    Volume vol(w, h, d, 1);
    for (int z = 0; z < d; z++)
        for (int y = 0; y < h; y++)
            for (int x = 0; x < w; x++)
                vol.setVoxel(x, y, z, static_cast<unsigned char>(100));

    MedianBlur3DFilter f(3);
    f.apply(vol);

    for (int z = 0; z < d; z++)
        for (int y = 0; y < h; y++)
            for (int x = 0; x < w; x++)
                EXPECT_EQ(vol.getVoxel(x, y, z), 100);
}

// 18. Single salt voxel removed by median
TEST(MedianRemovesSaltNoise)
{
    const int w = 7, h = 7, d = 7;
    Volume vol(w, h, d, 1);
    for (int z = 0; z < d; z++)
        for (int y = 0; y < h; y++)
            for (int x = 0; x < w; x++)
                vol.setVoxel(x, y, z, static_cast<unsigned char>(50));

    vol.setVoxel(3, 3, 3, static_cast<unsigned char>(255));

    MedianBlur3DFilter f(3);
    f.apply(vol);

    EXPECT_EQ(vol.getVoxel(3, 3, 3), 50);
}

// 19. Single pepper voxel removed by median
TEST(MedianRemovesPepperNoise)
{
    const int w = 7, h = 7, d = 7;
    Volume vol(w, h, d, 1);
    for (int z = 0; z < d; z++)
        for (int y = 0; y < h; y++)
            for (int x = 0; x < w; x++)
                vol.setVoxel(x, y, z, static_cast<unsigned char>(200));

    vol.setVoxel(3, 3, 3, static_cast<unsigned char>(0));

    MedianBlur3DFilter f(3);
    f.apply(vol);

    EXPECT_EQ(vol.getVoxel(3, 3, 3), 200);
}

// 20. 3-channel (RGB) volume: constant colors stay constant
TEST(MedianRGBVolume)
{
    const int w = 6, h = 6, d = 6;
    Volume vol(w, h, d, 3);
    for (int z = 0; z < d; z++)
        for (int y = 0; y < h; y++)
            for (int x = 0; x < w; x++)
            {
                vol.setVoxel(x, y, z, 0, 80);
                vol.setVoxel(x, y, z, 1, 120);
                vol.setVoxel(x, y, z, 2, 160);
            }

    MedianBlur3DFilter f(3);
    f.apply(vol);

    for (int z = 0; z < d; z++)
        for (int y = 0; y < h; y++)
            for (int x = 0; x < w; x++)
            {
                EXPECT_EQ(vol.getVoxel(x, y, z, 0), 80);
                EXPECT_EQ(vol.getVoxel(x, y, z, 1), 120);
                EXPECT_EQ(vol.getVoxel(x, y, z, 2), 160);
            }
}

// 21. Median preserves data size
TEST(MedianPreservesDataSize)
{
    Volume vol(5, 6, 7, 1);
    size_t sizeBefore = vol.getData().size();
    MedianBlur3DFilter f(3);
    f.apply(vol);
    EXPECT_EQ(vol.getData().size(), sizeBefore);
}

// 22. Kernel size 1 is a no-op (median of single voxel = itself)
TEST(MedianKernelSize1NoOp)
{
    Volume vol(4, 4, 4, 1);
    for (int z = 0; z < 4; z++)
        for (int y = 0; y < 4; y++)
            for (int x = 0; x < 4; x++)
                vol.setVoxel(x, y, z, static_cast<unsigned char>((x * 7 + y * 3 + z * 5) % 256));

    auto original = vol.getData();
    MedianBlur3DFilter f(1);
    f.apply(vol);
    EXPECT_EQ(vol.getData(), original);
}

// 23. Valid odd kernel sizes 1, 3, 5, 7 construct without throwing
TEST(MedianValidConstruction)
{
    for (int size : {1, 3, 5, 7})
    {
        bool threw = false;
        try
        {
            MedianBlur3DFilter f(size);
        }
        catch (...)
        {
            threw = true;
        }
        EXPECT_FALSE(threw);
    }
}

// 24. Median on boundary voxels: constant volume produces same output at corners
TEST(MedianBoundaryVoxels)
{
    const int w = 4, h = 4, d = 4;
    Volume vol(w, h, d, 1);
    for (int z = 0; z < d; z++)
        for (int y = 0; y < h; y++)
            for (int x = 0; x < w; x++)
                vol.setVoxel(x, y, z, static_cast<unsigned char>(100));

    MedianBlur3DFilter f(3);
    f.apply(vol);

    EXPECT_EQ(vol.getVoxel(0, 0, 0), 100);
    EXPECT_EQ(vol.getVoxel(w - 1, h - 1, d - 1), 100);
}

// ======================================================================
// main
// ======================================================================
int main()
{
    std::cout << "Running " << testCases().size() << " Filter3D unit tests...\n\n";

    for (auto &tc : testCases())
    {
        std::cout << "  " << tc.name << " ... ";
        try
        {
            tc.func();
            std::cout << "PASSED\n";
            ++g_passed;
        }
        catch (const std::exception &e)
        {
            std::cout << "FAILED\n    " << e.what() << "\n";
            ++g_failed;
        }
    }

    std::cout << "\n========================================\n"
              << "Results: " << g_passed << " passed, " << g_failed << " failed\n"
              << "========================================\n";

    return g_failed > 0 ? 1 : 0;
}
