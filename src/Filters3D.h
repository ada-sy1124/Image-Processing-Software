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
 
 * @file Filters3D.h
 * @brief Defines 3D volumetric image filters.
 * @details This file contains the abstract Filter3D base class and concrete 
 * implementations for separable 3D Gaussian blurring and 3D Median blurring 
 * applied to volumetric data.
 */

#ifndef FILTERS3D_H
#define FILTERS3D_H
 
#include "Volume.h"

/**
 * @class Filter3D
 * @brief Base class for separable 3D convolution filters.
 * @details Provides an abstract interface for applying 3D filters to volumetric data.
 */
class Filter3D {
public:
    /**
     * @brief Default virtual destructor.
     */
    virtual ~Filter3D() {}
 
    /**
     * @brief Applies the 3D filter to the given volume.
     * @param volume Reference to the 3D volume object to be processed in-place.
     */
    virtual void apply(Volume& volume) = 0;
};

/**
 * @class GaussianBlur3DFilter
 * @brief 3D Gaussian blur filter.
 * @details Applies a separable 3D Gaussian blur to a volume to reduce noise and detail.
 */
class GaussianBlur3DFilter : public Filter3D {
private:
    int size;     ///< The size of the convolution kernel.
    double stdev; ///< The standard deviation (sigma) of the Gaussian distribution.
    char pivot;   ///< The pivot axis for separable convolution (e.g., 'x', 'y', or 'z').
    int axIndex;  ///< The index of the axis along which the filter is applied.
 
public:
    /**
     * @brief Constructs a GaussianBlur3DFilter object.
     * @param size The size of the Gaussian kernel.
     * @param stdev The standard deviation controlling the blur strength.
     */
    GaussianBlur3DFilter(int size, double stdev) : size(size), stdev(stdev) {}
    
    /**
     * @brief Applies the 3D Gaussian blur operation to the volume.
     * @param volume Reference to the 3D volume object to be processed.
     */
    void apply(Volume& volume) override;
 
    /**
     * @brief Sets the pivot axis for the separable convolution.
     * @param pivot The character representing the axis (e.g., 'x', 'y', 'z').
     */
    void setPivot(char pivot) {
        this->pivot = pivot;
    }
 
    /**
     * @brief Sets the axis index for the convolution pass.
     * @param axIndex The integer index representing the specific axis or slice.
     */
    void setIndex(int axIndex) {
        this->axIndex = axIndex;
    }
};

/**
 * @class MedianBlur3DFilter
 * @brief 3D Median blur filter.
 * @details Applies a 3D median filter to remove noise (like salt-and-pepper) while 
 * preserving edges in volumetric data.
 */
class MedianBlur3DFilter : public Filter3D {
private:
    int size; ///< The size of the 3D filtering window.
 
public:
    /**
     * @brief Constructs a MedianBlur3DFilter object.
     * @param size The size of the median filtering window (typically an odd number).
     */
    explicit MedianBlur3DFilter(int size);

    /**
     * @brief Applies the 3D median blur operation to the volume.
     * @param volume Reference to the 3D volume object to be processed.
     */
    void apply(Volume& volume) override;
};
 
#endif