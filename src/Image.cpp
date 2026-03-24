/**
 * /////////////////////////////////////////////////////////
 * BarnesHut
 * /////////////////////////////////////////////////////////
 * @author Akira C T Eisenbeiss  (GitHub: @ada-ace25)
 * @author Ju Lin (GitHub: @ada-jl4025)
 * @author Yichen Liu (GitHub: @ada-yl2425)
 * @author Siyuan Yuan (GitHub: @ada-sy1124)
 * @author Charli Maguire (GitHub: @ada-cm1625)
 * @author Jiacheng Zhao (GitHub: @ada-jz1225)
 * @author Siqi Yao (GitHub: @ada-sy325)
 
 * ---------------------------------------------------------
 
 * @file Image.cpp
 * @brief Implementation of the Image class for image manipulation and I/O.
 * @details This file implements the methods for the Image class, handling 
 * image construction, file loading/saving using stb_image, pixel access 
 * with edge-clamping, and color space conversions (RGB, HSL, HSV).
 */



#include "Image.h"
#include <stdexcept>
#include <algorithm>
#include <cmath>

// stb_image IMPLEMENTATION is defined in main.cpp
#include "stb_image.h"
#include "stb_image_write.h"


// Constructors
Image::Image() : width(0), height(0), channels(0) {}

// We store a 2D image (width * height * channels) in a flat 1D std::vector.
// This ensures continuous memory allocation, which maximizes CPU cache hits and is 
// strictly required when interfacing with C-style libraries like stb_image.
Image::Image(int w, int h, int c)
    : width(w), height(h), channels(c), data(w * h * c, 0) {}

Image::Image(const unsigned char* rawData, int w, int h, int srcChannels)
    : Image() {
    loadFromData(rawData, w, h, srcChannels);
}


// File I/O
std::unique_ptr<Image> Image::load(const std::string& filename) {
    int w, h, fileChannels;
    unsigned char* raw = stbi_load(filename.c_str(), &w, &h, &fileChannels, 0);
    if (!raw) return nullptr;

    auto img = std::make_unique<Image>();

    // stb_image returns 2 channels for Grayscale + Alpha images.
    // Our processing pipeline only strictly supports 1-channel (Gray) or 3-channel (RGB).
    // So, if we see 2 channels, we manually strip the alpha channel to force it into a standard 1-channel format.
    if (fileChannels == 2) {
        img->width    = w;
        img->height   = h;
        img->channels = 1;
        img->data.resize(w * h);
        for (int i = 0; i < w * h; ++i) {
            img->data[i] = raw[i * 2]; // Keep Gray, drop Alpha
        }
        stbi_image_free(raw);
        return img;
    }

    try {
        img->loadFromData(raw, w, h, fileChannels);
    } catch (...) {
        stbi_image_free(raw);
        return nullptr;
    }
    stbi_image_free(raw);
    return img;
}

bool Image::save(const std::string& filename) const {
    if (width <= 0 || height <= 0) return false;

    // Choose format from extension
    std::string ext;
    auto dot = filename.rfind('.');
    if (dot != std::string::npos) {
        ext = filename.substr(dot);
        // Normalize extension to lowercase to gracefully handle cases like ".PNG" vs ".png"
        for (auto& ch : ext) ch = static_cast<char>(std::tolower(ch));
    }

    // Dispatch to the appropriate stb_write function based on extension.
    if (ext == ".png") {
        return stbi_write_png(filename.c_str(), width, height, channels,
                              data.data(), width * channels) != 0;
    } else if (ext == ".jpg" || ext == ".jpeg") {
        // 95 is the JPEG quality parameter
        return stbi_write_jpg(filename.c_str(), width, height, channels,
                              data.data(), 95) != 0;
    } else if (ext == ".bmp") {
        return stbi_write_bmp(filename.c_str(), width, height, channels,
                              data.data()) != 0;
    }

    // Default: save as PNG
    return stbi_write_png(filename.c_str(), width, height, channels,
                          data.data(), width * channels) != 0;
}


// Properties
int  Image::getWidth()    const { return width; }
int  Image::getHeight()   const { return height; }
int  Image::getChannels() const { return channels; }
void Image::setChannels(int c)  { channels = c; }


// Pixel access — unsigned char (with edge-clamp)
unsigned char Image::getPixel(int x, int y, int c) const {
    if (width <= 0 || height <= 0)
        throw std::out_of_range("Cannot read from an empty image");
    if (c < 0 || c >= channels)
        throw std::out_of_range("Channel index out of range");

    // Spatial convolution filters (like blur or edge detection) frequently 
    // attempt to read pixels outside the bounds of the image (e.g., at index -1).
    // By clamping coordinates to [0, width-1], we effectively extend the edge pixels outward.
    // This prevents segmentation faults and stops black borders from appearing around the filtered image.
    x = std::clamp(x, 0, width  - 1);
    y = std::clamp(y, 0, height - 1);

    return data[(y * width + x) * channels + c];
}

void Image::setPixel(int x, int y, int c, unsigned char value) {
    if (x < 0 || x >= width || y < 0 || y >= height)
        throw std::out_of_range("Pixel coordinates out of range");
    if (c < 0 || c >= channels)
        throw std::out_of_range("Channel index out of range");

    data[(y * width + x) * channels + c] = value;
}

void Image::removeAlpha() {
    if (channels != 4) return;
    
    // Most of our filters (like histogram eq or edge detection) 
    // define logic based strictly on color intensity. Including alpha as a 4th channel 
    // in mathematical operations would produce corrupted colors.
    std::vector<unsigned char> newData;
    newData.reserve(width * height * 3);
    for (int i = 0; i < width * height; ++i) {
        newData.push_back(data[i * 4 + 0]); // R
        newData.push_back(data[i * 4 + 1]); // G
        newData.push_back(data[i * 4 + 2]); // B
        // Ignore data[i*4 + 3] (Alpha)
    }
    // Move semantics cleanly transfer ownership without an expensive secondary copy
    data = std::move(newData);
    channels = 3;
}



// Pixel access — wider types for filter/convolution math
float Image::getPixelAsFloat(int x, int y, int c) const {
    // Allows kernels to do floating point math without casting locally everywhere
    return static_cast<float>(getPixel(x, y, c));
}

void Image::setPixelClamped(int x, int y, int c, int value) {
    // Guards against overflow/underflow after convolution additions/subtractions
    setPixel(x, y, c, static_cast<unsigned char>(std::clamp(value, 0, 255)));
}


// Raw data access
std::vector<unsigned char>&       Image::getData()       { return data; }
const std::vector<unsigned char>& Image::getData() const { return data; }

void Image::loadFromData(const unsigned char* rawData, int w, int h, int srcChannels) {
    // rawData is typically managed by stb_image. We copy it into
    // our std::vector so the Image class firmly owns its memory lifecycle (RAII).
    if (srcChannels == 1) {
        width    = w;
        height   = h;
        channels = 1;
        data.assign(rawData, rawData + w * h);
    } else if (srcChannels == 3) {
        width    = w;
        height   = h;
        channels = 3;
        data.assign(rawData, rawData + w * h * 3);
    } else if (srcChannels == 4) {
        width    = w;
        height   = h;
        channels = 3; // Force to RGB
        data     = stripAlphaChannel(rawData, w * h);
    } else {
        throw std::invalid_argument(
            "Image expects 1, 3, or 4 channels, got "
            + std::to_string(srcChannels));
    }
}

std::unique_ptr<Image> Image::clone() const {
    // Provides a safe deep copy for filters that need to read from an unmodified 
    // source while writing to a destination image (e.g., Sharpen).
    auto copy = std::make_unique<Image>(width, height, channels);
    copy->data = data;
    return copy;
}


// RGBA → RGB utility
std::vector<unsigned char> Image::stripAlphaChannel(
        const unsigned char* rgbaData, int pixelCount) {
    std::vector<unsigned char> rgb(pixelCount * 3);
    for (int i = 0; i < pixelCount; ++i) {
        rgb[i * 3 + 0] = rgbaData[i * 4 + 0];
        rgb[i * 3 + 1] = rgbaData[i * 4 + 1];
        rgb[i * 3 + 2] = rgbaData[i * 4 + 2];
    }
    return rgb;
}

void Image::stripAlphaChannel() {
    int pixelCount = width * height;

    std::vector<unsigned char> rgb(pixelCount * 3);

    for (int i = 0; i < pixelCount; ++i) {
        rgb[i * 3 + 0] = data[i * 4 + 0];
        rgb[i * 3 + 1] = data[i * 4 + 1];
        rgb[i * 3 + 2] = data[i * 4 + 2];
    }

    data = rgb;     // replace internal buffer
    channels = 3;   // update channel count
}


// Single-pixel colour-space conversions
// Math based on standard RGB to HSL/HSV cylinder mapping formulas
HSLPixel Image::RGBtoHSL(unsigned char r_val, unsigned char g_val, unsigned char b_val) {
    float r = r_val / 255.0f;
    float g = g_val / 255.0f;
    float b = b_val / 255.0f;

    float cmax  = std::max({r, g, b});
    float cmin  = std::min({r, g, b});
    float delta = cmax - cmin;

    HSLPixel hsl;
    hsl.l = (cmax + cmin) / 2.0f;

    if (delta == 0.0f) {
        hsl.s = 0.0f;
        hsl.h = 0.0f;
    } else {
        hsl.s = delta / (1.0f - std::abs(2.0f * hsl.l - 1.0f));
        if (cmax == r)
            hsl.h = 60.0f * std::fmod(((g - b) / delta), 6.0f);
        else if (cmax == g)
            hsl.h = 60.0f * (((b - r) / delta) + 2.0f);
        else
            hsl.h = 60.0f * (((r - g) / delta) + 4.0f);
        if (hsl.h < 0.0f) hsl.h += 360.0f;
    }
    return hsl;
}

RGBPixel Image::HSLtoRGB(const HSLPixel& hsl) {
    float c = (1.0f - std::abs(2.0f * hsl.l - 1.0f)) * hsl.s;
    float x = c * (1.0f - std::abs(std::fmod(hsl.h / 60.0f, 2.0f) - 1.0f));
    float m = hsl.l - c / 2.0f;

    float rp = 0, gp = 0, bp = 0;
    if      (hsl.h <  60) { rp = c; gp = x; }
    else if (hsl.h < 120) { rp = x; gp = c; }
    else if (hsl.h < 180) { gp = c; bp = x; }
    else if (hsl.h < 240) { gp = x; bp = c; }
    else if (hsl.h < 300) { rp = x; bp = c; }
    else                   { rp = c; bp = x; }

    RGBPixel rgb;
    rgb.r = static_cast<unsigned char>(std::round((rp + m) * 255.0f));
    rgb.g = static_cast<unsigned char>(std::round((gp + m) * 255.0f));
    rgb.b = static_cast<unsigned char>(std::round((bp + m) * 255.0f));
    return rgb;
}

HSVPixel Image::RGBtoHSV(unsigned char r_val, unsigned char g_val, unsigned char b_val) {
    float r = r_val / 255.0f;
    float g = g_val / 255.0f;
    float b = b_val / 255.0f;

    float cmax  = std::max({r, g, b});
    float cmin  = std::min({r, g, b});
    float delta = cmax - cmin;

    HSVPixel hsv;
    hsv.v = cmax;
    hsv.s = (cmax == 0.0f) ? 0.0f : delta / cmax;

    if (delta == 0.0f) {
        hsv.h = 0.0f;
    } else {
        if (cmax == r)
            hsv.h = 60.0f * std::fmod(((g - b) / delta), 6.0f);
        else if (cmax == g)
            hsv.h = 60.0f * (((b - r) / delta) + 2.0f);
        else
            hsv.h = 60.0f * (((r - g) / delta) + 4.0f);
        if (hsv.h < 0.0f) hsv.h += 360.0f;
    }
    return hsv;
}

RGBPixel Image::HSVtoRGB(const HSVPixel& hsv) {
    float c = hsv.v * hsv.s;
    float x = c * (1.0f - std::abs(std::fmod(hsv.h / 60.0f, 2.0f) - 1.0f));
    float m = hsv.v - c;

    float rp = 0, gp = 0, bp = 0;
    if      (hsv.h <  60) { rp = c; gp = x; }
    else if (hsv.h < 120) { rp = x; gp = c; }
    else if (hsv.h < 180) { gp = c; bp = x; }
    else if (hsv.h < 240) { gp = x; bp = c; }
    else if (hsv.h < 300) { rp = x; bp = c; }
    else                   { rp = c; bp = x; }

    RGBPixel rgb;
    rgb.r = static_cast<unsigned char>(std::round((rp + m) * 255.0f));
    rgb.g = static_cast<unsigned char>(std::round((gp + m) * 255.0f));
    rgb.b = static_cast<unsigned char>(std::round((bp + m) * 255.0f));
    return rgb;
}



// Batch colour-space conversions (require channels == 3)
// For filters like Histogram Equalization, doing pixel-by-pixel 
// conversion back and forth inside the algorithm loop is computationally heavy 
// and bad for cache locality. Converting the whole image into a contiguous 
// vector of HSV/HSL structs at once is significantly more efficient.
std::vector<HSLPixel> Image::toHSL() const {
    if (channels != 3)
        throw std::runtime_error("toHSL requires a 3-channel (RGB) image");
    if (width <= 0 || height <= 0)
        throw std::runtime_error("toHSL: image is empty");

    int n = width * height;
    std::vector<HSLPixel> result(n);
    for (int i = 0; i < n; ++i)
        result[i] = RGBtoHSL(data[i*3], data[i*3+1], data[i*3+2]);
    return result;
}

std::vector<HSVPixel> Image::toHSV() const {
    if (channels != 3)
        throw std::runtime_error("toHSV requires a 3-channel (RGB) image");
    if (width <= 0 || height <= 0)
        throw std::runtime_error("toHSV: image is empty");

    int n = width * height;
    std::vector<HSVPixel> result(n);
    for (int i = 0; i < n; ++i)
        result[i] = RGBtoHSV(data[i*3], data[i*3+1], data[i*3+2]);
    return result;
}

void Image::fromHSL(const std::vector<HSLPixel>& hslData) {
    if (channels != 3)
        throw std::runtime_error("fromHSL requires a 3-channel (RGB) image");
    int n = width * height;
    if (static_cast<int>(hslData.size()) != n)
        throw std::invalid_argument(
            "fromHSL: vector size mismatch ("
            + std::to_string(hslData.size()) + " vs " + std::to_string(n) + ")");

    for (int i = 0; i < n; ++i) {
        RGBPixel rgb = HSLtoRGB(hslData[i]);
        data[i*3]     = rgb.r;
        data[i*3 + 1] = rgb.g;
        data[i*3 + 2] = rgb.b;
    }
}

void Image::fromHSV(const std::vector<HSVPixel>& hsvData) {
    if (channels != 3)
        throw std::runtime_error("fromHSV requires a 3-channel (RGB) image");
    int n = width * height;
    if (static_cast<int>(hsvData.size()) != n)
        throw std::invalid_argument(
            "fromHSV: vector size mismatch ("
            + std::to_string(hsvData.size()) + " vs " + std::to_string(n) + ")");

    for (int i = 0; i < n; ++i) {
        RGBPixel rgb = HSVtoRGB(hsvData[i]);
        data[i*3]     = rgb.r;
        data[i*3 + 1] = rgb.g;
        data[i*3 + 2] = rgb.b;
    }
}
