set(SOURCE_DIR ${CMAKE_SOURCE_DIR})
set(BINARY_DIR ${CMAKE_BINARY_DIR})
set(OUTPUT_DIR ${CMAKE_BINARY_DIR}/output)

# Ensure the output directory exists when running cmake
file(MAKE_DIRECTORY ${OUTPUT_DIR})


### TEST IMAGE PROCESSING FUNCTIONALITY ###
add_test(NAME Brightness1 COMMAND APImageFilters -i ${SOURCE_DIR}/Images/small.png -b 100 ${OUTPUT_DIR}/brightness1.png)
add_test(NAME Brightness2 COMMAND APImageFilters -i ${SOURCE_DIR}/Images/small.png --brightness -100 ${OUTPUT_DIR}/brightness2.png)
add_test(NAME Greyscale1 COMMAND APImageFilters -i ${SOURCE_DIR}/Images/small.png -g ${OUTPUT_DIR}/greyscale1.png)
add_test(NAME Greyscale2 COMMAND APImageFilters -i ${SOURCE_DIR}/Images/small.png --greyscale ${OUTPUT_DIR}/greyscale2.png)
add_test(NAME HistogramHSV COMMAND APImageFilters -i ${SOURCE_DIR}/Images/small.png -h HSV ${OUTPUT_DIR}/histogram1.png)
add_test(NAME HistogramHSL COMMAND APImageFilters -i ${SOURCE_DIR}/Images/small.png --histogram HSL ${OUTPUT_DIR}/histogram2.png)
add_test(NAME BlurGaussian COMMAND APImageFilters -i ${SOURCE_DIR}/Images/small.png -r Gaussian 5 2.0 ${OUTPUT_DIR}/blur1.png)
add_test(NAME BlurBox COMMAND APImageFilters -i ${SOURCE_DIR}/Images/small.png -r Box 7 ${OUTPUT_DIR}/blur2.png)
add_test(NAME BlurMedian COMMAND APImageFilters -i ${SOURCE_DIR}/Images/small.png --blur Median 3 ${OUTPUT_DIR}/blur3.png)
add_test(NAME EdgeSobel COMMAND APImageFilters -i ${SOURCE_DIR}/Images/small.png --edge Sobel ${OUTPUT_DIR}/edge1.png)
add_test(NAME EdgePrewitt COMMAND APImageFilters -i ${SOURCE_DIR}/Images/small.png --edge Prewitt ${OUTPUT_DIR}/edge2.png)
add_test(NAME EdgeScharr COMMAND APImageFilters -i ${SOURCE_DIR}/Images/small.png -e Scharr ${OUTPUT_DIR}/edge3.png)
add_test(NAME EdgeRobertsCross COMMAND APImageFilters -i ${SOURCE_DIR}/Images/small.png -e RobertsCross ${OUTPUT_DIR}/edge4.png)
add_test(NAME Sharpen1 COMMAND APImageFilters -i ${SOURCE_DIR}/Images/small.png -L ${OUTPUT_DIR}/sharpen1.png)
add_test(NAME Sharpen2 COMMAND APImageFilters -i ${SOURCE_DIR}/Images/small.png --sharpen ${OUTPUT_DIR}/sharpen2.png)
add_test(NAME SaltPepper5 COMMAND APImageFilters -i ${SOURCE_DIR}/Images/small.png --saltpepper 5 ${OUTPUT_DIR}/saltpepper1.png)
add_test(NAME SaltPepper75 COMMAND APImageFilters -i ${SOURCE_DIR}/Images/small.png -n 75 ${OUTPUT_DIR}/saltpepper2.png)
add_test(NAME ThresholdHSV128 COMMAND APImageFilters -i ${SOURCE_DIR}/Images/small.png --threshold 128 HSV ${OUTPUT_DIR}/threshold1.png)
add_test(NAME ThresholdHSL64 COMMAND APImageFilters -i ${SOURCE_DIR}/Images/small.png -t 64 HSL ${OUTPUT_DIR}/threshold2.png)
add_test(NAME EmbossNW COMMAND APImageFilters -i ${SOURCE_DIR}/Images/small.png --emboss 2.5 NW HSV ${OUTPUT_DIR}/embossNW.png)
add_test(NAME EmbossSE COMMAND APImageFilters -i ${SOURCE_DIR}/Images/small.png -m 3.5 NW HSV ${OUTPUT_DIR}/embossSE.png)
add_test(NAME MultiFilter COMMAND APImageFilters
         -i ${SOURCE_DIR}/Images/small.png -b 100 -g -r Gaussian 5 1.0 -e Sobel ${OUTPUT_DIR}/multifilter.png)

# Give these short timeouts, since the test image is small
set_tests_properties(Brightness1 PROPERTIES TIMEOUT 10)
set_tests_properties(Brightness2 PROPERTIES TIMEOUT 10)
set_tests_properties(Greyscale1 PROPERTIES TIMEOUT 10)
set_tests_properties(Greyscale2 PROPERTIES TIMEOUT 10)
set_tests_properties(HistogramHSV PROPERTIES TIMEOUT 10)
set_tests_properties(HistogramHSL PROPERTIES TIMEOUT 10)
set_tests_properties(BlurGaussian PROPERTIES TIMEOUT 10)
set_tests_properties(BlurBox PROPERTIES TIMEOUT 10)
set_tests_properties(BlurMedian PROPERTIES TIMEOUT 10)
set_tests_properties(EdgeSobel PROPERTIES TIMEOUT 10)
set_tests_properties(EdgePrewitt PROPERTIES TIMEOUT 10)
set_tests_properties(EdgeScharr PROPERTIES TIMEOUT 10)
set_tests_properties(EdgeRobertsCross PROPERTIES TIMEOUT 10)
set_tests_properties(Sharpen1 PROPERTIES TIMEOUT 10)
set_tests_properties(Sharpen2 PROPERTIES TIMEOUT 10)
set_tests_properties(SaltPepper5 PROPERTIES TIMEOUT 10)
set_tests_properties(SaltPepper75 PROPERTIES TIMEOUT 10)
set_tests_properties(ThresholdHSV128 PROPERTIES TIMEOUT 10)
set_tests_properties(ThresholdHSL64 PROPERTIES TIMEOUT 10)
set_tests_properties(EmbossNW PROPERTIES TIMEOUT 10)
set_tests_properties(EmbossSE PROPERTIES TIMEOUT 10)
set_tests_properties(MultiFilter PROPERTIES TIMEOUT 60)

### TEST CORE VOLUME PROCESSING FUNCTIONALITY ###
add_test(NAME SliceXZ COMMAND APImageFilters -d ${SOURCE_DIR}/Scans/TestVolume/vol -s XZ 16 ${OUTPUT_DIR}/sliceXZ.png)
add_test(NAME SliceYZ COMMAND APImageFilters -d ${SOURCE_DIR}/Scans/TestVolume/vol --slice YZ 16 ${OUTPUT_DIR}/sliceYZ.png)
add_test(NAME ProjectionMIP COMMAND APImageFilters -d ${SOURCE_DIR}/Scans/TestVolume/vol -p MIP ${OUTPUT_DIR}/projectionMIP.png)
add_test(NAME ProjectionMinIP COMMAND APImageFilters -d ${SOURCE_DIR}/Scans/TestVolume/vol --projection MinIP ${OUTPUT_DIR}/projectionMinIP.png)
add_test(NAME ProjectionMeanAIP COMMAND APImageFilters -d ${SOURCE_DIR}/Scans/TestVolume/vol -p meanAIP ${OUTPUT_DIR}/projectionMeanAIP.png)
add_test(NAME ProjectionMedianAIP COMMAND APImageFilters -d ${SOURCE_DIR}/Scans/TestVolume/vol -p medianAIP ${OUTPUT_DIR}/projectionMedianAIP.png)

# Also test with filtering
add_test(NAME SliceXZGaussian COMMAND APImageFilters
         -d ${SOURCE_DIR}/Scans/TestVolume/vol -r Gaussian 3 2.0 -s XZ 16 ${OUTPUT_DIR}/sliceXZGaussian.png)
add_test(NAME SliceYZMedian COMMAND APImageFilters
         -d ${SOURCE_DIR}/Scans/TestVolume/vol --blur Median 3 --slice YZ 16 ${OUTPUT_DIR}/sliceYZMedian.png)
add_test(NAME ProjectionMIPGaussian COMMAND APImageFilters
         -d ${SOURCE_DIR}/Scans/TestVolume/vol -r Gaussian 3 1.0 -p MIP ${OUTPUT_DIR}/projectionMIPGaussian.png)
add_test(NAME ProjectionMinIPMedian COMMAND APImageFilters
         -d ${SOURCE_DIR}/Scans/TestVolume/vol --blur Median 3 --projection MIP ${OUTPUT_DIR}/projectionMIPMedian.png)

# And test the thin slab functionality
add_test(NAME ThinSlabSliceXZ COMMAND APImageFilters
         -d ${SOURCE_DIR}/Scans/TestVolume/vol -f 4 -l 28 -s XZ 16 ${OUTPUT_DIR}/sliceXZthinslab.png)
add_test(NAME ThinSlabSliceYZ COMMAND APImageFilters
         -d ${SOURCE_DIR}/Scans/TestVolume/vol --first 4 --last 28 -s YZ 16 ${OUTPUT_DIR}/sliceYZthinslab.png)
add_test(NAME ThinSlabProjectionMIP COMMAND APImageFilters
         -d ${SOURCE_DIR}/Scans/TestVolume/vol -f 4 -l 28 -p MIP ${OUTPUT_DIR}/projectionMIPthinslab.png)
add_test(NAME ThinSlabProjectionMinIP COMMAND APImageFilters
         -d ${SOURCE_DIR}/Scans/TestVolume/vol --first 4 --last 28 --projection MinIP ${OUTPUT_DIR}/projectionMinIPthinslab.png)
add_test(NAME ThinSlabSliceXZGaussian COMMAND APImageFilters
         -d ${SOURCE_DIR}/Scans/TestVolume/vol -f 4 -l 28 -r Gaussian 3 2.0 -s XZ 16 ${OUTPUT_DIR}/sliceXZGaussianthinslab.png)
add_test(NAME ThinSlabProjectMIPMedian COMMAND APImageFilters
         -d ${SOURCE_DIR}/Scans/TestVolume/vol --first 4 --last 28 -r Median 3 -p MIP ${OUTPUT_DIR}/projectionMIPMedianthinslab.png)

# Give these short timeouts, since the test volume is small
set_tests_properties(SliceXZ PROPERTIES TIMEOUT 60)
set_tests_properties(SliceYZ PROPERTIES TIMEOUT 60)
set_tests_properties(ProjectionMIP PROPERTIES TIMEOUT 60)
set_tests_properties(ProjectionMinIP PROPERTIES TIMEOUT 60)
set_tests_properties(ProjectionMeanAIP PROPERTIES TIMEOUT 60)
set_tests_properties(ProjectionMedianAIP PROPERTIES TIMEOUT 60)

set_tests_properties(SliceXZGaussian PROPERTIES TIMEOUT 120)
set_tests_properties(SliceYZMedian PROPERTIES TIMEOUT 120)
set_tests_properties(ProjectionMIPGaussian PROPERTIES TIMEOUT 120)
set_tests_properties(ProjectionMinIPMedian PROPERTIES TIMEOUT 120)

set_tests_properties(ThinSlabSliceXZ PROPERTIES TIMEOUT 60)
set_tests_properties(ThinSlabSliceYZ PROPERTIES TIMEOUT 60)
set_tests_properties(ThinSlabProjectionMIP PROPERTIES TIMEOUT 60)
set_tests_properties(ThinSlabProjectionMinIP PROPERTIES TIMEOUT 60)
set_tests_properties(ThinSlabSliceXZGaussian PROPERTIES TIMEOUT 120)
set_tests_properties(ThinSlabProjectMIPMedian PROPERTIES TIMEOUT 120)

