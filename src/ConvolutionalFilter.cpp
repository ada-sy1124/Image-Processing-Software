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

 * @file ConvolutionalFilter.cpp
 * @brief Implementation of various convolutional image filters.
 * @details This file implements the methods for filters such as BoxBlur, GaussianBlur, 
 * MedianBlur, Sharpen, edge detection (Sobel, Prewitt, Scharr, RobertsCross), and Emboss.
*/

#include "ConvolutionalFilter.h"
#include "SimpleFilter.h"
#include <stdexcept>
#include <cmath>
#include <algorithm>


// BoxBlurFilter

BoxBlurFilter::BoxBlurFilter(int size) : size(size) {}

void BoxBlurFilter::apply(Image &image)
{
    // To efficiently apply the box blur I will be moving the kernel across the image and use a sliding window approach.
    // On each next position the sum is the previous sum minus the leftmost column
    // of the previous kernel plus the rightmost column of the new kernel.

    int width = image.getWidth();
    int height = image.getHeight();
    int channels = image.getChannels();
    int kernelRadius = size / 2;
    float sizeRecip = 1.0f / size; // precompute reciprocal for efficiency

    if (size <= 0 || size % 2 == 0)
    {
        throw std::invalid_argument("Kernel size must be a positive odd integer");
    }

    if (size == 1)
    {
        // No blurring needed for a 1x1 kernel
        return;
    }

    // Iterate over each channel
    for (int c = 0; c < channels; ++c)
    {
        // Create an extended sumImage with additional rows at the top and bottom
        Image sumImage(width, height + 2 * kernelRadius, channels);

        // Compute the horizontal sums for the current channel
        for (int y = 0; y < height + 2 * kernelRadius; ++y)
        {
            int sum = 0;
            for (int x = 0; x < width; ++x)
            {
                if (x == 0)
                {
                    // Initialize the sum for the first pixel in each row
                    // Could do something like sum = kernelradius + 1 times first pixel but this is more consistent
                    for (int i = -kernelRadius; i <= kernelRadius; ++i)
                    {
                        sum += image.getPixel(i, y - kernelRadius, c);
                    }
                }
                else
                {
                    // Sliding window approach for subsequent pixels
                    sum += image.getPixel(x + kernelRadius, y - kernelRadius, c) - image.getPixel(x - kernelRadius - 1, y - kernelRadius, c);
                }
                // Store the horizontal sum in the sumImage
                // Divide by size now to avoid overflow
                // Will lose some precision especially for larger kernels
                // but avoids having to change data type and is memory efficient
                // add 0.5 for rounding
                sumImage.setPixel(x, y, c, (sum + 0.5f) * sizeRecip);
            }
        }

        // Compute the vertical sums and copy the blurred values directly to the original image
        for (int x = 0; x < width; ++x)
        {
            int sum = 0;
            // starts where the actual image would start and end where it would end
            for (int y = kernelRadius; y < height + kernelRadius; ++y)
            {
                if (y == kernelRadius)
                {
                    // Initialize the sum for the first pixel in each column
                    for (int i = 0; i <= 2 * kernelRadius; ++i)
                    {
                        sum += sumImage.getPixel(x, i, c);
                    }
                }
                else
                {
                    // Sliding window approach for subsequent pixels
                    sum += sumImage.getPixel(x, y + kernelRadius, c) - sumImage.getPixel(x, y - kernelRadius - 1, c);
                }
                // add 0.5 for rounding
                unsigned char blurredValue = (sum+0.5f) * sizeRecip; // divide by size to get the average
                image.setPixel(x, y - kernelRadius, c, blurredValue);
            }
        }
    }
}

// GaussianBlurFilter

GaussianBlurFilter::GaussianBlurFilter(int size, double stdev)
    : size(size), stdev(stdev) {}

// The kernel is separable so we can apply the 1D kernel horizontally and then vertically for efficiency
std::vector<float> calculateGaussianKernel(int size, float sigma) {
    std::vector<float> kernel(size);
    float sum = 0.0f;

    for (int i = 0; i < size; ++i) {
        int x = i - size / 2;
        kernel[i] = std::exp(-0.5f * (x * x) / (sigma * sigma));
        sum += kernel[i];
    }

    // Normalize the kernel
    for (int i = 0; i < size; ++i) {
        kernel[i] /= sum;
    }
    
    return kernel;
}

void GaussianBlurFilter::apply(Image& image) {
    int width = image.getWidth();
    int height = image.getHeight();
    int channels = image.getChannels();
    int kernelRadius = size / 2;

    if (size <= 0 || size % 2 == 0)
        throw std::invalid_argument("Kernel size must be a positive odd integer");
    if (stdev <= 0.0)
        throw std::invalid_argument("Standard deviation must be positive");
    if (size == 1)
        return;

    std::vector<float> kernel = calculateGaussianKernel(size, static_cast<float>(stdev));
    const float* kp = kernel.data();

    // grab raw pointer so we don't have to call getPixel() in the inner loop
    const unsigned char* src = image.getData().data();
    const int rowStride = width * channels;

    // float buffer for horizontal pass so we don't lose precision
    // by converting back to unsigned char between passes
    std::vector<float> tmp(static_cast<size_t>(width) * height);
    std::vector<unsigned char> out(image.getData().size());

    for (int c = 0; c < channels; ++c) {

        // horizontal pass: convolve along x
        for (int y = 0; y < height; ++y) {
            const unsigned char* row = src + y * rowStride;
            float* dstRow = tmp.data() + y * width;

            // left edge where kernel goes past x=0
            for (int x = 0; x < kernelRadius && x < width; ++x) {
                float sum = 0.0f;
                for (int k = -kernelRadius; k <= kernelRadius; ++k) {
                    int sx = x + k;
                    if (sx < 0) sx = 0;
                    else if (sx >= width) sx = width - 1;
                    sum += row[sx * channels + c] * kp[k + kernelRadius];
                }
                dstRow[x] = sum;
            }

            // interior pixels, no bounds check needed
            for (int x = kernelRadius; x < width - kernelRadius; ++x) {
                float sum = 0.0f;
                const unsigned char* base = row + (x - kernelRadius) * channels + c;
                for (int k = 0; k < size; ++k) {
                    sum += base[k * channels] * kp[k];
                }
                dstRow[x] = sum;
            }

            // right edge pixels
            int rightStart = (width - kernelRadius > kernelRadius) ? width - kernelRadius : kernelRadius;
            for (int x = rightStart; x < width; ++x) {
                float sum = 0.0f;
                for (int k = -kernelRadius; k <= kernelRadius; ++k) {
                    int sx = x + k;
                    if (sx < 0) sx = 0;
                    else if (sx >= width) sx = width - 1;
                    sum += row[sx * channels + c] * kp[k + kernelRadius];
                }
                dstRow[x] = sum;
            }
        }

        // vertical pass: convolve along y, read from tmp, write to out
        for (int x = 0; x < width; ++x) {

            // top edge
            for (int y = 0; y < kernelRadius && y < height; ++y) {
                float sum = 0.0f;
                for (int k = -kernelRadius; k <= kernelRadius; ++k) {
                    int sy = y + k;
                    if (sy < 0) sy = 0;
                    else if (sy >= height) sy = height - 1;
                    sum += tmp[sy * width + x] * kp[k + kernelRadius];
                }
                int v = static_cast<int>(sum + 0.5f);
                if (v < 0) v = 0; else if (v > 255) v = 255;
                out[(y * width + x) * channels + c] = static_cast<unsigned char>(v);
            }

            // interior, no bounds check
            for (int y = kernelRadius; y < height - kernelRadius; ++y) {
                float sum = 0.0f;
                const float* col = tmp.data() + (y - kernelRadius) * width + x;
                for (int k = 0; k < size; ++k) {
                    sum += col[k * width] * kp[k];
                }
                int v = static_cast<int>(sum + 0.5f);
                if (v < 0) v = 0; else if (v > 255) v = 255;
                out[(y * width + x) * channels + c] = static_cast<unsigned char>(v);
            }

            // bottom edge
            int botStart = (height - kernelRadius > kernelRadius) ? height - kernelRadius : kernelRadius;
            for (int y = botStart; y < height; ++y) {
                float sum = 0.0f;
                for (int k = -kernelRadius; k <= kernelRadius; ++k) {
                    int sy = y + k;
                    if (sy < 0) sy = 0;
                    else if (sy >= height) sy = height - 1;
                    sum += tmp[sy * width + x] * kp[k + kernelRadius];
                }
                int v = static_cast<int>(sum + 0.5f);
                if (v < 0) v = 0; else if (v > 255) v = 255;
                out[(y * width + x) * channels + c] = static_cast<unsigned char>(v);
            }
        }
    }

    image.getData() = std::move(out);
}

// MedianBlurFilter

MedianBlurFilter::MedianBlurFilter(int size) : size(size) {}

void MedianBlurFilter::apply(Image &image)
{
    // median blur = replace each pixel by median value of its neighbor

    // discard even kernels, not in the spec and are uncentered
    if (size % 2 == 0)
    {
        throw std::runtime_error("MedianBlurFilter: kernel size must be odd (e.g. 3, 5, 7)");
    }
    // reject negative kernel sizes
    if (size <= 0)
    {
        throw std::runtime_error("MedianBlurFilter: kernel size must be >0");
    }
    int w = image.getWidth();
    int h = image.getHeight();
    int c = image.getChannels(); // 1 for greyscale, 3 for RGB
    int radius = size / 2;       // kernel radius e.g. 3x3 = radius 1, 5x5 = radius 2

    // set MAX kernel size to be fixed ratio based on image size, in this case...
    int maxSize = std::min((w / 2) | 1, (h / 2) | 1); // max = half the size of the smallest dimension, since blurs to a blob
    maxSize = std::min(maxSize, 99); // also do not exceed 99 (a very high upper bound, unlikely to be used)
    if (size > maxSize) {
        throw std::runtime_error("MedianBlurFilter: kernel size unreasonably large for image");
    }

    // get a reference to the raw pixel buffer for fast direct access
    // greyscale = [G G G ...], RGB = [R G B R G B ...] row by row
    const std::vector<unsigned char> &src = image.getData();

    // allocate output buffer same size as input, instead of copying
    // write results here and then swap into the image at the end
    std::vector<unsigned char> out(w * h * c, 0);

    // collect all neighbor values in the kernel into an array,
    // use a fixed-size array on the stack
    std::vector<unsigned char> neighbors(size * size); // supports up to size x size kernel

    // iterate over every pixel
    for (int y = 0; y < h; y++)
    { // loop over rows
        for (int x = 0; x < w; x++)
        { // loop over cols

            // check for if in safe region of interior (kernel avoids padded area)
            bool interior = (x >= radius && x < w - radius &&
                             y >= radius && y < h - radius);

            // process each channel independently e.g. for greyscale c=1, for RGB c=3
            for (int ch = 0; ch < c; ch++)
            {

                int count = 0;

                // loop over the kernel neighborhood
                for (int ky = -radius; ky <= radius; ky++)
                {
                    for (int kx = -radius; kx <= radius; kx++)
                    {

                        // check if kernel seeps into padded area, if so, use built-in getPixel which handles edges
                        // o/w use more efficient indexing
                        unsigned char pixel;
                        if (interior)
                        {
                            pixel = src[((y + ky) * w + (x + kx)) * c + ch]; // direct index = (row * width + col) * channels + channel
                        }
                        else
                        {
                            pixel = image.getPixel(x + kx, y + ky, ch); // getPixel handles out-of-bounds
                        }

                        neighbors[count] = pixel;
                        count++;
                    }
                }

                /* QUICKSELECT ALGORITHM = find the median element w/o fully sorting
                 * should be faster than insertion sort O(n) with O(n^2) worst case
                 * instead of O(n^2) for insertion
                 *
                 * steps:
                 * 1. pick a pivot, partition array into smaller/larger halves
                 * 2. only recurse into the half that contains median index
                 * 3. repeat until median lands in correct position
                 */
                int medianIdx = count / 2;
                int lo = 0;
                int hi = count - 1;

                while (lo < hi)
                {
                    // use middle element as pivot to avoid worst case on sorted data
                    int mid = lo + (hi - lo) / 2;

                    // swap pivot to end so it's out of the way during partitioning
                    unsigned char temp = neighbors[mid];
                    neighbors[mid] = neighbors[hi];
                    neighbors[hi] = temp;
                    unsigned char pivot = neighbors[hi];

                    // partition i.e. move everything smaller than pivot to the left
                    int storeIdx = lo;
                    for (int i = lo; i < hi; i++)
                    {
                        if (neighbors[i] < pivot)
                        {
                            // swap neighbors[i] and neighbors[storeIdx]
                            temp = neighbors[i];
                            neighbors[i] = neighbors[storeIdx];
                            neighbors[storeIdx] = temp;
                            storeIdx++;
                        }
                    }

                    // put pivot in its final sorted position
                    temp = neighbors[storeIdx];
                    neighbors[storeIdx] = neighbors[hi];
                    neighbors[hi] = temp;

                    // now neighbors[storeIdx] is in its correct sorted position
                    // if that's the median index, then done here
                    // otherwise narrow the search to the half containing medianIdx
                    if (storeIdx == medianIdx)
                    {
                        break;
                    }
                    else if (storeIdx < medianIdx)
                    {
                        lo = storeIdx + 1; // median is in the right half
                    }
                    else
                    {
                        hi = storeIdx - 1; // median is in the left half
                    }
                }

                // quickselect guarantees the median is now at index count/2
                // e.g. for 9 values = index 4, for 25 values = index 12
                // rest of array is not fully sorted
                unsigned char median = neighbors[count / 2];

                // write to output buffer
                out[(y * w + x) * c + ch] = median;
            }
        }
    }

    // swap output into the image, no full copy, just move ownership
    image.getData() = std::move(out);
}

// SharpenFilter

void SharpenFilter::apply(Image& image) {
    // keep an unmodified copy so we don't read already-sharpened pixels
    auto original = image.clone();
    int w = image.getWidth();
    int h = image.getHeight();
    int ch = image.getChannels();

    // the spec defines I_sharp = I_original + G where G uses centre=4,
    // but we combine both into one kernel with centre=5 (4+1) so we only
    // need one pass, one clamp, and no extra buffer for the intermediate G
    const float kernel[3][3] = {
        { 0, -1,  0},
        {-1,  5, -1},
        { 0, -1,  0}
    };

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            for (int c = 0; c < ch; ++c) {
                float sum = 0.0f;
                // convolve the 3x3 neighbourhood around (x, y)
                for (int ky = -1; ky <= 1; ++ky) {
                    for (int kx = -1; kx <= 1; ++kx) {
                        // +1 offset maps [-1,1] range to [0,2] array indices
                        sum += kernel[ky + 1][kx + 1]
                             * original->getPixelAsFloat(x + kx, y + ky, c);
                    }
                }
                // clamp to [0, 255] to avoid overflow from negative weights
                image.setPixelClamped(x, y, c, static_cast<int>(sum));
            }
        }
    }
}

// general edge detection for a variable size kernel
// uses two kernels Gx and Gy of any kernel size to detect edges
void EdgeDetection(Image& image, 
                   const std::vector<std::vector<int>>& Gx, 
                   const std::vector<std::vector<int>>& Gy, 
                   double max_magnitude)
{

    // convert image to greyscale, so only one channel
    GreyscaleFilter grey;
    grey.apply(image);

    int w = image.getWidth();
    int h = image.getHeight();
    int radius = Gx.size() / 2; // kernel radius = half the kernel size e.g 3x3 = radius 1

    // get a reference to the raw pixel buffer for fast direct access
    // [G G G ...] row by row
    const std::vector<unsigned char> &src = image.getData();

    // allocate output buffer same size as input, instead of copying
    // write results here and then swap into the image at the end
    std::vector<unsigned char> out(w * h, 0);

    // iterate over image
    for (int y = 0; y < h; y++)
    { // loop over rows
        for (int x = 0; x < w; x++)
        { // loop over cols

            // check for if in safe region of interior (kernel avoids padded area)
            bool interior = (x >= radius && x < w - radius &&
                             y >= radius && y < h - radius);

            // one sum for each kernel
            int sumX = 0; // sum for Gx
            int sumY = 0; // sum for Gy

            // loop over the kernel neighborhood as defined by radius
            for (int ky = -radius; ky <= radius; ky++)
            {
                for (int kx = -radius; kx <= radius; kx++)
                {

                    // check if kernel seeps into padded area, if so, use built-in getPixel which handles edges
                    // o/w use more efficient indexing
                    // read the pixel value once as an int
                    int pixel;
                    if (interior)
                    {
                        pixel = static_cast<int>(src[(y + ky) * w + (x + kx)]);
                    }
                    else
                    {
                        pixel = static_cast<int>(image.getPixel(x + kx, y + ky, 0));
                    }

                    // use kernel weights
                    // ky+radius and kx+radius shift from [-radius,radius] to [0,size] (needed to index cpp vectors)
                    // e.g. 3x3 [-1,1] goes to [0,2]
                    int weightX = Gx[ky + radius][kx + radius];
                    int weightY = Gy[ky + radius][kx + radius];

                    // accumulate weighted sums for horizontal and vertical gradients
                    sumX += pixel * weightX;
                    sumY += pixel * weightY;
                }
            }

            // combine the two gradients into a single magnitude = overall edge strength
            // overall edge strength at each pixel calculated as magnitude sqrt(Gx^2 + Gy^2)
            // note: doubles instead of ints here
            double magnitude = std::sqrt(
                static_cast<double>(sumX) * sumX +
                static_cast<double>(sumY) * sumY);

            // the maximum possible magnitude for each kernel is when all pixels are 255
            // that would thus make the magnitude

            // to save computation, the max kernel size will be hard-coded for each kernel
            // so we do not need to calculate it on every run. see each filter fxn to see math
            magnitude = magnitude / max_magnitude * 255.0;
            if (magnitude > 255.0) magnitude = 255.0; // safety clamp, just in case

            // write to output buffer
            out[y * w + x] = static_cast<unsigned char>(magnitude);
        }
    }

    image = Image(w, h, 1); // output is a greyscale image
    image.getData() = std::move(out);
}

// SobelFilter

void SobelFilter::apply(Image &image)
{
    // two 3x3 kernels
    std::vector<std::vector<int>> Gx = {// horizontal kernel (detects left/right changes)
                                        {-1, 0, 1},
                                        {-2, 0, 2},
                                        {-1, 0, 1}};
    std::vector<std::vector<int>> Gy = {// vertical kernel (detects up/down changes)
                                        {-1, -2, -1},
                                        {0, 0, 0},
                                        {1, 2, 1}};
    double max_magnitude = 1442.5; // sqrt((4*255)^2 + (4*255)^2)
    EdgeDetection(image, Gx, Gy, max_magnitude);
}

// PrewittFilter

void PrewittFilter::apply(Image &image)
{
    // two 3x3 kernels
    std::vector<std::vector<int>> Gx = {// horizontal kernel (detects left/right changes)
                                        {-1, 0, 1},
                                        {-1, 0, 1},
                                        {-1, 0, 1}};
    std::vector<std::vector<int>> Gy = {// vertical kernel (detects up/down changes)
                                        {-1, -1, -1},
                                        {0, 0, 0},
                                        {1, 1, 1}};
    double max_magnitude = 1081.87; // sqrt((3*255)^2 + (3*255)^2)
    EdgeDetection(image, Gx, Gy, max_magnitude);
}

// ScharrFilter

void ScharrFilter::apply(Image &image)
{
    // two 3x3 kernels
    std::vector<std::vector<int>> Gx = {// horizontal kernel (detects left/right changes)
                                        {-3, 0, 3},
                                        {-10, 0, 10},
                                        {-3, 0, 3}};
    std::vector<std::vector<int>> Gy = {// vertical kernel (detects up/down changes)
                                        {-3, -10, -3},
                                        {0, 0, 0},
                                        {3, 10, 3}};
    double max_magnitude = 5769.99; // sqrt((16*255)^2 + (16*255)^2)
    EdgeDetection(image, Gx, Gy, max_magnitude);
}

// RobertsCrossFilter

void RobertsCrossFilter::apply(Image &image)
{
    std::vector<std::vector<int>> Gx = {
        {1, 0, 0},
        {0, -1, 0},
        {0, 0, 0}};
    std::vector<std::vector<int>> Gy = {
        {0, 1, 0},
        {-1, 0, 0},
        {0, 0, 0}};
    double max_magnitude = 360.624; // sqrt((1*255)^2 + (1*255)^2)
    EdgeDetection(image, Gx, Gy, max_magnitude);
}

// EmbossFilter
// write comments for the following codes

EmbossFilter::EmbossFilter(float strength, const std::string &direction,
                           const std::string &type)
    : strength(strength), direction(direction), type(type) {}
// build the emboss kernel based on the specified direction
static void buildEmbossKernel(const std::string &dir, float kernel[3][3])
{
    // emboss kernels for different directions
    if (dir == "NW")
    {
        float k[3][3] = {{-2, -1, 0}, {-1, 0, 1}, {0, 1, 2}};
        for (int r = 0; r < 3; ++r)
            for (int c = 0; c < 3; ++c)
                kernel[r][c] = k[r][c];
    }
    else if (dir == "NE")
    {
        float k[3][3] = {{0, -1, -2}, {1, 0, -1}, {2, 1, 0}};
        for (int r = 0; r < 3; ++r)
            for (int c = 0; c < 3; ++c)
                kernel[r][c] = k[r][c];
    }
    else if (dir == "SE")
    {
        float k[3][3] = {{2, 1, 0}, {1, 0, -1}, {0, -1, -2}};
        for (int r = 0; r < 3; ++r)
            for (int c = 0; c < 3; ++c)
                kernel[r][c] = k[r][c];
    }
    else if (dir == "SW")
    {
        float k[3][3] = {{0, 1, 2}, {-1, 0, 1}, {-2, -1, 0}};
        for (int r = 0; r < 3; ++r)
            for (int c = 0; c < 3; ++c)
                kernel[r][c] = k[r][c];
    }
    else
    {
        throw std::invalid_argument("EmbossFilter: unknown direction '" + dir + "'");
    }
}

// helper function to convolve a single channel with the given kernel and strength
static std::vector<float> convolveChannel(const std::vector<float> &src,
                                          int width, int height,
                                          float kernel[3][3],
                                          float strength)
{
    std::vector<float> dst(width * height);
    // iterate over each pixel in the output image
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            float acc = 0.0f;
            for (int ky = -1; ky <= 1; ++ky)
            {
                for (int kx = -1; kx <= 1; ++kx)
                {
                    int nx = std::clamp(x + kx, 0, width - 1);
                    int ny = std::clamp(y + ky, 0, height - 1);
                    acc += kernel[ky + 1][kx + 1] * src[ny * width + nx];
                }
            }
            // apply strength and add 128 to shift from [-255,255] to [0,255]
            float val = strength * acc + 128.0f;
            dst[y * width + x] = std::clamp(val, 0.0f, 255.0f);
        }
    }
    return dst;
}

// apply the emboss filter to the image
void EmbossFilter::apply(Image &image)
{
    float kernel[3][3];
    // build the emboss kernel based on the specified direction
    buildEmbossKernel(direction, kernel);

    int w = image.getWidth();
    int h = image.getHeight();

    // delete alpha channel if present
    if (image.getChannels() == 4)
    {
        image.removeAlpha();
    }

    int ch = image.getChannels();
    // if single channel, convolve directly
    if (ch == 1)
    {
        std::vector<float> src(w * h);
        for (int i = 0; i < w * h; ++i)
            src[i] = static_cast<float>(image.getData()[i]);
        std::vector<float> dst = convolveChannel(src, w, h, kernel, strength);
        for (int i = 0; i < w * h; ++i)
            image.getData()[i] = static_cast<unsigned char>(dst[i]);
        return;
    }

    if (ch != 3)
        throw std::runtime_error("EmbossFilter: unsupported channel count " + std::to_string(ch));
    
    if (type == "HSV")
    {
        std::vector<HSVPixel> hsv = image.toHSV();
        std::vector<float> vChan(w * h);
        for (int i = 0; i < w * h; ++i)
            vChan[i] = hsv[i].v * 255.0f;
        std::vector<float> vOut = convolveChannel(vChan, w, h, kernel, strength);
        for (int i = 0; i < w * h; ++i)
            hsv[i].v = vOut[i] / 255.0f;
        image.fromHSV(hsv);
    }
    else if (type == "HSL")
    {
        std::vector<HSLPixel> hsl = image.toHSL();
        std::vector<float> lChan(w * h);
        for (int i = 0; i < w * h; ++i)
            lChan[i] = hsl[i].l * 255.0f;
        std::vector<float> lOut = convolveChannel(lChan, w, h, kernel, strength);
        for (int i = 0; i < w * h; ++i)
            hsl[i].l = lOut[i] / 255.0f;
        image.fromHSL(hsl);
    }
    else
    {
        throw std::invalid_argument("EmbossFilter: unknown type '" + type + "'");
    }
}
