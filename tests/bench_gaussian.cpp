#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "Image.h"
#include "ConvolutionalFilter.h"
#include <chrono>
#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <memory>

static double benchGaussian(const Image& src, int kernelSize, double sigma, int runs) {
    double totalMs = 0.0;
    for (int r = 0; r < runs; ++r) {
        auto img = src.clone();
        GaussianBlurFilter filter(kernelSize, sigma);

        auto t0 = std::chrono::high_resolution_clock::now();
        filter.apply(*img);
        auto t1 = std::chrono::high_resolution_clock::now();

        double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
        totalMs += ms;
    }
    return totalMs / runs;
}

int main(int argc, char* argv[]) {
    std::string path = "../Images/stinkbug.png";
    if (argc > 1) path = argv[1];

    auto imgPtr = Image::load(path);
    if (!imgPtr) {
        std::cerr << "Failed to load " << path << "\n";
        return 1;
    }
    Image& img = *imgPtr;

    int w = img.getWidth();
    int h = img.getHeight();
    int ch = img.getChannels();
    std::cout << "Image: " << path << "  " << w << "x" << h << "x" << ch << "\n";
    std::cout << std::string(60, '-') << "\n";
    std::cout << std::setw(10) << "Kernel"
              << std::setw(10) << "Sigma"
              << std::setw(6)  << "Runs"
              << std::setw(14) << "Avg (ms)"
              << std::setw(16) << "Mpix/s"
              << "\n";
    std::cout << std::string(60, '-') << "\n";

    double megapixels = (double)(w * h * ch) / 1e6;

    struct TestCase { int k; double sigma; int runs; };
    std::vector<TestCase> cases = {
        {3,  1.0, 20},
        {5,  2.0, 20},
        {7,  2.0, 15},
        {9,  3.0, 10},
        {15, 4.0,  5},
        {21, 5.0,  3},
        {31, 6.0,  3},
        {51, 8.0,  2},
    };

    for (auto& tc : cases) {
        double avgMs = benchGaussian(img, tc.k, tc.sigma, tc.runs);
        double mpixPerSec = megapixels / (avgMs / 1000.0);
        std::cout << std::setw(10) << tc.k
                  << std::setw(10) << std::fixed << std::setprecision(1) << tc.sigma
                  << std::setw(6)  << tc.runs
                  << std::setw(14) << std::setprecision(2) << avgMs
                  << std::setw(16) << std::setprecision(2) << mpixPerSec
                  << "\n";
    }

    std::cout << std::string(60, '-') << "\n";
    return 0;
}
