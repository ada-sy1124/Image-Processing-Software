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
 
 * @file SimpleFilter.cpp
 * @brief Implementation of simple point-process image filters.
 * @details This file implements various basic image adjustments including 
 * grayscale conversion, manual and automatic brightness adjustment, histogram 
 * equalization (supporting Grayscale and RGB via HSV/HSL), binary thresholding, 
 * and salt-and-pepper noise injection.
 */


#include "SimpleFilter.h"
#include "Image.h"
#include <stdexcept>
#include <algorithm>
#include <cmath>
#include <iostream>  
#include <cstdlib>   

// GreyscaleFilter

void GreyscaleFilter::apply(Image& image) {
    int w = image.getWidth();
    int h = image.getHeight();
    int c = image.getChannels();

    // If already a single-channel grayscale image, do nothing to save CPU cycles
    if (c == 1) return;

    // 1. Prepare a new container for single-channel data
    // Pre-allocating the exact capacity prevents the vector from 
    // dynamically reallocating and copying data multiple times during the loop.
    std::vector<unsigned char> grayData;
    grayData.reserve(w * h);

    // 2. Iterate through each pixel and convert RGB to a single Y value
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            unsigned char r = image.getPixel(x, y, 0);
            unsigned char g = image.getPixel(x, y, 1);
            unsigned char b = image.getPixel(x, y, 2);

            // Luminance: Y = 0.2126R + 0.7152G + 0.0722B
            // These are the standard ITU-R BT.709 coefficients. 
            // The human eye is most sensitive to green light and least sensitive to blue. 
            // This weighted sum provides a perceptually accurate grayscale conversion 
            // rather than just doing a simple average ((R+G+B)/3).
            float gray = 0.2126f * r + 0.7152f * g + 0.0722f * b;

            grayData.push_back(static_cast<unsigned char>(
                std::clamp(static_cast<int>(gray + 0.5f), 0, 255)));
        }
    }

    // 3. Update the image object's internal data and channel count
    // Move semantics cleanly transfer ownership without an expensive deep copy.
    image.getData() = std::move(grayData);
    image.setChannels(1);
}

// BrightnessFilter

BrightnessFilter::BrightnessFilter(int value) : value(value), autoMode(false) {}

BrightnessFilter::BrightnessFilter() : value(0), autoMode(true) {}

void BrightnessFilter::apply(Image& image) {
    int w = image.getWidth();
    int h = image.getHeight();
    int c = image.getChannels();

    if (w <= 0 || h <= 0 || c <= 0) {
        throw std::runtime_error("BrightnessFilter: invalid image dimensions/channels.");
    }

    int delta = value;

    // Auto mode: compute mean intensity and shift it to 128
    if (autoMode) {
        long long sum = 0;
        long long count = 0;

        // Calculate the global average intensity of the image
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                if (c == 1) {
                    sum += image.getPixel(x, y, 0);
                    ++count;
                } else {
                    int r = image.getPixel(x, y, 0);
                    int g = image.getPixel(x, y, 1);
                    int b = image.getPixel(x, y, 2);
                    // Use a simple average for brightness estimation here
                    sum += (r + g + b) / 3;
                    ++count;
                }
            }
        }

        if (count == 0) {
            throw std::runtime_error("BrightnessFilter: empty image.");
        }

        int mean = static_cast<int>(sum / count);
        
        // 128 is the exact midpoint of the 8-bit color space (0-255). 
        // By shifting the mean intensity to 128, we attempt to maximize the available 
        // dynamic range, naturally correcting both overexposed and underexposed images.
        delta = 128 - mean;
    }

    // Apply delta to all channels
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            for (int ch = 0; ch < c; ++ch) {
                int v = static_cast<int>(image.getPixel(x, y, ch)) + delta;
                // Clamp prevents wrap-around artifacts (e.g., 255 + 5 becoming 4)
                v = std::clamp(v, 0, 255);
                image.setPixel(x, y, ch, static_cast<unsigned char>(v));
            }
        }
    }
}

// EqualizeHistogram

EqualizeHistogram::EqualizeHistogram(const std::string& type)
    : type(type) {}

void EqualizeHistogram::apply(Image& img) {
    // Histogram equalization stretches the intensity values. 
    // If we included the alpha channel, transparent pixels might become opaque 
    // and vice versa. We must strip it out.
    if (img.getChannels() == 4) {
        img.stripAlphaChannel(); 
        img.removeAlpha(); 
    }

    int ch = img.getChannels();

    // Route to the appropriate mathematical method based on channel count
    if (ch == 1) {
        applyGrayscale(img);
    } 
    else if (ch == 3) { 
        applyRGB(img); 
    }
    else {
        throw std::runtime_error("Unsupported number of channels for histogram equalisation");
    }
}


// Greyscale

void EqualizeHistogram::applyGrayscale(Image& img) {
    int total = img.getWidth() * img.getHeight();
    std::vector<int> hist(256, 0);

    const std::vector<unsigned char>& inData = img.getData();
    std::vector<unsigned char> outData = inData;

    // 1. Build the frequency histogram (count occurrences of each pixel value)
    for (auto px : inData) hist[px]++;

    // 2. Build mapping using the Cumulative Distribution Function (CDF)
    // The CDF calculates the running total of probabilities. By mapping 
    // pixel values to their CDF, we spread out the most frequent intensity values 
    // across the entire 0-255 range, maximizing contrast.
    std::vector<unsigned char> map(256,0);
    int cumsum = 0;
    for (int i=0;i<256;i++){
        cumsum += hist[i];
        map[i] = std::min(255, int(cumsum*255.0/total));
    }

    // 3. Apply the CDF mapping to every pixel
    for (int i=0;i<total;i++) outData[i] = map[inData[i]];

    img.getData() = outData;
}


// RGB

void EqualizeHistogram::applyRGB(Image& img) {
    int total = img.getWidth() * img.getHeight();

    // If we apply histogram equalization to the R, G, and B channels 
    // independently, their relative ratios will change, completely destroying the 
    // original colors (color shifting). We MUST convert to a color space like HSV or HSL, 
    // apply the equalization ONLY to the Value/Lightness channel, and then convert back 
    // to preserve the original Hues.
    if (type == "HSV") {
        std::vector<HSVPixel> hsv = img.toHSV();

        // Build histogram exclusively on the V (Value) channel
        std::vector<int> hist(256, 0);
        for (auto& px : hsv) hist[std::min(255, int(px.v * 255))]++;

        // Build CDF mapping
        std::vector<unsigned char> map(256, 0);
        int cumsum = 0;
        for (int i = 0; i < 256; ++i) {
            cumsum += hist[i];
            map[i] = std::min(255, cumsum * 255 / total);
        }

        // Apply mapping and scale back down to the [0.0, 1.0] float range expected by HSVPixel
        for (auto& px : hsv) {
            int idx = std::min(255, int(px.v * 255));
            px.v = map[idx] / 255.0f;
        }
        img.fromHSV(hsv);

    } else {
        std::vector<HSLPixel> hsl = img.toHSL();

        // Build histogram exclusively on the L (Lightness) channel
        std::vector<int> hist(256, 0);
        for (auto& px : hsl) hist[std::min(255, int(px.l * 255))]++;

        // Build CDF mapping
        std::vector<unsigned char> map(256, 0);
        int cumsum = 0;
        for (int i = 0; i < 256; ++i) {
            cumsum += hist[i];
            map[i] = std::min(255, cumsum * 255 / total);
        }

        // Apply mapping and scale back down to [0.0, 1.0]
        for (auto& px : hsl) {
            int idx = std::min(255, int(px.l * 255));
            px.l = map[idx] / 255.0f;
        }
        img.fromHSL(hsl);
    }
}

// ThresholdFilter
// constructor
ThresholdFilter::ThresholdFilter(int threshold, const std::string& type)
    : threshold(threshold), type(type) {}

void ThresholdFilter::apply(Image& image) {
    if (image.getChannels() == 1) {
        applyGrayscale(image);
    } else {
        applyRGB(image);
    }
}

// for grayscale images, threshold the single intensity channel
void ThresholdFilter::applyGrayscale(Image& image) {
    int width = image.getWidth();
    int height = image.getHeight();

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            unsigned char intensity = image.getPixel(x, y, 0);
            // Binarization: Everything equal or above the threshold becomes pure white (255), 
            // everything below becomes pure black (0).
            unsigned char outValue = (intensity >= threshold) ? 255 : 0;
            image.setPixel(x, y, 0, outValue);
        }
    }
}

// applies thresholding to the intensity component of an RGB image
void ThresholdFilter::applyRGB(Image& image) {
    int width = image.getWidth();
    int height = image.getHeight();


    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
    
            unsigned char r = image.getPixel(x, y, 0);
            unsigned char g = image.getPixel(x, y, 1);
            unsigned char b = image.getPixel(x, y, 2);

            unsigned char intensity = 0;

            // Convert RGB to an intensity metric based on the chosen colour space
            // This ensures we evaluate how "bright" the pixel is perceptually.
            if (type == "HSL") {
                HSLPixel hsl = Image::RGBtoHSL(r, g, b);
                intensity = static_cast<unsigned char>(hsl.l * 255.0f);
            } 
            else {
                HSVPixel hsv = Image::RGBtoHSV(r, g, b);
                intensity = static_cast<unsigned char>(hsv.v * 255.0f);
            }

            // threshold the intensity and set all channels to the output value
            // By forcing R, G, and B to be exactly the same (either all 0 or all 255), 
            // we guarantee the output image is strictly a binary black-and-white image.
            unsigned char outValue = (intensity >= threshold) ? 255 : 0;
            
            image.setPixel(x, y, 0, outValue);
            image.setPixel(x, y, 1, outValue);
            image.setPixel(x, y, 2, outValue);
        }
    }
}


// SaltPepperFilter

// Constructor receives the raw input amount for strict validation
SaltPepperFilter::SaltPepperFilter(int inputAmount) : amount(inputAmount) {
    // Strict boundary check: noise amount must be a valid percentage (0-100)
    if (amount < 0 || amount > 100) {
        // Output to the standard error stream (std::cerr) 
        std::cerr << "Invalid salt-and-pepper noise parameter!" << std::endl;
        std::cerr << "Expected range is 0 to 100, but you entered: " << amount << std::endl;
        std::cerr << "Terminating program." << std::endl;
        
        // Immediately terminate the program and return a failure status code to the OS
        std::exit(EXIT_FAILURE); 
    }
}

void SaltPepperFilter::apply(Image& image) {
    // At this point, amount is guaranteed to be valid (0-100).
    // If the amount is 0, do nothing to save CPU cycles.
    if (amount == 0) return;

    int width = image.getWidth();
    int height = image.getHeight();
    int channels = image.getChannels();

    // Ensure we only apply noise to the color channels (RGB) 
    // and leave the alpha channel (if it exists) untouched.
    int applyChannels = (channels >= 3) ? 3 : channels; 

    // Iterate through each pixel in the image
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            
            // Generate a random number between 0-99. 
            // This ensures each pixel has exactly an 'amount'% probability of mutating.
            if (std::rand() % 100 < amount) {
                
                // Randomly decide whether the noise is salt (pure white: 255) or pepper (pure black: 0)
                unsigned char noiseValue = (std::rand() % 2 == 0) ? 255 : 0;

                for (int c = 0; c < applyChannels; ++c) {
                    image.setPixel(x, y, c, noiseValue);
                }
            }
        }
    }
}