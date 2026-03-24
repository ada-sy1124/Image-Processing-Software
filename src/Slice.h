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
 
 * @file Slice.h
 * @brief Defines operations for extracting 2D slices from 3D volumetric data.
 * @details This file provides a static Slice class that allows users to extract 
 * specific orthogonal 2D planes (e.g., YZ or XZ planes) from a given 3D Volume.
 */

#ifndef SLICE_H
#define SLICE_H

#include "Image.h"
#include "Volume.h"

/**
 * @class Slice
 * @brief A utility class for extracting 2D images from a 3D volume.
 * @details Contains static methods to slice through a 3D volumetric dataset 
 * along a specified axis, returning the result as a standard 2D Image.
 */
class Slice {
public:
    /**
     * @brief Extracts a 2D slice from the volume based on a user-provided coordinate.
     * @param vol The 3D volume to slice.
     * @param coord The 1-based coordinate index for the slice.
     * @param pivot The axis to fix ('X' for YZ plane, 'Y' for XZ plane).
     * @return A 2D Image representing the extracted slice.
     */
    // pivot = 'X' → YZ plane (fix x, user gives x coordinate)
    // pivot = 'Y' → XZ plane (fix y, user gives y coordinate)
    // coord is 1-based as per spec
    static Image slice(const Volume& vol, int coord, char pivot);

private:
    /**
     * @brief Internal helper to perform the actual 2D extraction using 0-based indexing.
     * @param vol The 3D volume to extract from.
     * @param fixedIdx The 0-based coordinate index along the pivot axis.
     * @param pivot The axis to fix ('X' or 'Y').
     * @return A 2D Image representing the extracted slice.
     */
    static Image extract(const Volume& vol, int fixedIdx, char pivot);
};

#endif
