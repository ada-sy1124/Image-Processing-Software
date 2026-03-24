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
 
 * @file Image.h
 * @brief Defines the Image class and colour-space pixel structures.
 * @details This file provides a single concrete Image class that handles 
 * runtime channels (grayscale or RGB), file I/O, pixel access, and 
 * colour-space conversions (RGB, HSL, HSV).
 */

#ifndef IMAGE_H
#define IMAGE_H

#include <string>
#include <vector>
#include <memory>


// Colour-space pixel structs

/**
 * @struct RGBPixel
 * @brief Represents a single pixel in the RGB colour space.
 */
struct RGBPixel {
    unsigned char r; ///< Red channel value (0-255).
    unsigned char g; ///< Green channel value (0-255).
    unsigned char b; ///< Blue channel value (0-255).
};

/**
 * @struct HSLPixel
 * @brief Represents a single pixel in the HSL (Hue, Saturation, Lightness) colour space.
 */
struct HSLPixel {
    float h; ///< Hue angle in degrees [0, 360).
    float s; ///< Saturation [0.0, 1.0].
    float l; ///< Lightness [0.0, 1.0].
};

/**
 * @struct HSVPixel
 * @brief Represents a single pixel in the HSV (Hue, Saturation, Value) colour space.
 */
struct HSVPixel {
    float h; ///< Hue angle in degrees [0, 360).
    float s; ///< Saturation [0.0, 1.0].
    float v; ///< Value/Brightness [0.0, 1.0].
};


// Single concrete Image class.
//
// `channels` is a runtime attribute (1 = grayscale, 3 = RGB).
// No more abstract base / subclass hierarchy — this avoids the
// factory-loading problem and the GrayscaleFilter mutation bug.

/**
 * @class Image
 * @brief A concrete class for storing and manipulating image data.
 * @details Manages image dimensions, channel counts (1 for grayscale, 3 for RGB), 
 * and raw pixel data. It includes utilities for file I/O, alpha channel stripping, 
 * and bulk colour-space conversions.
 */
class Image {
private:
    int width;                      ///< The width of the image in pixels.
    int height;                     ///< The height of the image in pixels.
    int channels;                   ///< The number of colour channels (1 = grayscale, 3 = RGB).
    std::vector<unsigned char> data; ///< Flat vector storing the raw pixel data.

public:
    // ---- Constructors ----
    
    /**
     * @brief Default constructor. Initializes an empty image.
     */
    Image();

    /**
     * @brief Constructs a blank (zeroed) image with specified dimensions and channels.
     * @param w The width of the image.
     * @param h The height of the image.
     * @param c The number of channels (1 or 3).
     */
    Image(int w, int h, int c);   // blank (zeroed) image with c channels

    /**
     * @brief Constructs an image from a raw pixel buffer.
     * @details If srcChannels == 4, the alpha channel is silently stripped to 3-ch RGB.
     * @param rawData Pointer to the raw image data.
     * @param w The width of the image.
     * @param h The height of the image.
     * @param srcChannels The number of channels in the source data.
     */
    Image(const unsigned char* rawData, int w, int h, int srcChannels);

    // ---- File I/O  (uses stb_image / stb_image_write) ----
    
    /**
     * @brief Static factory: loads a file and returns a new Image.
     * @param filename The path to the image file to load.
     * @return A unique pointer to the loaded Image, or nullptr on failure.
     */
    static std::unique_ptr<Image> load(const std::string& filename);

    /**
     * @brief Saves the current image to disk.
     * @details The output format (PNG / JPG / BMP) is chosen by the file extension.
     * @param filename The path where the image should be saved.
     * @return True if the save was successful, false otherwise.
     */
    bool save(const std::string& filename) const;
    
    /**
     * @brief Removes the alpha channel from the image, converting it to 3 channels (RGB).
     */
    void removeAlpha();

    // Properties 
    
    /**
     * @brief Gets the width of the image.
     * @return The width in pixels.
     */
    int  getWidth()    const;

    /**
     * @brief Gets the height of the image.
     * @return The height in pixels.
     */
    int  getHeight()   const;

    /**
     * @brief Gets the number of channels in the image.
     * @return The channel count (e.g., 1 or 3).
     */
    int  getChannels() const;

    /**
     * @brief Sets the number of channels for the image.
     * @param c The new channel count.
     */
    void setChannels(int c);

    // Pixel access (unsigned char, with edge-clamp) 
    
    /**
     * @brief Gets the pixel value at specific coordinates, with edge-clamping.
     * @param x The x-coordinate.
     * @param y The y-coordinate.
     * @param c The channel index (defaults to 0).
     * @return The unsigned char value of the pixel channel.
     */
    unsigned char getPixel(int x, int y, int c = 0) const;

    /**
     * @brief Sets the pixel value at specific coordinates.
     * @param x The x-coordinate.
     * @param y The y-coordinate.
     * @param c The channel index.
     * @param value The value to set (0-255).
     */
    void          setPixel(int x, int y, int c, unsigned char value);

    // Pixel access — wider types for filter math
    
    /**
     * @brief Gets the pixel value as a float, with edge-clamping.
     * @param x The x-coordinate.
     * @param y The y-coordinate.
     * @param c The channel index (defaults to 0).
     * @return The float value in the range [0.0f, 255.0f].
     */
    float getPixelAsFloat(int x, int y, int c = 0) const;

    /**
     * @brief Sets a pixel using an integer value, clamping it to [0, 255] before storing.
     * @param x The x-coordinate.
     * @param y The y-coordinate.
     * @param c The channel index.
     * @param value The integer value to store (will be clamped).
     */
    void  setPixelClamped(int x, int y, int c, int value);



    // Raw data access 
    
    /**
     * @brief Gets a mutable reference to the internal raw pixel data.
     * @return A reference to the underlying std::vector.
     */
    std::vector<unsigned char>&       getData();

    /**
     * @brief Gets a constant reference to the internal raw pixel data.
     * @return A const reference to the underlying std::vector.
     */
    const std::vector<unsigned char>& getData() const;

    /**
     * @brief Replaces all internal data from a raw buffer.
     * @details Follows the same rules as the 4-argument constructor.
     * @param rawData Pointer to the new raw image data.
     * @param w The new width.
     * @param h The new height.
     * @param srcChannels The number of channels in the source data.
     */
    void loadFromData(const unsigned char* rawData, int w, int h, int srcChannels);

    /**
     * @brief Creates a deep copy of the image.
     * @return A unique pointer to the newly cloned Image.
     */
    std::unique_ptr<Image> clone() const;

    // RGBA to RGB utility
    
    /**
     * @brief Utility to strip the alpha channel from raw RGBA data.
     * @param rgbaData Pointer to the raw 4-channel data.
     * @param pixelCount The total number of pixels.
     * @return A vector containing the 3-channel RGB data.
     */
    static std::vector<unsigned char> stripAlphaChannel(
        const unsigned char* rgbaData, int pixelCount);
        
    /**
     * @brief In-place version of alpha channel stripping for existing 4-channel images.
     */
    void stripAlphaChannel();  // in-place version (for existing 4-channel images)
    

    // Single-pixel colour-space conversions (static helpers) 
    
    /**
     * @brief Converts an RGB pixel to HSL colour space.
     * @param r Red value (0-255).
     * @param g Green value (0-255).
     * @param b Blue value (0-255).
     * @return The converted HSLPixel.
     */
    static HSLPixel RGBtoHSL(unsigned char r, unsigned char g, unsigned char b);

    /**
     * @brief Converts an HSL pixel back to RGB colour space.
     * @param hsl The HSLPixel to convert.
     * @return The converted RGBPixel.
     */
    static RGBPixel HSLtoRGB(const HSLPixel& hsl);

    /**
     * @brief Converts an RGB pixel to HSV colour space.
     * @param r Red value (0-255).
     * @param g Green value (0-255).
     * @param b Blue value (0-255).
     * @return The converted HSVPixel.
     */
    static HSVPixel RGBtoHSV(unsigned char r, unsigned char g, unsigned char b);

    /**
     * @brief Converts an HSV pixel back to RGB colour space.
     * @param hsv The HSVPixel to convert.
     * @return The converted RGBPixel.
     */
    static RGBPixel HSVtoRGB(const HSVPixel& hsv);


    // Batch colour-space conversions 
    
    /**
     * @brief Converts the entire image to an array of HSL pixels.
     * @details Only valid when channels == 3; throws std::runtime_error otherwise.
     * @return A vector of HSLPixel data representing the image.
     */
    std::vector<HSLPixel> toHSL() const;

    /**
     * @brief Converts the entire image to an array of HSV pixels.
     * @details Only valid when channels == 3; throws std::runtime_error otherwise.
     * @return A vector of HSVPixel data representing the image.
     */
    std::vector<HSVPixel> toHSV() const;

    /**
     * @brief Replaces the image data by converting an array of HSL pixels back to RGB.
     * @param hslData The vector of HSLPixels to convert and load.
     */
    void fromHSL(const std::vector<HSLPixel>& hslData);

    /**
     * @brief Replaces the image data by converting an array of HSV pixels back to RGB.
     * @param hsvData The vector of HSVPixels to convert and load.
     */
    void fromHSV(const std::vector<HSVPixel>& hsvData);
};

#endif
