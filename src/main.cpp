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
 
 * @file main.cpp
 * @brief Main entry point for the APImageFilters application.
 * @details Parses command-line arguments and dispatches execution to either 
 * the 2D image processing pipeline or the 3D volume processing pipeline. 
 * Handles filter instantiation, error catching, and file I/O operations.
 */


/**
 * APImageFilters - Advanced Programming Group Project
 *
 * Main entry point: parses command-line arguments and dispatches
 * to the appropriate image/volume processing pipeline.
 */

// stb_image is a single-header library. Defining these macros exactly once 
// tells the compiler to generate the implementation code here, preventing linker errors.
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <stdexcept>
#include <exception>
#include <algorithm>
#include <cctype>

// Utility function to check if a parsed argument string is a valid positive integer.
// Prevents std::stoi from throwing unhandled exceptions if the user inputs junk data.
static bool isPositiveIntegerArg(const std::string& s) {
    if (s.empty()) return false;
    for (char ch : s) {
        if (ch < '0' || ch > '9') return false; // Fail immediately on non-digit characters
    }
    return true;
}

static bool isIntegerArg(const std::string& s) {
    if (s.empty()) return false;
    size_t start = 0;
    if (s[0] == '+' || s[0] == '-') {
        if (s.size() == 1) return false;
        start = 1;
    }
    for (size_t i = start; i < s.size(); ++i) {
        if (s[i] < '0' || s[i] > '9') return false;
    }
    return true;
}

#include "Image.h"
#include "Filter.h"
#include "SimpleFilter.h"
#include "ConvolutionalFilter.h"
#include "Filters3D.h"
#include "Volume.h"
#include "Projection.h"
#include "Slice.h"
#include "SliceVolume.h"

static void printUsage(const char* progName) {
    std::cerr << "Usage:\n"
              << "  Image:  " << progName << " -i <input_image> [filters...] <output_image>\n"
              << "  Volume: " << progName << " -d <data_volume> [options...] <output_image>\n";
}

// Simple helper to validate color space arguments
static bool isTypeArg(const std::string& s) {
    return s == "HSV" || s == "HSL";
}

static bool hasValidOutputExtension(const std::string& filename) {
    auto dot = filename.rfind('.');
    if (dot == std::string::npos || dot == filename.size() - 1) {
        return false;
    }

    std::string ext = filename.substr(dot);
    std::transform(ext.begin(), ext.end(), ext.begin(),
        [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });

    return ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp";
}

struct VolumeOptions {
    int firstIdx = -1;
    int lastIdx = -1;
    std::string extension = "png";

    bool hasBlur = false;
    std::string blurType;
    int blurSize = 0;
    double blurStdev = 2.0;

    bool hasSlice = false;
    std::string slicePlane;
    int sliceConstant = 0;

    bool hasProjection = false;
    std::string projectionType;
    int slabMin = -1;
    int slabMax = -1;
};

static std::string requireNextArg(int& i, int argc, char* argv[], const std::string& label) {
    if (i + 1 >= argc - 1) {
        throw std::invalid_argument("Missing argument for " + label);
    }
    ++i;
    return argv[i];
}

static int parseIntArg(const std::string& value, const std::string& label) {
    size_t pos = 0;
    int parsed = 0;
    try {
        parsed = std::stoi(value, &pos);
    } catch (const std::exception&) {
        throw std::invalid_argument("Invalid integer for " + label + ": '" + value + "'");
    }
    if (pos != value.size()) {
        throw std::invalid_argument("Invalid integer for " + label + ": '" + value + "'");
    }
    return parsed;
}

static double parseDoubleArg(const std::string& value, const std::string& label) {
    size_t pos = 0;
    double parsed = 0.0;
    try {
        parsed = std::stod(value, &pos);
    } catch (const std::exception&) {
        throw std::invalid_argument("Invalid number for " + label + ": '" + value + "'");
    }
    if (pos != value.size()) {
        throw std::invalid_argument("Invalid number for " + label + ": '" + value + "'");
    }
    return parsed;
}

static int requireNextIntArg(int& i, int argc, char* argv[], const std::string& label) {
    return parseIntArg(requireNextArg(i, argc, argv, label), label);
}

static bool isImageOptionFlag(const std::string& s) {
    return s == "-g" || s == "--greyscale" ||
           s == "-b" || s == "--brightness" ||
           s == "-h" || s == "--histogram" ||
           s == "-r" || s == "--blur" ||
           s == "-e" || s == "--edge" ||
           s == "-L" || s == "--sharpen" ||
           s == "-m" || s == "--emboss" ||
           s == "-n" || s == "--saltpepper" ||
           s == "-t" || s == "--threshold";
}

static std::vector<std::unique_ptr<Filter>> parseImageFilters(int argc, char* argv[]) {
    std::vector<std::unique_ptr<Filter>> filters;
    int i = 3;

    while (i < argc - 1) {
        std::string flag = argv[i];

        if (flag == "-g" || flag == "--greyscale") {
            filters.push_back(std::make_unique<GreyscaleFilter>());
        }
        else if (flag == "-b" || flag == "--brightness") {
            if (i + 1 >= argc - 1) {
                filters.push_back(std::make_unique<BrightnessFilter>());
            } else {
                std::string nextArg = argv[i + 1];
                if (nextArg == "auto") {
                    ++i;
                    filters.push_back(std::make_unique<BrightnessFilter>());
                } else if (isIntegerArg(nextArg)) {
                    ++i;
                    filters.push_back(std::make_unique<BrightnessFilter>(parseIntArg(nextArg, flag + " value")));
                } else if (isImageOptionFlag(nextArg)) {
                    filters.push_back(std::make_unique<BrightnessFilter>());
                } else {
                    throw std::invalid_argument("Invalid brightness value '" + nextArg + "'");
                }
            }
        }
        else if (flag == "-h" || flag == "--histogram") {
            std::string type = "HSV";
            if (i + 1 < argc - 1 && isTypeArg(argv[i + 1])) {
                type = argv[++i];
            }
            filters.push_back(std::make_unique<EqualizeHistogram>(type));
        }
        else if (flag == "-r" || flag == "--blur") {
            std::string blurType = requireNextArg(i, argc, argv, flag);
            int size = requireNextIntArg(i, argc, argv, flag + " size");

            if (blurType == "Gaussian") {
                double stdev = 2.0;
                if (i + 1 < argc - 1 && !isImageOptionFlag(argv[i + 1])) {
                    stdev = parseDoubleArg(argv[i + 1], flag + " stdev");
                    ++i;
                }
                filters.push_back(std::make_unique<GaussianBlurFilter>(size, stdev));
            } else if (blurType == "Box") {
                filters.push_back(std::make_unique<BoxBlurFilter>(size));
            } else if (blurType == "Median") {
                filters.push_back(std::make_unique<MedianBlurFilter>(size));
            } else {
                throw std::invalid_argument("unknown blur type '" + blurType + "'");
            }
        }
        else if (flag == "-e" || flag == "--edge") {
            std::string edgeType = requireNextArg(i, argc, argv, flag);
            if (edgeType == "Sobel") {
                filters.push_back(std::make_unique<SobelFilter>());
            } else if (edgeType == "Prewitt") {
                filters.push_back(std::make_unique<PrewittFilter>());
            } else if (edgeType == "Scharr") {
                filters.push_back(std::make_unique<ScharrFilter>());
            } else if (edgeType == "RobertsCross") {
                filters.push_back(std::make_unique<RobertsCrossFilter>());
            } else {
                throw std::invalid_argument("unknown edge detection type '" + edgeType + "'");
            }
        }
        else if (flag == "-L" || flag == "--sharpen") {
            filters.push_back(std::make_unique<SharpenFilter>());
        }
        else if (flag == "-m" || flag == "--emboss") {
            float strength = static_cast<float>(parseDoubleArg(requireNextArg(i, argc, argv, flag), flag + " strength"));
            std::string direction = requireNextArg(i, argc, argv, flag + " direction");
            std::string type = "HSV";
            if (i + 1 < argc - 1 && isTypeArg(argv[i + 1])) {
                type = argv[++i];
            }
            filters.push_back(std::make_unique<EmbossFilter>(strength, direction, type));
        }
        else if (flag == "-n" || flag == "--saltpepper") {
            int amount = requireNextIntArg(i, argc, argv, flag);
            filters.push_back(std::make_unique<SaltPepperFilter>(amount));
        }
        else if (flag == "-t" || flag == "--threshold") {
            int value = requireNextIntArg(i, argc, argv, flag);
            std::string type = "HSV";
            if (i + 1 < argc - 1 && isTypeArg(argv[i + 1])) {
                type = argv[++i];
            }
            filters.push_back(std::make_unique<ThresholdFilter>(value, type));
        }
        else {
            throw std::invalid_argument("unknown option '" + flag + "'");
        }

        ++i;
    }

    return filters;
}

static VolumeOptions parseVolumeOptions(int argc, char* argv[]) {
    VolumeOptions options;
    int i = 3;

    while (i < argc - 1) {
        std::string flag = argv[i];

        if (flag == "-f" || flag == "--first") {
            options.firstIdx = requireNextIntArg(i, argc, argv, flag);
        }
        else if (flag == "-l" || flag == "--last") {
            options.lastIdx = requireNextIntArg(i, argc, argv, flag);
        }
        else if (flag == "-x" || flag == "--extension") {
            options.extension = requireNextArg(i, argc, argv, flag);
        }
        else if (flag == "-r" || flag == "--blur") {
            options.hasBlur = true;
            options.blurType = requireNextArg(i, argc, argv, flag);
            options.blurSize = requireNextIntArg(i, argc, argv, flag + " size");

            if (options.blurType == "Gaussian" && i + 1 < argc - 1) {
                std::string nextArg = argv[i + 1];
                if (!nextArg.empty() && nextArg[0] != '-') {
                    options.blurStdev = parseDoubleArg(nextArg, flag + " stdev");
                    ++i;
                }
            }
        }
        else if (flag == "-s" || flag == "--slice") {
            options.hasSlice = true;
            options.slicePlane = requireNextArg(i, argc, argv, flag);
            options.sliceConstant = requireNextIntArg(i, argc, argv, flag + " constant");
        }
        else if (flag == "-p" || flag == "--projection") {
            options.hasProjection = true;
            options.projectionType = requireNextArg(i, argc, argv, flag);

            if (i + 2 < argc - 1) {
                const std::string startArg = argv[i + 1];
                const std::string endArg = argv[i + 2];
                if (isPositiveIntegerArg(startArg) && isPositiveIntegerArg(endArg)) {
                    options.slabMin = parseIntArg(startArg, flag + " start");
                    options.slabMax = parseIntArg(endArg, flag + " end");
                    i += 2;
                }
            }
        }
        else if (flag == "--slab") {
            options.slabMin = requireNextIntArg(i, argc, argv, flag + " min");
            options.slabMax = requireNextIntArg(i, argc, argv, flag + " max");
        }
        else {
            throw std::invalid_argument("Unknown option '" + flag + "'");
        }

        ++i;
    }

    if (!options.hasSlice && !options.hasProjection) {
        throw std::invalid_argument("volume mode requires --slice or --projection");
    }
    if (options.hasBlur && options.blurType != "Gaussian" && options.blurType != "Median") {
        throw std::invalid_argument("unknown 3D blur type '" + options.blurType + "'");
    }
    if (options.hasSlice && options.slicePlane != "XZ" && options.slicePlane != "YZ") {
        throw std::invalid_argument("unknown slice plane '" + options.slicePlane + "'");
    }
    if (options.hasProjection && options.projectionType != "MIP" &&
        options.projectionType != "MinIP" &&
        options.projectionType != "meanAIP" &&
        options.projectionType != "medianAIP") {
        throw std::invalid_argument("unknown projection type '" + options.projectionType + "'");
    }

    return options;
}

int main(int argc, char* argv[]) {
    // We need at least 4 arguments: executable_name, mode_flag (-i or -d), input_path, and output_path
    if (argc < 4) {
        printUsage(argv[0]);
        return 1;
    }

    // The output file is always strictly the last argument in our CLI design
    std::string outputFile = argv[argc - 1];
    std::string modeFlag = argv[1];

    if (!hasValidOutputExtension(outputFile)) {
        std::cerr << "Error: output file must end with .png, .jpg, .jpeg, or .bmp\n";
        return 1;
    }

    // ==================== IMAGE MODE ====================
    if (modeFlag == "-i") {
        std::string inputFile = argv[2]; // Input file is right after the mode flag

        // Load the image using a smart pointer. This ensures automatic memory cleanup 
        // if the program exits early due to an error.
        auto imgPtr = Image::load(inputFile);
        if (!imgPtr) {
            std::cerr << "Error: failed to load image '" << inputFile << "'\n";
            return 1;
        }
        Image& img = *imgPtr; // Dereference for easier syntax below

        std::vector<std::unique_ptr<Filter>> filters;
        try {
            filters = parseImageFilters(argc, argv);
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << "\n";
            printUsage(argv[0]);
            return 1;
        }

        // Execute the pipeline sequentially.
        // Catching specific exceptions prevents a single bad filter (e.g. invalid kernel size) 
        // from crashing the entire program. It just skips the bad filter and keeps processing.
        for (auto& f : filters) {
            try {
                f->apply(img);
            } catch (const std::runtime_error& e) {
                std::cerr << "Warning: skipping filter (" << e.what() << ")\n";
            } catch (const std::invalid_argument& e) {
                std::cerr << "Error: filter threw exception (" << e.what() << ")\n";
            } catch (const std::exception& e) {
                std::cerr << "Error: filter threw unexpected exception (" << e.what() << ")\n";
            } catch (...) {
                std::cerr << "Error: filter threw unknown exception\n";
            }
        }

        // Save using stb_image_write (format auto-detected by extension inside save())
        if (!img.save(outputFile)) {
            std::cerr << "Error: failed to save image '" << outputFile << "'\n";
            return 1;
        }
    }




    //VOLUME MODE
    else if (modeFlag == "-d") {
        std::string volumePrefix = argv[2]; // Folder/prefix for the image sequence

        VolumeOptions volumeOptions;
        try {
            volumeOptions = parseVolumeOptions(argc, argv);
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << "\n";
            printUsage(argv[0]);
            return 1;
        }

        // Load the 3D volume sequence into memory
        Volume volume;
        if (!volume.load(volumePrefix, volumeOptions.firstIdx, volumeOptions.lastIdx, volumeOptions.extension)) {
            std::cerr << "Error: failed to load volume '" << volumePrefix << "'\n";
            return 1;
        }

        Image output;

        // OPTIMIZATION PATH: SLICE + BLUR 

        if (volumeOptions.hasSlice && volumeOptions.hasBlur) {
            // To avoid blurring the entire massive volume when we only need one slice, 
            // we figure out which axis we are slicing on, and instruct SliceVolume to crop the data.
            char plane;
            if (volumeOptions.slicePlane == "XZ") {
                plane = 'Y';
            } else if (volumeOptions.slicePlane == "YZ") {
                plane = 'X';
            } else {
                std::cerr << "Error: unknown slice plane '" << volumeOptions.slicePlane << "'\n";
                return 1;
            }

            // SliceVolume uses inheritance to wrap the Volume data, then crops everything 
            // EXCEPT the small 'slab' needed to calculate the blur at the target coordinate.
            // Use this casting so that we don't have to define a volume multiple times
            SliceVolume sv;
            static_cast<Volume&>(sv) = volume; 
            sv.rotate(volumeOptions.sliceConstant, volumeOptions.blurSize, plane); // Modifies dimensions internally

            // Apply the 3D blur strictly to the cropped data. Huge time/memory saving.
            try {
                if (volumeOptions.blurType == "Gaussian") {
                    GaussianBlur3DFilter f(volumeOptions.blurSize, volumeOptions.blurStdev);
                    f.apply(sv);
                } else if (volumeOptions.blurType == "Median") {
                    MedianBlur3DFilter f(volumeOptions.blurSize);
                    f.apply(sv);
                } else {
                    std::cerr << "Error: unknown 3D blur type '" << volumeOptions.blurType << "'\n";
                    return 1;
                }
            } catch (const std::exception& e) {
                std::cerr << "Warning: skipping 3D blur (" << e.what() << ")\n";
            }

            // Finally, pull out the exact target slice from the center of our blurred slab
            output = sv.slice(volumeOptions.sliceConstant);

        } else if (volumeOptions.hasSlice) {
            // Un-blurred slice path: Just take the 2D plane directly out of the 3D buffer.
            if (volumeOptions.slicePlane == "XZ") {
                output = Slice::slice(volume, volumeOptions.sliceConstant, 'Y');
            } else if (volumeOptions.slicePlane == "YZ") {
                output = Slice::slice(volume, volumeOptions.sliceConstant, 'X');
            } else {
                std::cerr << "Error: unknown slice plane '" << volumeOptions.slicePlane << "'\n";
                return 1;
            }

        } 


        // OPTIMIZATION PATH: PROJECTION + BLUR 

        else if (volumeOptions.hasProjection) {
            // Convert user's 1-based indices to 0-based indices for array access.
            // A value of -1 means "use the full volume".
            int zStart = (volumeOptions.slabMin > 0) ? volumeOptions.slabMin - 1 : -1;
            int zEnd   = (volumeOptions.slabMax > 0) ? volumeOptions.slabMax - 1 : -1;

            const int depth = volume.getDepth();
            if (zStart < 0) zStart = 0;           // Default to slice 0
            if (zEnd < 0) zEnd = depth - 1;       // Default to last slice
            
            // Safety check: ensure coordinates are within the actual loaded volume depth
            zStart = std::clamp(zStart, 0, depth - 1);
            zEnd = std::clamp(zEnd, 0, depth - 1);
            
            // Fix backwards input (e.g., user inputs start: 50, end: 10)
            if (zStart > zEnd) std::swap(zStart, zEnd);

            if (volumeOptions.hasBlur) {
                // To blur a specific chunk (slab) correctly, we must also include the pixels 
                // surrounding it (the padding radius) so edge pixels don't get corrupted during convolution.
                const int radius = volumeOptions.blurSize / 2;
                const int blurStart = std::max(0, zStart - radius);     // Don't read past front
                const int blurEnd = std::min(depth - 1, zEnd + radius); // Don't read past back
                const int subDepth = blurEnd - blurStart + 1;

                // Create a temporary volume to hold just our target slab + padding
                Volume subVolume(volume.getWidth(), volume.getHeight(), subDepth, volume.getChannels());

                // Copy the required data from the main volume into the subVolume
                for (int z = blurStart; z <= blurEnd; ++z) {
                    int localZ = z - blurStart; // Shift to 0-based index for the subVolume
                    for (int y = 0; y < volume.getHeight(); ++y) {
                        for (int x = 0; x < volume.getWidth(); ++x) {
                            for (int c = 0; c < volume.getChannels(); ++c) {
                                subVolume.setVoxel(x, y, localZ, c, volume.getVoxel(x, y, z, c));
                            }
                        }
                    }
                }

                // Apply blur only to the subVolume
                try {
                    if (volumeOptions.blurType == "Gaussian") {
                        GaussianBlur3DFilter f(volumeOptions.blurSize, volumeOptions.blurStdev);
                        f.apply(subVolume);
                    } else if (volumeOptions.blurType == "Median") {
                        MedianBlur3DFilter f(volumeOptions.blurSize);
                        f.apply(subVolume);
                    } else {
                        std::cerr << "Error: unknown 3D blur type '" << volumeOptions.blurType << "'\n";
                        return 1;
                    }
                } catch (const std::exception& e) {
                    std::cerr << "Warning: skipping 3D blur (" << e.what() << ")\n";
                }

                // Write the blurred data BACK into the original volume. 
                // Notice we only copy [zStart, zEnd], ignoring the padding radius we pulled in earlier.
                for (int z = zStart; z <= zEnd; ++z) {
                    int localZ = z - blurStart;
                    for (int y = 0; y < volume.getHeight(); ++y) {
                        for (int x = 0; x < volume.getWidth(); ++x) {
                            for (int c = 0; c < volume.getChannels(); ++c) {
                                volume.setVoxel(x, y, z, c, subVolume.getVoxel(x, y, localZ, c));
                            }
                        }
                    }
                }
            }

            // Finally, execute the projection. The data is either already blurred above, or left untouched.
            if (volumeOptions.projectionType == "MIP") {
                output = Projection::maxIntensity(volume, zStart, zEnd);
            } else if (volumeOptions.projectionType == "MinIP") {
                output = Projection::minIntensity(volume, zStart, zEnd);
            } else if (volumeOptions.projectionType == "meanAIP") {
                output = Projection::averageIntensity(volume, zStart, zEnd);
            } else if (volumeOptions.projectionType == "medianAIP") {
                output = Projection::medianIntensity(volume, zStart, zEnd);
            } else {
                std::cerr << "Error: unknown projection type '" << volumeOptions.projectionType << "'\n";
                return 1;
            }
        }

        // Save the resulting 2D output image
        if (!output.save(outputFile)) {
            std::cerr << "Error: failed to save output '" << outputFile << "'\n";
            return 1;
        }
    }
    else {
        std::cerr << "Error: first argument must be -i or -d\n";
        printUsage(argv[0]);
        return 1;
    }

    return 0; 
}