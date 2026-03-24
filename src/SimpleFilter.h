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
 
 * @file SimpleFilter.h
 * @brief Defines a collection of simple point-process image filters.
 * @details This file contains the abstract SimpleFilter base class and several 
 * concrete implementations for basic image adjustments, such as grayscale conversion, 
 * brightness adjustment, histogram equalization, thresholding, and noise injection.
 */

#ifndef SIMPLEFILTER_H
#define SIMPLEFILTER_H

#include "Filter.h"
#include <string>

/**
 * @class SimpleFilter
 * @brief Abstract base class for simple, point-process image filters.
 * @details Inherits from the base Filter class. Subclasses must implement the 
 * apply() method for pixel-by-pixel operations.
 */
class SimpleFilter : public Filter {
public:
    /**
     * @brief Default virtual destructor.
     */
    virtual ~SimpleFilter() {}

    /**
     * @brief Applies the simple filter to the given image.
     * @param image Reference to the Image object to be processed in-place.
     */
    virtual void apply(Image& image) override = 0;
};

/**
 * @class GreyscaleFilter
 * @brief Converts a color image to grayscale.
 */
class GreyscaleFilter : public SimpleFilter {
public:
    /**
     * @brief Applies the grayscale conversion to the image.
     * @param image Reference to the Image object to be processed.
     */
    void apply(Image& image) override;
};

/**
 * @class BrightnessFilter
 * @brief Adjusts the brightness of an image.
 * @details Supports both a manual adjustment mode (by a specific value) 
 * and an automatic adjustment mode.
 */
class BrightnessFilter : public SimpleFilter {
private:
    int value;      ///< The amount to add to or subtract from pixel values.
    bool autoMode;  ///< Flag indicating if the filter is in automatic adjustment mode.

public:
    /**
     * @brief Constructs a BrightnessFilter with a specific manual adjustment value.
     * @param value The brightness offset (positive to brighten, negative to darken).
     */
    BrightnessFilter(int value);

    /**
     * @brief Constructs a BrightnessFilter in automatic mode.
     */
    BrightnessFilter();  // auto mode

    /**
     * @brief Applies the brightness adjustment to the image.
     * @param image Reference to the Image object to be processed.
     */
    void apply(Image& image) override;
};

/**
 * @class EqualizeHistogram
 * @brief Performs histogram equalization to improve image contrast.
 */
class EqualizeHistogram : public SimpleFilter {
private:
    std::string type; ///< The color space to use for equalization (e.g., "HSV").

    /**
     * @brief Internal helper to equalize a grayscale image.
     * @param img Reference to the grayscale image to process.
     */
    void equaliseGreyscale(Image& img);

    /**
     * @brief Internal helper to equalize a color RGB image.
     * @param img Reference to the RGB image to process.
     */
    void equaliseRGB(Image& img);

public:
    /**
     * @brief Constructs an EqualizeHistogram filter.
     * @param type The color model to use for processing (defaults to "HSV").
     */
    EqualizeHistogram(const std::string& type = "HSV");

    /**
     * @brief Applies the histogram equalization to the image automatically determining type.
     * @param image Reference to the Image object to be processed.
     */
    void apply(Image& image) override;

    /**
     * @brief Explicitly applies histogram equalization assuming an RGB format.
     * @param image Reference to the Image object to be processed.
     */
    void applyRGB(Image& image);

    /**
     * @brief Explicitly applies histogram equalization assuming a grayscale format.
     * @param image Reference to the Image object to be processed.
     */
    void applyGrayscale(Image& image);
};

/**
 * @class ThresholdFilter
 * @brief Applies a threshold to an image, converting it to binary (black and white).
 */
class ThresholdFilter : public SimpleFilter {
private:
    int threshold;      ///< The threshold cutoff value (0-255).
    std::string type;   // "HSV" or "HSL" ///< The color space to evaluate brightness in.

    // helper functions for different image types

    /**
     * @brief Internal helper to apply thresholding to an RGB image.
     * @param image Reference to the RGB image to process.
     */
    void applyRGB(Image& image);

    /**
     * @brief Internal helper to apply thresholding to a grayscale image.
     * @param image Reference to the grayscale image to process.
     */
    void applyGrayscale(Image& image);

public:
    /**
     * @brief Constructs a ThresholdFilter.
     * @param threshold The cutoff value. Pixels below this become black, others white.
     * @param type The color model to use for brightness evaluation (defaults to "HSV").
     */
    ThresholdFilter(int threshold, const std::string& type = "HSV");

    /**
     * @brief Applies the binary thresholding to the image.
     * @param image Reference to the Image object to be processed.
     */
    void apply(Image& image) override;
};

/**
 * @class SaltPepperFilter
 * @brief Injects random salt-and-pepper noise into an image.
 */
class SaltPepperFilter : public SimpleFilter {
private:
    int amount; ///< The intensity or density parameter for the noise injection.

public:
    /**
     * @brief Constructs a SaltPepperFilter.
     * @param amount Determines how many pixels are affected by the noise.
     */
    SaltPepperFilter(int amount);

    /**
     * @brief Applies the salt-and-pepper noise to the image.
     * @param image Reference to the Image object to be processed.
     */
    void apply(Image& image) override;
};

#endif
