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
 
 * @file Volume.h
 * @brief Defines the base Volume class for 3D volumetric data.
 * @details This file provides the core data structure for representing and 
 * manipulating 3D volumes (e.g., loaded from a sequence of 2D image slices). 
 * It handles voxel access, clamping, and file I/O operations.
 */

#ifndef VOLUME_H
#define VOLUME_H

#include <vector>
#include <string>
#include "Image.h"
#include <memory> // for std::unique_ptr

/**
 * @class Volume
 * @brief A class representing a 3D volume of pixel/voxel data.
 * @details Manages a flat 1D buffer that conceptually represents a 3D grid 
 * of voxels, supporting both single-channel (grayscale) and multi-channel (RGB) data.
 */
class Volume {
protected:
    int width;    ///< The x dimension (width) of the volume.
    int height;   ///< The y dimension (height) of the volume.
    int depth;    ///< The z dimension (depth or number of slices).
    int channels; ///< The number of channels per voxel (1 = greyscale, 3 = RGB).

    // flat buffer layout: data[((z * height + y) * width + x) * channels + c]
    /**
     * @brief The flat 1D vector storing the actual volumetric data.
     */
    std::vector<unsigned char> data;

public:

    // constructors
    /**
     * @brief Default constructor. Initializes an empty volume.
     */
    Volume(); // default = empty volume
    
    /**
     * @brief Constructs a volume with specified dimensions, initialized with zeroes.
     * @param w The width (x dimension).
     * @param h The height (y dimension).
     * @param d The depth (z dimension).
     * @param c The number of channels (defaults to 1).
     */
    Volume(int w, int h, int d, int c = 1); // volume with zeroes

    /**
     * @brief Virtual destructor for the Volume class.
     */
    virtual ~Volume(); // destructor

    // file input and output
    /**
     * @brief Loads a volume from a sequence of image files.
     * @param prefix The file path prefix (e.g., "Scans/TestVolume/vol").
     * @param first The starting slice index. Defaults to -1 (auto-detect).
     * @param last The ending slice index. Defaults to -1 (auto-detect).
     * @param extension The image file extension (e.g., ".png").
     * @return True if the volume was successfully loaded, false otherwise.
     */
    // prefix is e.g. "Scans/TestVolume/vol"
    // extension is e.g. ".png"
    // first/last are optional slice indices, -1 means auto-detect
    bool load(const std::string& prefix,
              int first = -1,
              int last = -1,
              std::string extension = ".png");

    // properties
    /**
     * @brief Gets the width (x dimension) of the volume.
     * @return The width in voxels.
     */
    int getWidth() const;

    /**
     * @brief Gets the height (y dimension) of the volume.
     * @return The height in voxels.
     */
    int getHeight() const;

    /**
     * @brief Gets the depth (z dimension) of the volume.
     * @return The depth in voxels (number of slices).
     */
    int getDepth() const;

    /**
     * @brief Gets the number of channels per voxel.
     * @return The channel count (e.g., 1 or 3).
     */
    int getChannels() const;

    // access voxels — multi-channel
    /**
     * @brief Retrieves the voxel value at the specified coordinates and channel.
     * @details Out of bounds x/y/z coordinates are clamped to the nearest valid index.
     * @param x The x-coordinate.
     * @param y The y-coordinate.
     * @param z The z-coordinate.
     * @param c The channel index (defaults to 0).
     * @return The unsigned char value of the voxel channel.
     */
    // out of bounds x/y/z are clamped to nearest valid
    unsigned char getVoxel(int x, int y, int z, int c = 0) const;

    /**
     * @brief Sets the voxel value at the specified coordinates and channel.
     * @param x The x-coordinate.
     * @param y The y-coordinate.
     * @param z The z-coordinate.
     * @param c The channel index.
     * @param value The unsigned char value to set.
     */
    void setVoxel(int x, int y, int z, int c, unsigned char value);

    /**
     * @brief Convenience method to set the voxel value on channel 0.
     * @param x The x-coordinate.
     * @param y The y-coordinate.
     * @param z The z-coordinate.
     * @param value The unsigned char value to set.
     */
    void setVoxel(int x, int y, int z, unsigned char value); // convenience: channel 0

    /**
     * @brief Retrieves the voxel value as a float.
     * @param x The x-coordinate.
     * @param y The y-coordinate.
     * @param z The z-coordinate.
     * @param c The channel index (defaults to 0).
     * @return The float value in the range [0.0, 255.0].
     */
    // return voxel value as float in [0.0, 255.0]
    float getVoxelAsFloat(int x, int y, int z, int c = 0) const;

    /**
     * @brief Sets the voxel value using an integer, clamping it to valid bounds.
     * @param x The x-coordinate.
     * @param y The y-coordinate.
     * @param z The z-coordinate.
     * @param c The channel index.
     * @param value The integer value (will be clamped to [0, 255] before storing).
     */
    // accepts int, clamps to [0, 255] before storing
    void setVoxelClamped(int x, int y, int z, int c, int value);

    /**
     * @brief Convenience method to set a clamped integer voxel value on channel 0.
     * @param x The x-coordinate.
     * @param y The y-coordinate.
     * @param z The z-coordinate.
     * @param value The integer value to clamp and set.
     */
    void setVoxelClamped(int x, int y, int z, int value); // convenience: channel 0

    // raw data access
    /**
     * @brief Gets a mutable reference to the internal raw voxel data.
     * @return A reference to the underlying std::vector.
     */
    std::vector<unsigned char>& getData();

    /**
     * @brief Gets a constant reference to the internal raw voxel data.
     * @return A const reference to the underlying std::vector.
     */
    const std::vector<unsigned char>& getData() const;

    // deep copy
    /**
     * @brief Creates a deep copy of the volume.
     * @return A unique pointer to the newly cloned Volume.
     */
    std::unique_ptr<Volume> clone() const;


};

#endif
