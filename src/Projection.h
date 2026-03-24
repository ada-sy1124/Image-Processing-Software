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
 
 * @file Projection.h
 * @brief Defines operations for creating 2D projections from 3D volumetric data.
 * @details This file provides a static class, Projection, which contains methods 
 * to perform various intensity projections (Maximum, Minimum, Average, and Median) 
 * along the Z-axis of a 3D volume, optionally within a specified slice range (slab).
 */

#ifndef PROJECTION_H
#define PROJECTION_H

#include "Volume.h"
#include "Image.h"

/**
 * @class Projection
 * @brief A utility class for generating 2D image projections from a 3D volume.
 * @details All projection methods are static. They reduce volumetric data along 
 * the depth (Z-axis) into a single 2D image based on specific statistical criteria 
 * (e.g., maximum, minimum, mean, median).
 */
class Projection {
public:
    /**
     * @brief Performs a Maximum Intensity Projection (MIP) on the volume.
     * @details Projects the voxel with the highest intensity value along the viewing 
     * ray onto the 2D image. Often used for visualizing bright structures like blood vessels.
     * @param volume The 3D volume to project.
     * @param zStart The starting Z-index (inclusive) of the slab. Defaults to -1 (full volume).
     * @param zEnd The ending Z-index (inclusive) of the slab. Defaults to -1 (full volume).
     * @return A 2D Image representing the maximum intensity projection.
     */
    // zStart/zEnd default to -1 meaning "use full volume".
    // When specified, they are 0-indexed inclusive bounds for the z slab.
    static Image maxIntensity(const Volume& volume, int zStart = -1, int zEnd = -1);

    /**
     * @brief Performs a Minimum Intensity Projection (MinIP) on the volume.
     * @details Projects the voxel with the lowest intensity value along the viewing 
     * ray onto the 2D image. Often used for visualizing dark structures like airways.
     * @param volume The 3D volume to project.
     * @param zStart The starting Z-index (inclusive). Defaults to -1.
     * @param zEnd The ending Z-index (inclusive). Defaults to -1.
     * @return A 2D Image representing the minimum intensity projection.
     */
    static Image minIntensity(const Volume& volume, int zStart = -1, int zEnd = -1);

    /**
     * @brief Performs an Average Intensity Projection (AIP) on the volume.
     * @details Calculates the mean intensity value of all voxels along the viewing 
     * ray for each pixel in the 2D image.
     * @param volume The 3D volume to project.
     * @param zStart The starting Z-index (inclusive). Defaults to -1.
     * @param zEnd The ending Z-index (inclusive). Defaults to -1.
     * @return A 2D Image representing the average intensity projection.
     */
    static Image averageIntensity(const Volume& volume, int zStart = -1, int zEnd = -1);

    /**
     * @brief Performs a Median Intensity Projection on the volume.
     * @details Finds the median intensity value of all voxels along the viewing 
     * ray for each pixel in the 2D image.
     * @param volume The 3D volume to project.
     * @param zStart The starting Z-index (inclusive). Defaults to -1.
     * @param zEnd The ending Z-index (inclusive). Defaults to -1.
     * @return A 2D Image representing the median intensity projection.
     */
    static Image medianIntensity(const Volume& volume, int zStart = -1, int zEnd = -1);

private:
    /**
     * @brief Helper method to resolve and validate the Z-slab boundaries.
     * @details Converts the default -1 values to actual volume bounds (0 to depth-1) 
     * and ensures the indices are within valid ranges.
     * @param depth The total depth (number of slices) of the volume.
     * @param zStart Reference to the starting Z-index variable to be resolved.
     * @param zEnd Reference to the ending Z-index variable to be resolved.
     */
    static void resolveSlabBounds(int depth, int& zStart, int& zEnd);
};

#endif
