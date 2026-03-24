/*
Unit tests for the Image class
Compile via CMake with CTest integration
Expected directory layout (relative to project root):
    Images/bourton.png
    Images/stinkbug.png
 */

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "Image.h"

#include <iostream>
#include <cmath>
#include <cstring>
#include <cassert>
#include <filesystem>
#include <sstream>
#include <vector>
#include <algorithm>

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

// Helper: locate project-root Images/ directory.
static std::string imagesDir() {
    for (auto& candidate : {"Images", "../Images", "../../Images"}) {
        if (std::filesystem::is_directory(candidate)) return candidate;
    }
    return "Images";  // fallback
}

static std::string imgPath(const std::string& filename) {
    return imagesDir() + "/" + filename;
}

// 1. Default constructor
TEST(DefaultConstructor) {
    Image img;
    EXPECT_EQ(img.getWidth(), 0);
    EXPECT_EQ(img.getHeight(), 0);
    EXPECT_EQ(img.getChannels(), 0);
    EXPECT_TRUE(img.getData().empty());
}

// 2. Parameterised constructor (blank image)
TEST(ParameterisedConstructor) {
    Image img(64, 48, 3);
    EXPECT_EQ(img.getWidth(), 64);
    EXPECT_EQ(img.getHeight(), 48);
    EXPECT_EQ(img.getChannels(), 3);
    EXPECT_EQ(static_cast<int>(img.getData().size()), 64 * 48 * 3);

    // All pixels should be zero-initialised
    for (auto v : img.getData()) {
        EXPECT_EQ(v, 0);
    }
}

TEST(ParameterisedConstructorGrayscale) {
    Image img(10, 10, 1);
    EXPECT_EQ(img.getChannels(), 1);
    EXPECT_EQ(static_cast<int>(img.getData().size()), 100);
}

// 3. Constructor from raw data (3-channel)
TEST(RawDataConstructorRGB) {
    const int w = 2, h = 2;
    unsigned char raw[2 * 2 * 3] = {
        255, 0, 0,    0, 255, 0,
        0, 0, 255,  128, 128, 128
    };
    Image img(raw, w, h, 3);
    EXPECT_EQ(img.getWidth(), 2);
    EXPECT_EQ(img.getHeight(), 2);
    EXPECT_EQ(img.getChannels(), 3);
    EXPECT_EQ(img.getPixel(0, 0, 0), 255);  // R
    EXPECT_EQ(img.getPixel(0, 0, 1), 0);    // G
    EXPECT_EQ(img.getPixel(1, 0, 1), 255);  // green pixel G channel
}

// 4. Constructor from raw data (4-channel -> strips alpha)
TEST(RawDataConstructorRGBA) {
    const int w = 2, h = 1;
    unsigned char raw[2 * 1 * 4] = {
        10, 20, 30, 255,
        40, 50, 60, 128
    };
    Image img(raw, w, h, 4);
    EXPECT_EQ(img.getChannels(), 3);  // alpha stripped
    EXPECT_EQ(img.getPixel(0, 0, 0), 10);
    EXPECT_EQ(img.getPixel(0, 0, 1), 20);
    EXPECT_EQ(img.getPixel(0, 0, 2), 30);
    EXPECT_EQ(img.getPixel(1, 0, 0), 40);
}

// 5. Constructor from raw data (1-channel grayscale)
TEST(RawDataConstructorGray) {
    const int w = 3, h = 1;
    unsigned char raw[3] = {0, 128, 255};
    Image img(raw, w, h, 1);
    EXPECT_EQ(img.getChannels(), 1);
    EXPECT_EQ(img.getPixel(0, 0, 0), 0);
    EXPECT_EQ(img.getPixel(1, 0, 0), 128);
    EXPECT_EQ(img.getPixel(2, 0, 0), 255);
}

// 6. Load bourton.png
TEST(LoadPNG) {
    std::string path = imgPath("bourton.png");
    if (!std::filesystem::exists(path)) {
        std::cerr << "  [SKIP] " << path << " not found\n";
        return;
    }
    auto img = Image::load(path);
    EXPECT_TRUE(img != nullptr);
    EXPECT_TRUE(img->getWidth() > 0);
    EXPECT_TRUE(img->getHeight() > 0);
    EXPECT_TRUE(img->getChannels() == 1 || img->getChannels() == 3);
}

// 7. Load stinkbug.png
TEST(LoadStinkbug) {
    std::string path = imgPath("stinkbug.png");
    if (!std::filesystem::exists(path)) {
        std::cerr << "  [SKIP] " << path << " not found\n";
        return;
    }
    auto img = Image::load(path);
    EXPECT_TRUE(img != nullptr);
    EXPECT_TRUE(img->getWidth() > 0);
    EXPECT_TRUE(img->getHeight() > 0);
}

// 8. Load non-existent file returns nullptr
TEST(LoadNonExistent) {
    auto img = Image::load("this_file_does_not_exist_abc123.png");
    EXPECT_TRUE(img == nullptr);
}

// 9. Save and reload PNG
TEST(SaveAndReloadPNG) {
    Image img(4, 4, 3);
    // Paint a known pattern
    for (int y = 0; y < 4; ++y)
        for (int x = 0; x < 4; ++x) {
            img.setPixel(x, y, 0, static_cast<unsigned char>(x * 60));
            img.setPixel(x, y, 1, static_cast<unsigned char>(y * 60));
            img.setPixel(x, y, 2, 100);
        }

    std::string tmp = "test_save_reload.png";
    EXPECT_TRUE(img.save(tmp));

    auto loaded = Image::load(tmp);
    EXPECT_TRUE(loaded != nullptr);
    EXPECT_EQ(loaded->getWidth(), 4);
    EXPECT_EQ(loaded->getHeight(), 4);
    EXPECT_EQ(loaded->getChannels(), 3);

    // PNG is lossless — pixels should match exactly
    for (int y = 0; y < 4; ++y)
        for (int x = 0; x < 4; ++x)
            for (int c = 0; c < 3; ++c)
                EXPECT_EQ(loaded->getPixel(x, y, c), img.getPixel(x, y, c));

    std::filesystem::remove(tmp);
}

// 10. Save and reload JPG (allow small difference)
TEST(SaveAndReloadJPG) {
    Image img(8, 8, 3);
    for (int y = 0; y < 8; ++y)
        for (int x = 0; x < 8; ++x) {
            img.setPixel(x, y, 0, 128);
            img.setPixel(x, y, 1, 128);
            img.setPixel(x, y, 2, 128);
        }

    std::string tmp = "test_save_reload.jpg";
    EXPECT_TRUE(img.save(tmp));

    auto loaded = Image::load(tmp);
    EXPECT_TRUE(loaded != nullptr);
    EXPECT_EQ(loaded->getWidth(), 8);
    EXPECT_EQ(loaded->getHeight(), 8);

    // JPG is lossy — allow a tolerance
    for (int y = 0; y < 8; ++y)
        for (int x = 0; x < 8; ++x)
            for (int c = 0; c < 3; ++c)
                EXPECT_NEAR(static_cast<int>(loaded->getPixel(x, y, c)), 128, 10);

    std::filesystem::remove(tmp);
}

// 11. Save and reload BMP
TEST(SaveAndReloadBMP) {
    Image img(4, 4, 3);
    img.setPixel(0, 0, 0, 255);
    img.setPixel(0, 0, 1, 0);
    img.setPixel(0, 0, 2, 0);

    std::string tmp = "test_save_reload.bmp";
    EXPECT_TRUE(img.save(tmp));

    auto loaded = Image::load(tmp);
    EXPECT_TRUE(loaded != nullptr);
    EXPECT_EQ(loaded->getPixel(0, 0, 0), 255);
    EXPECT_EQ(loaded->getPixel(0, 0, 1), 0);

    std::filesystem::remove(tmp);
}

// 12. Save empty image returns false
TEST(SaveEmptyImage) {
    Image img;
    EXPECT_FALSE(img.save("empty_output.png"));
}

// 13. Pixel access
TEST(PixelGetSet) {
    Image img(10, 10, 3);
    img.setPixel(5, 7, 0, 42);
    img.setPixel(5, 7, 1, 100);
    img.setPixel(5, 7, 2, 200);
    EXPECT_EQ(img.getPixel(5, 7, 0), 42);
    EXPECT_EQ(img.getPixel(5, 7, 1), 100);
    EXPECT_EQ(img.getPixel(5, 7, 2), 200);
}

// 14. Pixel access — edge clamping on getPixel
TEST(PixelEdgeClamp) {
    Image img(4, 4, 1);
    img.setPixel(0, 0, 0, 10);
    img.setPixel(3, 3, 0, 99);

    // Negative coordinates should clamp to 0
    EXPECT_EQ(img.getPixel(-5, 0, 0), 10);
    EXPECT_EQ(img.getPixel(0, -3, 0), 10);

    // Coordinates beyond bounds should clamp to max
    EXPECT_EQ(img.getPixel(100, 100, 0), 99);
    EXPECT_EQ(img.getPixel(3, 999, 0), 99);
}

// 15. Pixel access — out-of-range setPixel throws
TEST(PixelSetOutOfRange) {
    Image img(4, 4, 3);
    EXPECT_THROW(img.setPixel(-1, 0, 0, 0), std::out_of_range);
    EXPECT_THROW(img.setPixel(0, -1, 0, 0), std::out_of_range);
    EXPECT_THROW(img.setPixel(4, 0, 0, 0), std::out_of_range);
    EXPECT_THROW(img.setPixel(0, 4, 0, 0), std::out_of_range);
}

// 16. Pixel access — channel out-of-range throws
TEST(PixelChannelOutOfRange) {
    Image img(4, 4, 3);
    EXPECT_THROW(img.getPixel(0, 0, 3), std::out_of_range);
    EXPECT_THROW(img.getPixel(0, 0, -1), std::out_of_range);
    EXPECT_THROW(img.setPixel(0, 0, 3, 0), std::out_of_range);
}

// 17. getPixel on empty image throws
TEST(GetPixelEmptyImage) {
    Image img;
    EXPECT_THROW(img.getPixel(0, 0, 0), std::out_of_range);
}

// 18. getPixelAsFloat
TEST(GetPixelAsFloat) {
    Image img(2, 2, 1);
    img.setPixel(0, 0, 0, 128);
    float val = img.getPixelAsFloat(0, 0, 0);
    EXPECT_NEAR(val, 128.0f, 0.001f);
}

// 19. setPixelClamped — values clamped to [0, 255]
TEST(SetPixelClamped) {
    Image img(2, 2, 1);
    img.setPixelClamped(0, 0, 0, 300);
    EXPECT_EQ(img.getPixel(0, 0, 0), 255);

    img.setPixelClamped(1, 0, 0, -50);
    EXPECT_EQ(img.getPixel(1, 0, 0), 0);

    img.setPixelClamped(0, 1, 0, 100);
    EXPECT_EQ(img.getPixel(0, 1, 0), 100);
}

// 20. Clone produces independent deep copy
TEST(Clone) {
    Image img(4, 4, 3);
    img.setPixel(2, 2, 0, 77);

    auto copy = img.clone();
    EXPECT_EQ(copy->getWidth(), 4);
    EXPECT_EQ(copy->getHeight(), 4);
    EXPECT_EQ(copy->getChannels(), 3);
    EXPECT_EQ(copy->getPixel(2, 2, 0), 77);

    // Modify clone — original should be unaffected
    copy->setPixel(2, 2, 0, 200);
    EXPECT_EQ(img.getPixel(2, 2, 0), 77);
    EXPECT_EQ(copy->getPixel(2, 2, 0), 200);
}

// 21. setChannels
TEST(SetChannels) {
    Image img(4, 4, 3);
    EXPECT_EQ(img.getChannels(), 3);
    img.setChannels(1);
    EXPECT_EQ(img.getChannels(), 1);
}

// 22. Raw data access
TEST(RawDataAccess) {
    Image img(2, 2, 3);
    auto& data = img.getData();
    EXPECT_EQ(static_cast<int>(data.size()), 2 * 2 * 3);

    // Write via raw data, read via getPixel
    data[0] = 111;
    EXPECT_EQ(img.getPixel(0, 0, 0), 111);

    // Const version
    const Image& cimg = img;
    const auto& cdata = cimg.getData();
    EXPECT_EQ(cdata[0], 111);
}

// 23. loadFromData — overwrite existing image
TEST(LoadFromDataOverwrite) {
    Image img(2, 2, 1);
    unsigned char newData[3 * 2 * 3];
    std::memset(newData, 42, sizeof(newData));

    img.loadFromData(newData, 3, 2, 3);
    EXPECT_EQ(img.getWidth(), 3);
    EXPECT_EQ(img.getHeight(), 2);
    EXPECT_EQ(img.getChannels(), 3);
    EXPECT_EQ(img.getPixel(0, 0, 0), 42);
}

// 24. loadFromData — invalid channel count throws
TEST(LoadFromDataInvalidChannels) {
    Image img;
    unsigned char dummy[10] = {};
    EXPECT_THROW(img.loadFromData(dummy, 1, 1, 5), std::invalid_argument);
    EXPECT_THROW(img.loadFromData(dummy, 1, 1, 2), std::invalid_argument);
}

// 25. stripAlphaChannel static helper
TEST(StripAlphaChannel) {
    unsigned char rgba[8] = {10, 20, 30, 255, 40, 50, 60, 128};
    auto rgb = Image::stripAlphaChannel(rgba, 2);
    EXPECT_EQ(static_cast<int>(rgb.size()), 6);
    EXPECT_EQ(rgb[0], 10);
    EXPECT_EQ(rgb[1], 20);
    EXPECT_EQ(rgb[2], 30);
    EXPECT_EQ(rgb[3], 40);
    EXPECT_EQ(rgb[4], 50);
    EXPECT_EQ(rgb[5], 60);
}

// 26. RGB <-> HSL round-trip
TEST(RGBtoHSLandBack) {
    // Test several known colours
    struct { unsigned char r, g, b; } colours[] = {
        {255, 0, 0}, {0, 255, 0}, {0, 0, 255},
        {255, 255, 255}, {0, 0, 0}, {128, 128, 128},
        {255, 128, 0}, {64, 128, 192}
    };
    for (auto& c : colours) {
        HSLPixel hsl = Image::RGBtoHSL(c.r, c.g, c.b);
        RGBPixel back = Image::HSLtoRGB(hsl);
        EXPECT_NEAR(static_cast<int>(back.r), static_cast<int>(c.r), 1);
        EXPECT_NEAR(static_cast<int>(back.g), static_cast<int>(c.g), 1);
        EXPECT_NEAR(static_cast<int>(back.b), static_cast<int>(c.b), 1);
    }
}

// 27. RGB -> HSL known values
TEST(RGBtoHSLKnownValues) {
    // Pure red -> H=0, S=1, L=0.5
    HSLPixel hsl = Image::RGBtoHSL(255, 0, 0);
    EXPECT_NEAR(hsl.h, 0.0f, 1.0f);
    EXPECT_NEAR(hsl.s, 1.0f, 0.01f);
    EXPECT_NEAR(hsl.l, 0.5f, 0.01f);

    // White -> L=1
    HSLPixel white = Image::RGBtoHSL(255, 255, 255);
    EXPECT_NEAR(white.l, 1.0f, 0.01f);
    EXPECT_NEAR(white.s, 0.0f, 0.01f);

    // Black -> L=0
    HSLPixel black = Image::RGBtoHSL(0, 0, 0);
    EXPECT_NEAR(black.l, 0.0f, 0.01f);
}

// 28. RGB -> HSV round-trip
TEST(RGBtoHSVandBack) {
    struct { unsigned char r, g, b; } colours[] = {
        {255, 0, 0}, {0, 255, 0}, {0, 0, 255},
        {255, 255, 255}, {0, 0, 0}, {128, 128, 128},
        {200, 100, 50}
    };
    for (auto& c : colours) {
        HSVPixel hsv = Image::RGBtoHSV(c.r, c.g, c.b);
        RGBPixel back = Image::HSVtoRGB(hsv);
        EXPECT_NEAR(static_cast<int>(back.r), static_cast<int>(c.r), 1);
        EXPECT_NEAR(static_cast<int>(back.g), static_cast<int>(c.g), 1);
        EXPECT_NEAR(static_cast<int>(back.b), static_cast<int>(c.b), 1);
    }
}

// 29. RGB -> HSV known values
TEST(RGBtoHSVKnownValues) {
    // Pure red -> H=0, S=1, V=1
    HSVPixel hsv = Image::RGBtoHSV(255, 0, 0);
    EXPECT_NEAR(hsv.h, 0.0f, 1.0f);
    EXPECT_NEAR(hsv.s, 1.0f, 0.01f);
    EXPECT_NEAR(hsv.v, 1.0f, 0.01f);

    // Black -> V=0
    HSVPixel black = Image::RGBtoHSV(0, 0, 0);
    EXPECT_NEAR(black.v, 0.0f, 0.01f);

    // White -> S=0, V=1
    HSVPixel white = Image::RGBtoHSV(255, 255, 255);
    EXPECT_NEAR(white.s, 0.0f, 0.01f);
    EXPECT_NEAR(white.v, 1.0f, 0.01f);
}

// 30. Batch toHSL / fromHSL
TEST(BatchHSLRoundTrip) {
    Image img(4, 4, 3);
    // Fill with a gradient
    for (int y = 0; y < 4; ++y)
        for (int x = 0; x < 4; ++x) {
            img.setPixel(x, y, 0, static_cast<unsigned char>(x * 60));
            img.setPixel(x, y, 1, static_cast<unsigned char>(y * 60));
            img.setPixel(x, y, 2, 100);
        }

    auto origData = img.getData();
    auto hslVec = img.toHSL();
    EXPECT_EQ(static_cast<int>(hslVec.size()), 16);

    img.fromHSL(hslVec);

    // Should match original within rounding tolerance
    for (int i = 0; i < static_cast<int>(origData.size()); ++i) {
        EXPECT_NEAR(static_cast<int>(img.getData()[i]),
                     static_cast<int>(origData[i]), 1);
    }
}

// 31. Batch toHSV / fromHSV round-trip
TEST(BatchHSVRoundTrip) {
    Image img(4, 4, 3);
    for (int y = 0; y < 4; ++y)
        for (int x = 0; x < 4; ++x) {
            img.setPixel(x, y, 0, static_cast<unsigned char>(x * 50 + 10));
            img.setPixel(x, y, 1, static_cast<unsigned char>(y * 50 + 10));
            img.setPixel(x, y, 2, 80);
        }

    auto origData = img.getData();
    auto hsvVec = img.toHSV();
    EXPECT_EQ(static_cast<int>(hsvVec.size()), 16);

    img.fromHSV(hsvVec);

    for (int i = 0; i < static_cast<int>(origData.size()); ++i) {
        EXPECT_NEAR(static_cast<int>(img.getData()[i]),
                     static_cast<int>(origData[i]), 1);
    }
}

// 32. toHSL on non-3-channel image throws
TEST(ToHSLWrongChannels) {
    Image img(4, 4, 1);
    EXPECT_THROW(img.toHSL(), std::runtime_error);
}

// 33. toHSV on non-3-channel image throws
TEST(ToHSVWrongChannels) {
    Image img(4, 4, 1);
    EXPECT_THROW(img.toHSV(), std::runtime_error);
}

// 34. fromHSL size mismatch throws
TEST(FromHSLSizeMismatch) {
    Image img(4, 4, 3);
    std::vector<HSLPixel> wrong(10);  // should be 16
    EXPECT_THROW(img.fromHSL(wrong), std::invalid_argument);
}

// 35. fromHSV size mismatch throws
TEST(FromHSVSizeMismatch) {
    Image img(4, 4, 3);
    std::vector<HSVPixel> wrong(10);
    EXPECT_THROW(img.fromHSV(wrong), std::invalid_argument);
}

// 36. toHSL on empty image throws
TEST(ToHSLEmptyImage) {
    Image img(0, 0, 3);
    EXPECT_THROW(img.toHSL(), std::runtime_error);
}

// 37. Save with unknown extension defaults to PNG
TEST(SaveUnknownExtension) {
    Image img(2, 2, 3);
    img.setPixel(0, 0, 0, 100);
    std::string tmp = "test_output.xyz";
    EXPECT_TRUE(img.save(tmp));  // should default to PNG

    auto loaded = Image::load(tmp);
    EXPECT_TRUE(loaded != nullptr);
    EXPECT_EQ(loaded->getPixel(0, 0, 0), 100);

    std::filesystem::remove(tmp);
}

// 38. Large image allocation
TEST(LargeImage) {
    Image img(1000, 1000, 3);
    EXPECT_EQ(static_cast<int>(img.getData().size()), 1000 * 1000 * 3);
    img.setPixel(999, 999, 2, 42);
    EXPECT_EQ(img.getPixel(999, 999, 2), 42);
}

// 39. Grayscale save/load round-trip
TEST(GrayscaleSaveLoad) {
    Image img(8, 8, 1);
    for (int y = 0; y < 8; ++y)
        for (int x = 0; x < 8; ++x)
            img.setPixel(x, y, 0, static_cast<unsigned char>((x + y) * 10));

    std::string tmp = "test_gray.png";
    EXPECT_TRUE(img.save(tmp));

    auto loaded = Image::load(tmp);
    EXPECT_TRUE(loaded != nullptr);
    EXPECT_EQ(loaded->getChannels(), 1);
    for (int y = 0; y < 8; ++y)
        for (int x = 0; x < 8; ++x)
            EXPECT_EQ(loaded->getPixel(x, y, 0), img.getPixel(x, y, 0));

    std::filesystem::remove(tmp);
}

// 40. HSL hue wrapping — all six HSL sectors
TEST(HSLAllSectors) {
    // H values in each 60° sector
    struct { float h, s, l; } cases[] = {
        {30.0f, 1.0f, 0.5f},   // sector 0-60
        {90.0f, 1.0f, 0.5f},   // sector 60-120
        {150.0f, 1.0f, 0.5f},  // sector 120-180
        {210.0f, 1.0f, 0.5f},  // sector 180-240
        {270.0f, 1.0f, 0.5f},  // sector 240-300
        {330.0f, 1.0f, 0.5f},  // sector 300-360
    };
    for (auto& c : cases) {
        HSLPixel hsl{c.h, c.s, c.l};
        RGBPixel rgb = Image::HSLtoRGB(hsl);
        HSLPixel back = Image::RGBtoHSL(rgb.r, rgb.g, rgb.b);
        EXPECT_NEAR(back.h, c.h, 2.0f);
        EXPECT_NEAR(back.s, c.s, 0.05f);
        EXPECT_NEAR(back.l, c.l, 0.05f);
    }
}

// main — run all registered tests
int main() {
    std::cout << "Running " << testCases().size() << " Image unit tests...\n\n";

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
