#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "Volume.h"
#include "Projection.h"
#include <chrono>
#include <iostream>
#include <iomanip>
#include <functional>
#include <string>

static void fillRandom(Volume& vol) {
    auto& data = vol.getData();
    unsigned int seed = 12345;
    for (size_t i = 0; i < data.size(); i++) {
        seed = seed * 1103515245u + 12345u;
        data[i] = static_cast<unsigned char>((seed >> 16) & 0xFF);
    }
}

static double bench(std::function<Image()> fn, int runs) {
    double total = 0;
    for (int r = 0; r < runs; r++) {
        auto t0 = std::chrono::high_resolution_clock::now();
        Image out = fn();
        auto t1 = std::chrono::high_resolution_clock::now();
        total += std::chrono::duration<double, std::milli>(t1 - t0).count();
    }
    return total / runs;
}

int main() {
    std::cout << std::string(78, '=') << "\n";
    std::cout << "  Projection Benchmark\n";
    std::cout << std::string(78, '=') << "\n\n";

    std::cout << std::setw(12) << "Volume"
              << std::setw(14) << "Projection"
              << std::setw(8)  << "Slab"
              << std::setw(6)  << "Runs"
              << std::setw(14) << "Avg (ms)"
              << std::setw(16) << "Mvox/s"
              << "\n";
    std::cout << std::string(78, '-') << "\n";

    struct VolumeCase { int dim; int runs; };
    std::vector<VolumeCase> volumeCases = {
        {128, 10},
        {256,  5},
        {512,  2},
    };

    struct ProjCase {
        std::string name;
        std::function<Image(const Volume&)> fullFn;
        std::function<Image(const Volume&, int, int)> slabFn;
    };

    for (auto& vc : volumeCases) {
        int d = vc.dim;
        Volume vol(d, d, d);
        fillRandom(vol);
        double mvox = static_cast<double>(d) * d * d / 1e6;

        std::vector<ProjCase> projCases = {
            {"MIP",
             [](const Volume& v) { return Projection::maxIntensity(v); },
             [](const Volume& v, int zs, int ze) { return Projection::maxIntensity(v, zs, ze); }},
            {"MinIP",
             [](const Volume& v) { return Projection::minIntensity(v); },
             [](const Volume& v, int zs, int ze) { return Projection::minIntensity(v, zs, ze); }},
            {"meanAIP",
             [](const Volume& v) { return Projection::averageIntensity(v); },
             [](const Volume& v, int zs, int ze) { return Projection::averageIntensity(v, zs, ze); }},
            {"medianAIP",
             [](const Volume& v) { return Projection::medianIntensity(v); },
             [](const Volume& v, int zs, int ze) { return Projection::medianIntensity(v, zs, ze); }},
        };

        std::string dimStr = std::to_string(d) + "^3";

        for (auto& pc : projCases) {
            double avgMs = bench([&]() { return pc.fullFn(vol); }, vc.runs);
            double mvoxPerSec = mvox / (avgMs / 1000.0);
            std::cout << std::setw(12) << dimStr
                      << std::setw(14) << pc.name
                      << std::setw(8)  << "full"
                      << std::setw(6)  << vc.runs
                      << std::setw(14) << std::fixed << std::setprecision(2) << avgMs
                      << std::setw(16) << std::setprecision(2) << mvoxPerSec
                      << "\n";

            int zStart = d / 4;
            int zEnd = d * 3 / 4;
            int slabDepth = zEnd - zStart + 1;
            double slabMvox = static_cast<double>(d) * d * slabDepth / 1e6;

            double slabMs = bench([&]() { return pc.slabFn(vol, zStart, zEnd); }, vc.runs);
            double slabMvoxPerSec = slabMvox / (slabMs / 1000.0);
            std::cout << std::setw(12) << dimStr
                      << std::setw(14) << pc.name
                      << std::setw(8)  << "50%"
                      << std::setw(6)  << vc.runs
                      << std::setw(14) << std::fixed << std::setprecision(2) << slabMs
                      << std::setw(16) << std::setprecision(2) << slabMvoxPerSec
                      << "\n";
        }
        std::cout << std::string(78, '-') << "\n";
    }

    return 0;
}
