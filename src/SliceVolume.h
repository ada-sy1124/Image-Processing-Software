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
 
 * @file SliceVolume.h
 * @brief Defines a specialized Volume class for slice manipulation.
 * @details Inherits from the base Volume class to provide functionality 
 * for rotating the volumetric dataset and extracting 2D slices along specific planes.
 */

#include "Volume.h"
#include "Image.h"
#include <vector>

/**
 * @class SliceVolume
 * @brief Represents a 3D volume that supports plane rotation and slice extraction.
 * @details Extends the basic Volume data structure with methods specifically 
 * designed to change the orientation (plane) and extract corresponding 2D Images.
 */
class SliceVolume : public Volume {
private:
    /**
     * @brief The currently active plane for slicing (e.g., 'X', 'Y', or 'Z').
     */
    char rotatedPlane = 'X'; // set by rotate()

public:
    /**
     * @brief Rotates the active plane of the volume for subsequent slicing.
     * @param coord The coordinate index around which the rotation is applied.
     * @param kernSize The size of the kernel or neighborhood involved in the rotation.
     * @param plane The character representing the new active plane (e.g., 'X', 'Y', 'Z').
     */
    void rotate(int coord, int kernSize, char plane);

    /**
     * @brief Extracts a 2D slice from the volume along the currently rotated plane.
     * @param coord The coordinate index at which to extract the slice.
     * @return A 2D Image representing the extracted slice.
     */
    Image slice(int coord);

};
