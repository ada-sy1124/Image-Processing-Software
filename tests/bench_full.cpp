#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "Image.h"
#include "Volume.h"
#include "Projection.h"
#include "Slice.h"
#include "SimpleFilter.h"
#include "ConvolutionalFilter.h"
#include "Filters3D.h"
#include <chrono>
#include <iostream>
#include <iomanip>
#include <functional>
#include <vector>
#include <string>
#include <numeric>
#include <algorithm>
#include <cmath>
#include <memory>

using Clock = std::chrono::high_resolution_clock;

struct BenchResult {
    std::string section;
    std::string name;
    std::string size;
    int runs;
    double avgMs;
    double minMs;
    double maxMs;
    double stddevMs;
    double throughput;
    std::string throughputUnit;
};

static std::vector<BenchResult> allResults;

static std::vector<double> runBench(std::function<void()> fn, int runs) {
    std::vector<double> times;
    times.reserve(runs);
    for (int r = 0; r < runs; r++) {
        auto t0 = Clock::now();
        fn();
        auto t1 = Clock::now();
        times.push_back(std::chrono::duration<double, std::milli>(t1 - t0).count());
    }
    return times;
}

static void record(const std::string& section, const std::string& name,
                   const std::string& size, const std::vector<double>& times,
                   double dataSize, const std::string& unit) {
    double sum = std::accumulate(times.begin(), times.end(), 0.0);
    double avg = sum / times.size();
    double mn = *std::min_element(times.begin(), times.end());
    double mx = *std::max_element(times.begin(), times.end());
    double sq = 0;
    for (auto t : times) sq += (t - avg) * (t - avg);
    double sd = times.size() > 1 ? std::sqrt(sq / (times.size() - 1)) : 0.0;
    double tp = dataSize / (avg / 1000.0);
    allResults.push_back({section, name, size, (int)times.size(), avg, mn, mx, sd, tp, unit});
}

static void printSection(const std::string& title) {
    std::cout << "\n" << std::string(100, '=') << "\n";
    std::cout << "  " << title << "\n";
    std::cout << std::string(100, '=') << "\n";
}

static void printHeader() {
    std::cout << std::setw(22) << "Test"
              << std::setw(12) << "Size"
              << std::setw(6)  << "Runs"
              << std::setw(12) << "Avg(ms)"
              << std::setw(12) << "Min(ms)"
              << std::setw(12) << "Max(ms)"
              << std::setw(12) << "StdDev"
              << std::setw(14) << "Throughput"
              << "\n";
    std::cout << std::string(100, '-') << "\n";
}

static void printRow(const BenchResult& r) {
    std::cout << std::setw(22) << r.name
              << std::setw(12) << r.size
              << std::setw(6)  << r.runs
              << std::setw(12) << std::fixed << std::setprecision(2) << r.avgMs
              << std::setw(12) << std::setprecision(2) << r.minMs
              << std::setw(12) << std::setprecision(2) << r.maxMs
              << std::setw(12) << std::setprecision(2) << r.stddevMs
              << std::setw(10) << std::setprecision(2) << r.throughput
              << " " << r.throughputUnit
              << "\n";
}

static void fillRandomImg(Image& img) {
    auto& d = img.getData();
    unsigned int seed = 42;
    for (size_t i = 0; i < d.size(); i++) {
        seed = seed * 1103515245u + 12345u;
        d[i] = static_cast<unsigned char>((seed >> 16) & 0xFF);
    }
}

static void fillRandomVol(Volume& vol) {
    auto& d = vol.getData();
    unsigned int seed = 12345;
    for (size_t i = 0; i < d.size(); i++) {
        seed = seed * 1103515245u + 12345u;
        d[i] = static_cast<unsigned char>((seed >> 16) & 0xFF);
    }
}

// ============================================================
// SECTION 1: Image construction & I/O
// ============================================================
static void benchImageBasics() {
    printSection("1. IMAGE CONSTRUCTION & I/O");
    printHeader();

    struct SzCase { int w; int h; int ch; int runs; };
    std::vector<SzCase> sizes = {{256,256,1,50},{256,256,3,50},{512,512,3,30},
                                  {1024,1024,3,20},{2048,2048,3,10},{4096,4096,3,5}};
    for (auto& s : sizes) {
        std::string sz = std::to_string(s.w)+"x"+std::to_string(s.h)+"x"+std::to_string(s.ch);
        double mpix = (double)s.w * s.h / 1e6;
        {
            auto times = runBench([&]() { Image img(s.w, s.h, s.ch); }, s.runs);
            record("Image","Construct",sz,times,mpix,"Mpix/s");
            printRow(allResults.back());
        }
        {
            Image src(s.w, s.h, s.ch); fillRandomImg(src);
            auto times = runBench([&]() { auto c = src.clone(); }, s.runs);
            record("Image","Clone",sz,times,mpix,"Mpix/s");
            printRow(allResults.back());
        }
    }
    // save/load benchmark with temp file
    {
        Image img(1024, 1024, 3); fillRandomImg(img);
        auto times = runBench([&]() { img.save("/tmp/bench_test.bmp"); }, 10);
        record("Image","Save BMP","1024x1024x3",times,1.048576,"Mpix/s");
        printRow(allResults.back());
    }
    {
        Image img(1024, 1024, 3); fillRandomImg(img);
        img.save("/tmp/bench_test.png");
        auto times = runBench([&]() { auto p = Image::load("/tmp/bench_test.png"); }, 10);
        record("Image","Load PNG","1024x1024x3",times,1.048576,"Mpix/s");
        printRow(allResults.back());
    }
}

// ============================================================
// SECTION 2: Pixel access patterns
// ============================================================
static void benchPixelAccess() {
    printSection("2. PIXEL ACCESS PATTERNS");
    printHeader();

    struct SzCase { int w; int h; int runs; };
    std::vector<SzCase> sizes = {{512,512,20},{1024,1024,10},{2048,2048,5}};
    for (auto& s : sizes) {
        std::string sz = std::to_string(s.w)+"x"+std::to_string(s.h);
        double mpix = (double)s.w * s.h / 1e6;
        Image img(s.w, s.h, 3); fillRandomImg(img);
        {
            volatile unsigned char sink = 0;
            auto times = runBench([&]() {
                for (int y = 0; y < s.h; y++)
                    for (int x = 0; x < s.w; x++)
                        sink = img.getPixel(x, y, 0);
            }, s.runs);
            record("PixelAccess","getPixel seq",sz,times,mpix,"Mpix/s");
            printRow(allResults.back());
        }
        {
            auto times = runBench([&]() {
                for (int y = 0; y < s.h; y++)
                    for (int x = 0; x < s.w; x++)
                        img.setPixel(x, y, 0, 128);
            }, s.runs);
            record("PixelAccess","setPixel seq",sz,times,mpix,"Mpix/s");
            printRow(allResults.back());
        }
        {
            volatile float sink = 0;
            auto times = runBench([&]() {
                for (int y = 0; y < s.h; y++)
                    for (int x = 0; x < s.w; x++)
                        sink = img.getPixelAsFloat(x, y, 0);
            }, s.runs);
            record("PixelAccess","getPixelFloat",sz,times,mpix,"Mpix/s");
            printRow(allResults.back());
        }
    }
}

// ============================================================
// SECTION 3: Colour space conversions
// ============================================================
static void benchColourConversions() {
    printSection("3. COLOUR SPACE CONVERSIONS");
    printHeader();

    struct SzCase { int w; int h; int runs; };
    std::vector<SzCase> sizes = {{256,256,30},{512,512,15},{1024,1024,5},{2048,2048,3}};
    for (auto& s : sizes) {
        std::string sz = std::to_string(s.w)+"x"+std::to_string(s.h);
        double mpix = (double)s.w * s.h / 1e6;
        Image img(s.w, s.h, 3); fillRandomImg(img);
        {
            auto times = runBench([&]() { auto hsl = img.toHSL(); }, s.runs);
            record("Colour","RGB->HSL",sz,times,mpix,"Mpix/s");
            printRow(allResults.back());
        }
        {
            auto times = runBench([&]() { auto hsv = img.toHSV(); }, s.runs);
            record("Colour","RGB->HSV",sz,times,mpix,"Mpix/s");
            printRow(allResults.back());
        }
        {
            auto hsl = img.toHSL();
            auto times = runBench([&]() { img.fromHSL(hsl); }, s.runs);
            record("Colour","HSL->RGB",sz,times,mpix,"Mpix/s");
            printRow(allResults.back());
        }
        {
            auto hsv = img.toHSV();
            auto times = runBench([&]() { img.fromHSV(hsv); }, s.runs);
            record("Colour","HSV->RGB",sz,times,mpix,"Mpix/s");
            printRow(allResults.back());
        }
    }
}


// ============================================================
// SECTION 4: Simple Filters (Greyscale, Brightness, Histogram, Threshold)
// ============================================================
static void benchSimpleFilters() {
    printSection("4. SIMPLE FILTERS");
    printHeader();

    struct SzCase { int w; int h; int runs; };
    std::vector<SzCase> sizes = {{512,512,20},{1024,1024,10},{2048,2048,5},{4096,4096,2}};

    for (auto& s : sizes) {
        std::string sz = std::to_string(s.w)+"x"+std::to_string(s.h);
        double mpix = (double)s.w * s.h / 1e6;

        // Greyscale filter on RGB image
        {
            Image img(s.w, s.h, 3); fillRandomImg(img);
            GreyscaleFilter f;
            auto times = runBench([&]() {
                auto c = img.clone(); f.apply(*c);
            }, s.runs);
            record("SimpleFilter","Greyscale",sz,times,mpix,"Mpix/s");
            printRow(allResults.back());
        }
        // Brightness +50
        {
            Image img(s.w, s.h, 3); fillRandomImg(img);
            BrightnessFilter f(50);
            auto times = runBench([&]() {
                auto c = img.clone(); f.apply(*c);
            }, s.runs);
            record("SimpleFilter","Brightness+50",sz,times,mpix,"Mpix/s");
            printRow(allResults.back());
        }
        // Brightness auto
        {
            Image img(s.w, s.h, 3); fillRandomImg(img);
            BrightnessFilter f;
            auto times = runBench([&]() {
                auto c = img.clone(); f.apply(*c);
            }, s.runs);
            record("SimpleFilter","BrightnessAuto",sz,times,mpix,"Mpix/s");
            printRow(allResults.back());
        }
        // Histogram equalisation HSV
        {
            Image img(s.w, s.h, 3); fillRandomImg(img);
            EqualizeHistogram f("HSV");
            auto times = runBench([&]() {
                auto c = img.clone(); f.apply(*c);
            }, s.runs);
            record("SimpleFilter","HistEq HSV",sz,times,mpix,"Mpix/s");
            printRow(allResults.back());
        }
        // Histogram equalisation HSL
        {
            Image img(s.w, s.h, 3); fillRandomImg(img);
            EqualizeHistogram f("HSL");
            auto times = runBench([&]() {
                auto c = img.clone(); f.apply(*c);
            }, s.runs);
            record("SimpleFilter","HistEq HSL",sz,times,mpix,"Mpix/s");
            printRow(allResults.back());
        }
        // Threshold HSV
        {
            Image img(s.w, s.h, 3); fillRandomImg(img);
            ThresholdFilter f(128, "HSV");
            auto times = runBench([&]() {
                auto c = img.clone(); f.apply(*c);
            }, s.runs);
            record("SimpleFilter","Threshold128",sz,times,mpix,"Mpix/s");
            printRow(allResults.back());
        }
        // Salt & Pepper 10%
        {
            Image img(s.w, s.h, 3); fillRandomImg(img);
            SaltPepperFilter f(10);
            auto times = runBench([&]() {
                auto c = img.clone(); f.apply(*c);
            }, s.runs);
            record("SimpleFilter","Salt&Pepper10%",sz,times,mpix,"Mpix/s");
            printRow(allResults.back());
        }
        // Greyscale on greyscale (single channel)
        {
            Image img(s.w, s.h, 1); fillRandomImg(img);
            GreyscaleFilter f;
            auto times = runBench([&]() {
                auto c = img.clone(); f.apply(*c);
            }, s.runs);
            record("SimpleFilter","Greyscale(1ch)",sz,times,mpix,"Mpix/s");
            printRow(allResults.back());
        }
    }
}

// ============================================================
// SECTION 5: Convolutional Filters (Box, Gaussian, Median, Edge, etc.)
// ============================================================
static void benchConvFilters() {
    printSection("5. CONVOLUTIONAL FILTERS");
    printHeader();

    struct SzCase { int w; int h; };
    std::vector<SzCase> sizes = {{512,512},{1024,1024},{2048,2048}};

    for (auto& s : sizes) {
        std::string sz = std::to_string(s.w)+"x"+std::to_string(s.h);
        double mpix = (double)s.w * s.h / 1e6;
        int runs = (s.w <= 512) ? 10 : (s.w <= 1024 ? 5 : 2);

        Image img(s.w, s.h, 3); fillRandomImg(img);

        // Box blur various sizes
        for (int k : {3, 5, 9, 15, 31}) {
            BoxBlurFilter f(k);
            auto times = runBench([&]() {
                auto c = img.clone(); f.apply(*c);
            }, runs);
            record("ConvFilter","BoxBlur k="+std::to_string(k),sz,times,mpix,"Mpix/s");
            printRow(allResults.back());
        }
        // Gaussian blur various sizes
        for (int k : {3, 5, 9, 15, 31, 51}) {
            double sigma = k / 3.0;
            GaussianBlurFilter f(k, sigma);
            auto times = runBench([&]() {
                auto c = img.clone(); f.apply(*c);
            }, runs);
            record("ConvFilter","Gaussian k="+std::to_string(k),sz,times,mpix,"Mpix/s");
            printRow(allResults.back());
        }
        // Median blur various sizes
        for (int k : {3, 5, 7, 9}) {
            MedianBlurFilter f(k);
            int medRuns = std::max(1, runs / 2);
            auto times = runBench([&]() {
                auto c = img.clone(); f.apply(*c);
            }, medRuns);
            record("ConvFilter","Median k="+std::to_string(k),sz,times,mpix,"Mpix/s");
            printRow(allResults.back());
        }
        // Sharpen
        {
            SharpenFilter f;
            auto times = runBench([&]() {
                auto c = img.clone(); f.apply(*c);
            }, runs);
            record("ConvFilter","Sharpen",sz,times,mpix,"Mpix/s");
            printRow(allResults.back());
        }
        // Edge detectors
        {
            SobelFilter f;
            auto times = runBench([&]() {
                auto c = img.clone(); f.apply(*c);
            }, runs);
            record("ConvFilter","Sobel",sz,times,mpix,"Mpix/s");
            printRow(allResults.back());
        }
        {
            PrewittFilter f;
            auto times = runBench([&]() {
                auto c = img.clone(); f.apply(*c);
            }, runs);
            record("ConvFilter","Prewitt",sz,times,mpix,"Mpix/s");
            printRow(allResults.back());
        }
        {
            ScharrFilter f;
            auto times = runBench([&]() {
                auto c = img.clone(); f.apply(*c);
            }, runs);
            record("ConvFilter","Scharr",sz,times,mpix,"Mpix/s");
            printRow(allResults.back());
        }
        {
            RobertsCrossFilter f;
            auto times = runBench([&]() {
                auto c = img.clone(); f.apply(*c);
            }, runs);
            record("ConvFilter","RobertsCross",sz,times,mpix,"Mpix/s");
            printRow(allResults.back());
        }
        // Emboss in all directions
        for (auto& dir : {"NW", "NE", "SE", "SW"}) {
            EmbossFilter f(1.0f, dir, "HSV");
            auto times = runBench([&]() {
                auto c = img.clone(); f.apply(*c);
            }, runs);
            record("ConvFilter",std::string("Emboss ")+dir,sz,times,mpix,"Mpix/s");
            printRow(allResults.back());
        }
    }
}

// ============================================================
// SECTION 6: Volume construction & voxel access
// ============================================================
static void benchVolumeBasics() {
    printSection("6. VOLUME CONSTRUCTION & VOXEL ACCESS");
    printHeader();

    struct SzCase { int d; int runs; };
    std::vector<SzCase> sizes = {{64,30},{128,15},{256,5},{512,2}};

    for (auto& s : sizes) {
        std::string sz = std::to_string(s.d)+"^3";
        double mvox = (double)s.d * s.d * s.d / 1e6;
        {
            auto times = runBench([&]() { Volume v(s.d, s.d, s.d); }, s.runs);
            record("Volume","Construct",sz,times,mvox,"Mvox/s");
            printRow(allResults.back());
        }
        {
            Volume v(s.d, s.d, s.d); fillRandomVol(v);
            auto times = runBench([&]() { auto c = v.clone(); }, s.runs);
            record("Volume","Clone",sz,times,mvox,"Mvox/s");
            printRow(allResults.back());
        }
        // Sequential voxel read
        if (s.d <= 256) {
            Volume v(s.d, s.d, s.d); fillRandomVol(v);
            volatile unsigned char sink = 0;
            auto times = runBench([&]() {
                for (int z = 0; z < s.d; z++)
                    for (int y = 0; y < s.d; y++)
                        for (int x = 0; x < s.d; x++)
                            sink = v.getVoxel(x, y, z);
            }, std::max(1, s.runs / 3));
            record("Volume","getVoxel seq",sz,times,mvox,"Mvox/s");
            printRow(allResults.back());
        }
        // Sequential voxel write
        if (s.d <= 256) {
            Volume v(s.d, s.d, s.d);
            auto times = runBench([&]() {
                for (int z = 0; z < s.d; z++)
                    for (int y = 0; y < s.d; y++)
                        for (int x = 0; x < s.d; x++)
                            v.setVoxel(x, y, z, 128);
            }, std::max(1, s.runs / 3));
            record("Volume","setVoxel seq",sz,times,mvox,"Mvox/s");
            printRow(allResults.back());
        }
    }

    // Multi-channel volume benchmarks
    printSection("6b. MULTI-CHANNEL VOLUME (RGB)");
    printHeader();
    for (int d : {64, 128, 256}) {
        std::string sz = std::to_string(d)+"^3 RGB";
        double mvox = (double)d * d * d / 1e6;
        int runs = (d <= 64) ? 20 : (d <= 128 ? 10 : 3);
        {
            auto times = runBench([&]() { Volume v(d, d, d, 3); }, runs);
            record("VolumeRGB","Construct",sz,times,mvox,"Mvox/s");
            printRow(allResults.back());
        }
        {
            Volume v(d, d, d, 3); fillRandomVol(v);
            auto times = runBench([&]() { auto c = v.clone(); }, runs);
            record("VolumeRGB","Clone",sz,times,mvox,"Mvox/s");
            printRow(allResults.back());
        }
    }
}

// ============================================================
// SECTION 7: Projections (MIP, MinIP, AverageIP, MedianIP)
// ============================================================
static void benchProjections() {
    printSection("7. PROJECTIONS (Greyscale Volumes)");
    printHeader();

    struct VolCase { int d; int runs; };
    std::vector<VolCase> vols = {{64,20},{128,10},{256,5},{512,2}};

    for (auto& vc : vols) {
        Volume vol(vc.d, vc.d, vc.d);
        fillRandomVol(vol);
        std::string sz = std::to_string(vc.d)+"^3";
        double mvox = (double)vc.d * vc.d * vc.d / 1e6;

        struct ProjTest {
            std::string name;
            std::function<Image(const Volume&)> fn;
        };
        std::vector<ProjTest> projs = {
            {"MIP",      [](const Volume& v){ return Projection::maxIntensity(v); }},
            {"MinIP",    [](const Volume& v){ return Projection::minIntensity(v); }},
            {"MeanAIP",  [](const Volume& v){ return Projection::averageIntensity(v); }},
            {"MedianAIP",[](const Volume& v){ return Projection::medianIntensity(v); }},
        };

        for (auto& pt : projs) {
            auto times = runBench([&]() { auto img = pt.fn(vol); }, vc.runs);
            record("Projection",pt.name+" full",sz,times,mvox,"Mvox/s");
            printRow(allResults.back());
        }

        // Thin slab (25%, 50%, 75%)
        for (int pct : {25, 50, 75}) {
            int slabSize = vc.d * pct / 100;
            int zStart = (vc.d - slabSize) / 2;
            int zEnd = zStart + slabSize - 1;
            double slabMvox = (double)vc.d * vc.d * slabSize / 1e6;
            std::string slabSz = sz + " " + std::to_string(pct) + "%";

            auto times = runBench([&]() {
                auto img = Projection::maxIntensity(vol, zStart, zEnd);
            }, vc.runs);
            record("Projection","MIP slab"+std::to_string(pct)+"%",slabSz,times,slabMvox,"Mvox/s");
            printRow(allResults.back());
        }
    }

    // RGB volume projections
    printSection("7b. PROJECTIONS (RGB Volumes)");
    printHeader();
    for (int d : {64, 128, 256}) {
        Volume vol(d, d, d, 3);
        fillRandomVol(vol);
        std::string sz = std::to_string(d)+"^3 RGB";
        double mvox = (double)d * d * d / 1e6;
        int runs = (d <= 64) ? 15 : (d <= 128 ? 5 : 2);

        auto times_mip = runBench([&]() { auto img = Projection::maxIntensity(vol); }, runs);
        record("ProjRGB","MIP",sz,times_mip,mvox,"Mvox/s");
        printRow(allResults.back());

        auto times_min = runBench([&]() { auto img = Projection::minIntensity(vol); }, runs);
        record("ProjRGB","MinIP",sz,times_min,mvox,"Mvox/s");
        printRow(allResults.back());

        auto times_avg = runBench([&]() { auto img = Projection::averageIntensity(vol); }, runs);
        record("ProjRGB","MeanAIP",sz,times_avg,mvox,"Mvox/s");
        printRow(allResults.back());

        auto times_med = runBench([&]() { auto img = Projection::medianIntensity(vol); }, runs);
        record("ProjRGB","MedianAIP",sz,times_med,mvox,"Mvox/s");
        printRow(allResults.back());
    }
}

// ============================================================
// SECTION 8: Slicing
// ============================================================
static void benchSlicing() {
    printSection("8. VOLUME SLICING");
    printHeader();

    for (int d : {64, 128, 256, 512}) {
        Volume vol(d, d, d);
        fillRandomVol(vol);
        std::string sz = std::to_string(d)+"^3";
        double mvox = (double)d * d / 1e6;
        int runs = (d <= 128) ? 20 : (d <= 256 ? 10 : 5);
        int mid = d / 2;

        auto times_xz = runBench([&]() {
            auto img = Slice::slice(vol, mid, 'Y');
        }, runs);
        record("Slice","XZ plane",sz,times_xz,mvox,"Mpix/s");
        printRow(allResults.back());

        auto times_yz = runBench([&]() {
            auto img = Slice::slice(vol, mid, 'X');
        }, runs);
        record("Slice","YZ plane",sz,times_yz,mvox,"Mpix/s");
        printRow(allResults.back());
    }

    // RGB slicing
    for (int d : {64, 128, 256}) {
        Volume vol(d, d, d, 3);
        fillRandomVol(vol);
        std::string sz = std::to_string(d)+"^3 RGB";
        double mvox = (double)d * d / 1e6;
        int runs = (d <= 128) ? 15 : 5;
        int mid = d / 2;

        auto times_xz = runBench([&]() {
            auto img = Slice::slice(vol, mid, 'Y');
        }, runs);
        record("SliceRGB","XZ plane",sz,times_xz,mvox,"Mpix/s");
        printRow(allResults.back());

        auto times_yz = runBench([&]() {
            auto img = Slice::slice(vol, mid, 'X');
        }, runs);
        record("SliceRGB","YZ plane",sz,times_yz,mvox,"Mpix/s");
        printRow(allResults.back());
    }
}

// ============================================================
// SECTION 9: 3D Filters (Gaussian3D, Median3D)
// ============================================================
static void bench3DFilters() {
    printSection("9. 3D FILTERS");
    printHeader();

    struct VolCase { int d; };
    std::vector<VolCase> vols = {{32},{64},{128}};

    for (auto& vc : vols) {
        std::string sz = std::to_string(vc.d)+"^3";
        double mvox = (double)vc.d * vc.d * vc.d / 1e6;
        int runs = (vc.d <= 32) ? 10 : (vc.d <= 64 ? 5 : 2);

        // Gaussian 3D various kernel sizes
        for (int k : {3, 5, 7}) {
            if (vc.d <= 32 || k <= 5) {
                Volume v(vc.d, vc.d, vc.d); fillRandomVol(v);
                GaussianBlur3DFilter f(k, k / 3.0);
                auto times = runBench([&]() {
                    auto cl = v.clone(); f.apply(*cl);
                }, runs);
                record("Filter3D","Gauss3D k="+std::to_string(k),sz,times,mvox,"Mvox/s");
                printRow(allResults.back());
            }
        }
        // Median 3D various kernel sizes
        for (int k : {3, 5}) {
            Volume v(vc.d, vc.d, vc.d); fillRandomVol(v);
            MedianBlur3DFilter f(k);
            auto times = runBench([&]() {
                auto cl = v.clone(); f.apply(*cl);
            }, runs);
            record("Filter3D","Median3D k="+std::to_string(k),sz,times,mvox,"Mvox/s");
            printRow(allResults.back());
        }
    }

    // 3D filters on RGB volumes
    printSection("9b. 3D FILTERS (RGB Volumes)");
    printHeader();
    for (int d : {32, 64}) {
        std::string sz = std::to_string(d)+"^3 RGB";
        double mvox = (double)d * d * d / 1e6;
        int runs = (d <= 32) ? 5 : 2;

        Volume v(d, d, d, 3); fillRandomVol(v);
        {
            GaussianBlur3DFilter f(3, 1.0);
            auto times = runBench([&]() {
                auto cl = v.clone(); f.apply(*cl);
            }, runs);
            record("Filter3DRGB","Gauss3D k=3",sz,times,mvox,"Mvox/s");
            printRow(allResults.back());
        }
        {
            MedianBlur3DFilter f(3);
            auto times = runBench([&]() {
                auto cl = v.clone(); f.apply(*cl);
            }, runs);
            record("Filter3DRGB","Median3D k=3",sz,times,mvox,"Mvox/s");
            printRow(allResults.back());
        }
    }
}

// ============================================================
// SECTION 10: Filter chaining (realistic pipelines)
// ============================================================
static void benchPipelines() {
    printSection("10. REALISTIC PIPELINES");
    printHeader();

    struct SzCase { int w; int h; int runs; };
    std::vector<SzCase> sizes = {{512,512,10},{1024,1024,5},{2048,2048,2}};

    for (auto& s : sizes) {
        std::string sz = std::to_string(s.w)+"x"+std::to_string(s.h);
        double mpix = (double)s.w * s.h / 1e6;
        Image img(s.w, s.h, 3); fillRandomImg(img);

        // Pipeline 1: Greyscale -> Gaussian -> Sobel
        {
            auto times = runBench([&]() {
                auto c = img.clone();
                GreyscaleFilter gf; gf.apply(*c);
                GaussianBlurFilter gb(5, 1.5); gb.apply(*c);
                SobelFilter sf; sf.apply(*c);
            }, s.runs);
            record("Pipeline","Grey+Gauss+Sobel",sz,times,mpix,"Mpix/s");
            printRow(allResults.back());
        }
        // Pipeline 2: Brightness -> HistEq -> Sharpen
        {
            auto times = runBench([&]() {
                auto c = img.clone();
                BrightnessFilter bf(30); bf.apply(*c);
                EqualizeHistogram eq("HSV"); eq.apply(*c);
                SharpenFilter sf; sf.apply(*c);
            }, s.runs);
            record("Pipeline","Bright+Hist+Sharp",sz,times,mpix,"Mpix/s");
            printRow(allResults.back());
        }
        // Pipeline 3: Gaussian -> Greyscale -> Threshold
        {
            auto times = runBench([&]() {
                auto c = img.clone();
                GaussianBlurFilter gb(9, 3.0); gb.apply(*c);
                GreyscaleFilter gf; gf.apply(*c);
                ThresholdFilter tf(128, "HSV"); tf.apply(*c);
            }, s.runs);
            record("Pipeline","Gauss+Grey+Thresh",sz,times,mpix,"Mpix/s");
            printRow(allResults.back());
        }
        // Pipeline 4: BoxBlur -> Emboss -> Brightness
        {
            auto times = runBench([&]() {
                auto c = img.clone();
                BoxBlurFilter bb(5); bb.apply(*c);
                EmbossFilter ef(1.0f, "NW", "HSV"); ef.apply(*c);
                BrightnessFilter bf(20); bf.apply(*c);
            }, s.runs);
            record("Pipeline","Box+Emboss+Bright",sz,times,mpix,"Mpix/s");
            printRow(allResults.back());
        }
        // Pipeline 5: Heavy chain - 3 gaussian passes + edge
        {
            auto times = runBench([&]() {
                auto c = img.clone();
                GaussianBlurFilter g1(3, 1.0); g1.apply(*c);
                GaussianBlurFilter g2(5, 2.0); g2.apply(*c);
                GaussianBlurFilter g3(7, 2.5); g3.apply(*c);
                ScharrFilter sf; sf.apply(*c);
            }, s.runs);
            record("Pipeline","3xGauss+Scharr",sz,times,mpix,"Mpix/s");
            printRow(allResults.back());
        }
    }

    // Volume pipeline
    printSection("10b. VOLUME PIPELINES");
    printHeader();
    for (int d : {64, 128}) {
        std::string sz = std::to_string(d)+"^3";
        double mvox = (double)d * d * d / 1e6;
        int runs = (d <= 64) ? 5 : 2;

        Volume vol(d, d, d); fillRandomVol(vol);
        // Pipeline: Gaussian3D -> MIP projection
        {
            auto times = runBench([&]() {
                auto cl = vol.clone();
                GaussianBlur3DFilter f(3, 1.0); f.apply(*cl);
                auto img = Projection::maxIntensity(*cl);
            }, runs);
            record("VolPipeline","Gauss3D+MIP",sz,times,mvox,"Mvox/s");
            printRow(allResults.back());
        }
        // Pipeline: Median3D -> slice extraction
        {
            auto times = runBench([&]() {
                auto cl = vol.clone();
                MedianBlur3DFilter f(3); f.apply(*cl);
                auto img = Slice::slice(*cl, d/2, 'Y');
            }, runs);
            record("VolPipeline","Med3D+SliceXZ",sz,times,mvox,"Mvox/s");
            printRow(allResults.back());
        }
        // Pipeline: Gaussian3D -> All 4 projections
        {
            auto times = runBench([&]() {
                auto cl = vol.clone();
                GaussianBlur3DFilter f(3, 1.0); f.apply(*cl);
                auto i1 = Projection::maxIntensity(*cl);
                auto i2 = Projection::minIntensity(*cl);
                auto i3 = Projection::averageIntensity(*cl);
                auto i4 = Projection::medianIntensity(*cl);
            }, runs);
            record("VolPipeline","G3D+AllProj",sz,times,mvox,"Mvox/s");
            printRow(allResults.back());
        }
    }
}

// ============================================================
// SECTION 11: Kernel-size scaling analysis
// ============================================================
static void benchScaling() {
    printSection("11. KERNEL-SIZE SCALING ANALYSIS (1024x1024 RGB)");
    printHeader();

    Image img(1024, 1024, 3); fillRandomImg(img);
    double mpix = 1.048576;

    // Box blur scaling
    for (int k : {3, 5, 7, 9, 11, 15, 21, 31, 41, 51, 63}) {
        BoxBlurFilter f(k);
        auto times = runBench([&]() {
            auto c = img.clone(); f.apply(*c);
        }, 5);
        record("BoxScaling","k="+std::to_string(k),"1024x1024",times,mpix,"Mpix/s");
        printRow(allResults.back());
    }

    std::cout << "\n";
    printHeader();
    // Gaussian blur scaling
    for (int k : {3, 5, 7, 9, 11, 15, 21, 31, 41, 51, 63}) {
        GaussianBlurFilter f(k, k / 3.0);
        auto times = runBench([&]() {
            auto c = img.clone(); f.apply(*c);
        }, 5);
        record("GaussScaling","k="+std::to_string(k),"1024x1024",times,mpix,"Mpix/s");
        printRow(allResults.back());
    }

    std::cout << "\n";
    printHeader();
    // Median blur scaling (slower, fewer sizes)
    for (int k : {3, 5, 7, 9, 11}) {
        MedianBlurFilter f(k);
        auto times = runBench([&]() {
            auto c = img.clone(); f.apply(*c);
        }, 3);
        record("MedianScaling","k="+std::to_string(k),"1024x1024",times,mpix,"Mpix/s");
        printRow(allResults.back());
    }
}

// ============================================================
// SECTION 12: Volume-size scaling analysis
// ============================================================
static void benchVolumeScaling() {
    printSection("12. VOLUME-SIZE SCALING (MIP Projection)");
    printHeader();

    for (int d : {32, 48, 64, 96, 128, 192, 256, 384, 512}) {
        Volume vol(d, d, d);
        fillRandomVol(vol);
        double mvox = (double)d * d * d / 1e6;
        std::string sz = std::to_string(d)+"^3";
        int runs = (d <= 128) ? 10 : (d <= 256 ? 5 : 2);

        auto times = runBench([&]() {
            auto img = Projection::maxIntensity(vol);
        }, runs);
        record("VolScaling","MIP",sz,times,mvox,"Mvox/s");
        printRow(allResults.back());
    }

    std::cout << "\n";
    printHeader();
    for (int d : {32, 48, 64, 96, 128, 192, 256, 384, 512}) {
        Volume vol(d, d, d);
        fillRandomVol(vol);
        double mvox = (double)d * d * d / 1e6;
        std::string sz = std::to_string(d)+"^3";
        int runs = (d <= 128) ? 10 : (d <= 256 ? 5 : 2);

        auto times = runBench([&]() {
            auto img = Projection::medianIntensity(vol);
        }, runs);
        record("VolScaling","MedianAIP",sz,times,mvox,"Mvox/s");
        printRow(allResults.back());
    }
}

// ============================================================
// SECTION 13: Memory bandwidth test (raw data copy)
// ============================================================
static void benchMemory() {
    printSection("13. MEMORY BANDWIDTH BASELINE");
    printHeader();

    for (int d : {64, 128, 256, 512}) {
        Volume v(d, d, d);
        fillRandomVol(v);
        double mb = (double)d * d * d / (1024.0 * 1024.0);
        std::string sz = std::to_string(d)+"^3";
        int runs = (d <= 128) ? 20 : (d <= 256 ? 10 : 5);

        // Raw memcpy via getData
        auto times = runBench([&]() {
            auto& src = v.getData();
            std::vector<unsigned char> dst(src.size());
            std::copy(src.begin(), src.end(), dst.begin());
        }, runs);
        double mbps = mb / (allResults.empty() ? 1 : 1) ;
        // recalc throughput as MB/s
        double avgMs = 0;
        for (auto t : times) avgMs += t;
        avgMs /= times.size();
        double tp = mb / (avgMs / 1000.0);

        record("Memory","memcpy",sz,times,mb,"MB/s");
        // override the throughput unit
        allResults.back().throughput = tp;
        allResults.back().throughputUnit = "MB/s";
        printRow(allResults.back());
    }

    // Image memcpy baseline
    for (int w : {512, 1024, 2048, 4096}) {
        Image img(w, w, 3); fillRandomImg(img);
        double mb = (double)w * w * 3 / (1024.0 * 1024.0);
        std::string sz = std::to_string(w)+"x"+std::to_string(w)+"x3";
        int runs = (w <= 1024) ? 20 : (w <= 2048 ? 10 : 5);

        auto times = runBench([&]() {
            auto& src = img.getData();
            std::vector<unsigned char> dst(src.size());
            std::copy(src.begin(), src.end(), dst.begin());
        }, runs);

        double avgMs = 0;
        for (auto t : times) avgMs += t;
        avgMs /= times.size();
        double tp = mb / (avgMs / 1000.0);

        record("Memory","img memcpy",sz,times,mb,"MB/s");
        allResults.back().throughput = tp;
        allResults.back().throughputUnit = "MB/s";
        printRow(allResults.back());
    }
}

// ============================================================
// SECTION 14: Greyscale vs RGB performance comparison
// ============================================================
static void benchGreyVsRGB() {
    printSection("14. GREYSCALE vs RGB PERFORMANCE");
    printHeader();

    int w = 1024, h = 1024;
    double mpix = (double)w * h / 1e6;
    int runs = 5;

    Image grey(w, h, 1); fillRandomImg(grey);
    Image rgb(w, h, 3);  fillRandomImg(rgb);

    struct FilterTest {
        std::string name;
        std::function<void(Image&)> fn;
    };
    std::vector<FilterTest> filters = {
        {"BoxBlur k=5",    [](Image& i){ BoxBlurFilter f(5); f.apply(i); }},
        {"Gaussian k=9",   [](Image& i){ GaussianBlurFilter f(9, 3.0); f.apply(i); }},
        {"Median k=5",     [](Image& i){ MedianBlurFilter f(5); f.apply(i); }},
        {"Sharpen",        [](Image& i){ SharpenFilter f; f.apply(i); }},
        {"Brightness+30",  [](Image& i){ BrightnessFilter f(30); f.apply(i); }},
    };

    for (auto& ft : filters) {
        // Grey
        {
            auto times = runBench([&]() {
                auto c = grey.clone(); ft.fn(*c);
            }, runs);
            record("GreyVsRGB",ft.name+" (1ch)","1024x1024",times,mpix,"Mpix/s");
            printRow(allResults.back());
        }
        // RGB
        {
            auto times = runBench([&]() {
                auto c = rgb.clone(); ft.fn(*c);
            }, runs);
            record("GreyVsRGB",ft.name+" (3ch)","1024x1024",times,mpix,"Mpix/s");
            printRow(allResults.back());
        }
    }
}

// ============================================================
// FINAL SUMMARY
// ============================================================
static void printSummary() {
    printSection("FINAL SUMMARY - ALL RESULTS");
    std::cout << std::setw(16) << "Section"
              << std::setw(22) << "Test"
              << std::setw(14) << "Size"
              << std::setw(6)  << "Runs"
              << std::setw(12) << "Avg(ms)"
              << std::setw(14) << "Throughput"
              << "\n";
    std::cout << std::string(100, '-') << "\n";
    for (auto& r : allResults) {
        std::cout << std::setw(16) << r.section
                  << std::setw(22) << r.name
                  << std::setw(14) << r.size
                  << std::setw(6)  << r.runs
                  << std::setw(12) << std::fixed << std::setprecision(2) << r.avgMs
                  << std::setw(10) << std::setprecision(2) << r.throughput
                  << " " << r.throughputUnit
                  << "\n";
    }

    std::cout << "\n" << std::string(100, '=') << "\n";
    std::cout << "  Total benchmarks run: " << allResults.size() << "\n";
    std::cout << std::string(100, '=') << "\n";
}

// ============================================================
// MAIN
// ============================================================
int main() {
    std::cout << std::string(100, '#') << "\n";
    std::cout << "  COMPREHENSIVE PERFORMANCE BENCHMARK SUITE\n";
    std::cout << "  APImageFilters - Full Implementation Benchmark\n";
    std::cout << std::string(100, '#') << "\n";

    benchImageBasics();
    benchPixelAccess();
    benchColourConversions();
    benchSimpleFilters();
    benchConvFilters();
    benchVolumeBasics();
    benchProjections();
    benchSlicing();
    bench3DFilters();
    benchPipelines();
    benchScaling();
    benchVolumeScaling();
    benchMemory();
    benchGreyVsRGB();
    printSummary();

    return 0;
}
