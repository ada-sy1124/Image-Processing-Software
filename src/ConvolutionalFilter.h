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

 * @file ConvolutionalFilter.h
 * @brief Defines a collection of convolutional image filters.
 * @details This file contains the abstract ConvolutionalFilter base class and several 
 * concrete implementations for spatial domain image filtering operations, such as 
 * blurring, sharpening, edge detection, and embossing using convolution kernels.
 */

#ifndef CONVOLUTIONALFILTER_H
#define CONVOLUTIONALFILTER_H

#include "Filter.h"
#include <string>

/**
 * @brief Base class for convolutional filters.
 * Inherits from Filter and provides an abstract interface for all image processing 
 * operations based on convolution matrices (kernels).
 */
class ConvolutionalFilter : public Filter {
public:
    /**
     * @brief Default virtual destructor.
     */
    virtual ~ConvolutionalFilter() {}

    /**
     * @brief Applies the convolutional filter to the target image.
     * @param image Reference to the image object to be processed.
     */
    virtual void apply(Image& image) override = 0;
};

/**
 * @brief Box blur (mean blur) filter.
 * Smooths the image by replacing each pixel with the average value of its local neighborhood.
 */
class BoxBlurFilter : public ConvolutionalFilter {
private:
    int size; ///< The size of the convolution kernel.

public:
    /**
     * @brief Constructs a BoxBlurFilter object.
     * @param size The size of the kernel (usually an odd number, e.g., 3 for 3x3).
     */
    BoxBlurFilter(int size);

    /**
     * @brief Applies the box blur operation to the image.
     * @param image Reference to the image object to be processed.
     */
    void apply(Image& image) override;
};

/**
 * @brief Gaussian blur filter.
 * Uses a Gaussian function to calculate weights for smoothing, which preserves 
 * edge details better than a simple box blur.
 */
class GaussianBlurFilter : public ConvolutionalFilter {
private:
    int size;     ///< The size of the convolution kernel.
    double stdev; ///< The standard deviation (sigma) of the Gaussian distribution.

public:
    /**
     * @brief Constructs a GaussianBlurFilter object.
     * @param size The size of the convolution kernel.
     * @param stdev The standard deviation, which controls the extent of the blur.
     */
    GaussianBlurFilter(int size, double stdev);

    /**
     * @brief Applies the Gaussian blur operation to the image.
     * @param image Reference to the image object to be processed.
     */
    void apply(Image& image) override;
};

/**
 * @brief Median blur filter.
 * Replaces each pixel with the median value of its neighboring pixels. 
 * Highly effective for removing salt-and-pepper noise.
 */
class MedianBlurFilter : public ConvolutionalFilter {
private:
    int size; ///< The size of the filtering window.

public:
    /**
     * @brief Constructs a MedianBlurFilter object.
     * @param size The size of the window (usually an odd number).
     */
    MedianBlurFilter(int size);

    /**
     * @brief Applies the median blur operation to the image.
     * @param image Reference to the image object to be processed.
     */
    void apply(Image& image) override;
};

/**
 * @brief Sharpen filter.
 * Enhances the edges and detail contrast of the image.
 */
class SharpenFilter : public ConvolutionalFilter {
public:
    /**
     * @brief Applies the sharpen operation to the image.
     * @param image Reference to the image object to be processed.
     */
    void apply(Image& image) override;
};

/**
 * @brief Sobel edge detection filter.
 * Highlights edges by calculating the spatial gradient of the image intensity 
 * using the Sobel operator.
 */
class SobelFilter : public ConvolutionalFilter {
public:
    /**
     * @brief Applies Sobel edge detection to the image.
     * @param image Reference to the image object to be processed.
     */
    void apply(Image& image) override;
};

/**
 * @brief Prewitt edge detection filter.
 * Uses the Prewitt operator for edge detection. Suitable for images with 
 * noticeable edges and some noise.
 */
class PrewittFilter : public ConvolutionalFilter {
public:
    /**
     * @brief Applies Prewitt edge detection to the image.
     * @param image Reference to the image object to be processed.
     */
    void apply(Image& image) override;
};

/**
 * @brief Scharr edge detection filter.
 * Provides a more accurate, rotation-invariant gradient calculation compared 
 * to the Sobel operator.
 */
class ScharrFilter : public ConvolutionalFilter {
public:
    /**
     * @brief Applies Scharr edge detection to the image.
     * @param image Reference to the image object to be processed.
     */
    void apply(Image& image) override;
};

/**
 * @brief Roberts Cross edge detection filter.
 * Approximates gradient magnitude using diagonal directional differences. 
 * Computationally fast.
 */
class RobertsCrossFilter : public ConvolutionalFilter {
public:
    /**
     * @brief Applies Roberts Cross edge detection to the image.
     * @param image Reference to the image object to be processed.
     */
    void apply(Image& image) override;
};

/**
 * @brief Emboss effect filter.
 * Creates a 3D embossed visual effect by replacing pixels with shadows cast 
 * from a specific direction.
 */
class EmbossFilter : public ConvolutionalFilter {
private:
    float strength;         ///< The intensity/strength of the emboss effect.
    std::string direction;  ///< The direction of the light source: "NW", "NE", "SE", "SW".
    std::string type;       ///< The color space type to use: "HSV" or "HSL".

public:
    /**
     * @brief Constructs an EmbossFilter object.
     * @param strength The strength of the effect.
     * @param direction The light source direction (e.g., "NW" for North-West).
     * @param type The color space to use. Defaults to "HSV".
     */
    EmbossFilter(float strength, const std::string& direction,
                 const std::string& type = "HSV");

    /**
     * @brief Applies the emboss effect to the image.
     * @param image Reference to the image object to be processed.
     */
    void apply(Image& image) override;
};

#endif
