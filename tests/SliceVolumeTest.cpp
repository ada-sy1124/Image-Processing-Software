#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "SliceVolume.h"
#include "Image.h"
#include "Volume.h"
#include <cassert>
#include <iostream>
#include <vector>

static void assertEqual(const std::vector<unsigned char>& actual,
                        const std::vector<unsigned char>& expected,
                        const char* label) {
    if (actual != expected) {
        std::cerr << label << " mismatch\nExpected: ";
        for (auto v : expected) std::cerr << static_cast<int>(v) << " ";
        std::cerr << "\nActual:   ";
        for (auto v : actual) std::cerr << static_cast<int>(v) << " ";
        std::cerr << "\n";
        assert(false);
    }
}

int main() {
    // Create three test images
    Image image001(2, 2, 1);
    image001.setPixel(0, 0, 0, 1);
    image001.setPixel(1, 0, 0, 2);
    image001.setPixel(0, 1, 0, 3);
    image001.setPixel(1, 1, 0, 4);

    Image image002(2, 2, 1);
    image002.setPixel(0, 0, 0, 5);
    image002.setPixel(1, 0, 0, 6);
    image002.setPixel(0, 1, 0, 7);
    image002.setPixel(1, 1, 0, 8);

    Image image003(2, 2, 1);
    image003.setPixel(0, 0, 0, 9);
    image003.setPixel(1, 0, 0, 10);
    image003.setPixel(0, 1, 0, 11);
    image003.setPixel(1, 1, 0, 12);

    // Save the images to files
    image001.save("image001.png");
    image002.save("image002.png");
    image003.save("image003.png");

    // Create a SliceVolume and load the images
    SliceVolume volume;

    volume.load("image", 1, 3, ".png");
    volume.rotate(2, 3, 'X');

    // Current rotate('X') crops along X slab and keeps Y/Z full.
    // For this tiny 2x2x3 volume with kernel 3 and coord 2, slab includes all X,
    // so buffer order stays z-major with x as innermost dimension.
    const std::vector<unsigned char>& volumeDataX = volume.getData();
    std::vector<unsigned char> expectedDataX = {
        1, 2, 3, 4,
        5, 6, 7, 8,
        9, 10, 11, 12
    };
    assertEqual(volumeDataX, expectedDataX, "rotate X data");

    // Check extracted YZ slice from center slab index.
    Image sliceX = volume.slice(2);
    assert(sliceX.getWidth() == 2);
    assert(sliceX.getHeight() == 3);
    assert(sliceX.getChannels() == 1);
    std::vector<unsigned char> expectedSliceX = {
        10, 12,
        6, 8,
        2, 4
    };
    assertEqual(sliceX.getData(), expectedSliceX, "slice after rotate X");

    volume.load("image", 1, 3, ".png");
    volume.rotate(2, 3, 'Y');

    // Current rotate('Y') crops along Y slab and keeps X/Z full.
    const std::vector<unsigned char>& volumeDataY = volume.getData();
    std::vector<unsigned char> expectedDataY = {
        1, 2, 3, 4,
        5, 6, 7, 8,
        9, 10, 11, 12
    };
    assertEqual(volumeDataY, expectedDataY, "rotate Y data");

    // Check extracted XZ slice from center slab index.
    Image sliceY = volume.slice(2);
    assert(sliceY.getWidth() == 2);
    assert(sliceY.getHeight() == 3);
    assert(sliceY.getChannels() == 1);
    std::vector<unsigned char> expectedSliceY = {
        11, 12,
        7, 8,
        3, 4
    };
    assertEqual(sliceY.getData(), expectedSliceY, "slice after rotate Y");

    std::cout << "Test passed!" << std::endl;

    return 0;
}