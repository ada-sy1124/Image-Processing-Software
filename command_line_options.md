# Command line options

Our program accepts the command line options detailed below. 
The first option should always be `-i <input_image>` or `-d <data_volume>` to specify whether the program is going to be processing an image or volume, and to specify where to find the input. The last option should be the output image name (this does not require a flag). 
The program then processes the input image or volume according to the options provided, and saves the output image to the specified location.

Note that each option (other than the -i/-d flags and the output name) can use either the long or short flag format: e.g. `--greyscale` or `-g`.

## Image Processing Options

- Input Image: `-i <input_image>`
- Output Image: `<output_image>` (must have extension .png or .jpg)

### Filters
- Greyscale: `--greyscale` or `-g`
- Brightness: `--brightness <value>` or `-b <value>`
  - `<value>` can be an integer value or `auto` to set the average value to 128. If neither is provided it will be set to `auto`.
- Histogram Equalisation: `--histogram <type>` or `-h <type>` (e.g., HSV, HSL)
  - Note that `<type>` is an optional parameter.  For RGB impages, it determines whether to use the intensity from the HSV or HSL colourspace. The default, if not provided, is HSV.
- Blur: `--blur <type> <size> [<stdev>]` or `-r <type> <size> [<stdev>]` (e.g., Gaussian 5 2.0, Box 7, Median 3.
    - Accepted `<type>` is Box, Gaussian, or Median
    - `<size>` must be an odd valued integer
    - note `<stdev>` is only required for Gaussian
- Edge Detection: `--edge <type>` or `-e <type>` (e.g., Sobel, Prewitt, Scharr, RobertsCross)
- Laplacian Sharpening: `--sharpen` or `-L`
- Emboss: `--emboss <strength> <direction> <type>` or `-m <strength> <direction> <type>`
  - `<strength>` is a positive float, in the range 0–10.
  - `<direction>` is one of `NW`, `NE`, `SE`, or `SW`
  - Note that `<type>` is an optional parameter. For RGB impages, it determines whether to use the intensity from the HSV or HSL colourspace. The default, if not provided, uses HSV.
- Salt and Pepper Noise: `--saltpepper <amount>` or `-n <amount>`
  - `<amount>` is a percentage amount of noise
- Threshold: `--threshold <value> <type>` or `-t <value> <type>` (e.g., 128 HSV, 64 HSL)
  - Note that `<type>` is an optional parameter. For RGB impages, it determines whether to use the intensity from the HSV or HSL colourspace. The default, if not provided, is HSV.

You can specify one or multiple filters, by chaining the options together (e.g. `-g -r Median 3` will convert to greyscale and then apply a median blur filter).

## Volume Processing Options

- Input Volume: `-d <data_volume>`
  - Note that `<data_volume>` should include the path and the common beginning of the filename of the images of the volume, without the numerical part of the filename.
  - For example, `Scans/TestVolume/vol`, or `Scans/fracture/granite1_`
- Output Image: `<output_image>` (must have extension .png or .jpg)

### Volume Reading
- First Index: `--first <index>` or `-f <index>` (optional)
- Last Index: `--last <index>` or `-l <index>` (optional)
- Extension: `--extension <ext>` or `-x <ext>` (optional; default is png).

If first and last index are not defined, all images in volume will be read.

### Volume Blur Filter
- Blur: `--blur <type> <size> [<stdev>]` or `-r <type> <size> [<stdev>]` (e.g., `Gaussian 3 2.0, Median 3`; note `<stdev>` is only required for Gaussian)

Note that the blur filter is optional in volume processing; if it is spcified, the subsequent slice or projection will be applied to the blurred volume, otherwise it will be applied to the original volume.

### Volume Processing Options
- Slice: `--slice <plane> <constant>` or `-s <plane> <constant>` (e.g., `XZ 16`, `YZ 16`)
- Projection: `--projection <type>` or `-p <type>` (MIP, MinIP, meanAIP, medianAIP)
- Thin Slab (Projection only): `--slab <min_z> <max_z>` (1-indexed z coordinates for the sub-range of slices to project over; if not specified, all loaded slices are used)

Note that it is necessary to specify one of these processing options (Slice or Projection), as this defines the output image from the command.

## Example Commands

- Brightness: `./APImageFilters -i input.png -b 100 output.png`
- Greyscale: `./APImageFilters -i input.png -g output.png`
- Histogram Equalisation: `./APImageFilters -i input.png -h HSV output.png`
- Blur (Gaussian): `./APImageFilters -i input.png -r Gaussian 5 2.0 output.png`
- Edge Detection (Sobel): `./APImageFilters -i input.png --edge Sobel output.png`
- Laplacian Sharpening: `./APImageFilters -i input.png --sharpen output.png`
- Emboss: `./APImageFilters -i input.png --emboss 2.5 NE HSV output.png`
- Salt and Pepper Noise: `./APImageFilters -i input.png --saltpepper 5 output.png`
- Threshold: `./APImageFilters -i input.png --threshold 128 HSV output.png`
- Volume Slice (XZ): `./APImageFilters -d volume -s XZ 16 output.png`
- Volume Projection (MIP): `./APImageFilters -d volume -p MIP output.png`
- Volume Projection with Thin Slab: `./APImageFilters -d volume -p MIP --slab 10 50 output.png`

