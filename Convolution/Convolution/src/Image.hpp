#pragma once

#include "Vector3.hpp"
#include "Kernel.hpp"

/// <summary>
/// Class representing RGB image.
/// </summary>
class Image {
public:

	/// <summary>
	/// Load image using stb_image library, make it grayscale and store it as Vector3 array. This constructor also creates a CDF.
	/// </summary>
	/// <param name="fileName">file path</param>
	Image(const char* fileName);

	/// <summary>
	/// Not copyable or movable
	/// </summary>
	Image(const Image&) = delete;
	void operator=(const Image&) = delete;
	Image(Image&&) = delete;
	Image& operator=(Image&&) = delete;

	/// <summary>
	/// Save transformed image as HDR file.
	/// </summary>
	/// <param name="fileName">file path</param>
	void SaveHDR(const char* fileName);

	/// <summary>
	/// Save transformed image as PNG file.
	/// </summary>
	/// <param name="fileName">file path</param>
	void SavePNG(const char* fileName);

	/// <summary>
	/// Return width of the image.
	/// </summary>
	/// <returns>width</returns>
	int Width();

	/// <summary>
	/// Return height of the image.
	/// </summary>
	/// <returns>height</returns>
	int Height();

	/// <summary>
	/// Return pointer do the original image array.
	/// </summary>
	/// <returns>data pointer</returns>
	float* DataPtr();

	/// <summary>
	/// Get RGB value of the original image at a given position
	/// </summary>
	/// <param name="x">horizontal position</param>
	/// <param name="y">vertical position</param>
	/// <returns>RGB value</returns>
	float Lookup(int x, int y);

	/// <summary>
	/// Get RGB value of the original image at a given position
	/// </summary>
	/// <param name="x">horizontal position</param>
	/// <param name="y">vertical position</param>
	/// <returns>RGB value</returns>
	float LookupT(int x, int y);

	/// <summary>
	/// Return the number of pixels of a given intensity of the original image.
	/// </summary>
	/// <param name="intensity">integer intensity (0-255)</param>
	/// <returns>number of pixels</returns>
	int HistogramValue(int intensity);

	/// <summary>
	/// Return the number of pixels of a given intensity of the transformed image.
	/// </summary>
	/// <param name="intensity">integer intensity (0-255)</param>
	/// <returns>number of pixels</returns>
	int HistogramValueT(int intensity);

	/// <summary>
	/// Return the cumulative histogram value of a given intensity of the original image.
	/// </summary>
	/// <param name="intensity">integer intensity (0-255)</param>
	/// <returns>cumulative value</returns>
	float DistributionValue(int intensity);

	/// <summary>
	/// Return the cumulative histogram value of a given intensity of the transformed image.
	/// </summary>
	/// <param name="intensity">integer intensity (0-255)</param>
	/// <returns>cumulative value</returns>
	float DistributionValueT(int intensity);

	/// <summary>
	/// Return the maximum of the histogram of the original image.
	/// </summary>
	/// <returns>maximal</returns>
	int HistogramMax();

	/// <summary>
	/// Return the maximum of the histogram of the transformed image.
	/// </summary>
	/// <returns>maximal</returns>
	int HistogramMaxT();

	/// <summary>
	/// Perform gamma correction on the original image and store it to the transformed image.
	/// </summary>
	void GammaCorrection();

	/// <summary>
	/// Perform histogram equalization on the original image and store it to the transformed image.
	/// </summary>
	void EqualizeHistogram();

	/// <summary>
	/// Perform threshold transformation on the original image and store it to the transformed image.
	/// </summary>
	void Threshold();

	/// <summary>
	/// Perform negative intensity transformation on the original image and store it to the transformed image.
	/// </summary>
	void Negative();

	/// <summary>
	/// Perform quantization on the original image and store it to the transformed image.
	/// </summary>
	void Quantization();

	/// <summary>
	/// Perform non-linear contrast transformation on the original image and store it to the transformed image.
	/// </summary>
	void NonLinearContrast();

	/// <summary>
	/// Perform fourier transform on the original image and store it to the transformed image.
	/// </summary>
	void FFT();

	/// <summary>
	/// Perform high-pass filtering on the original image and store it to the transformed image.
	/// </summary>
	void HighPassFilter();

	/// <summary>
	/// Perform low-pass filtering on the original image and store it to the transformed image.
	/// </summary>
	void LowPassFilter();


	void RemoveArtifactsCameraMan();

	void ApplyGaussianFilter(GaussianKernel2D& kernel);

	void ApplySeparableGaussianFilter(GaussianKernel1D& kernel);

private:

	/// <summary>
	/// Update CDF of after image transformation
	/// </summary>
	void UpdateTransformedCDF();

	int histogram[256]; // Histogram of the original image
	int histogramT[256]; // Histogram of the transformed image
	float distribution[256]; // CDF of the original image (not normalized)
	float distributionT[256]; // CDF of the transformed image (not normalized)
	int componentsPerPixel = 3; // Number of channels in one pixel
	int histogramMax; // Maximum in the histogram of the original image
	int histogramMaxT; // Maximum in the histogram of the transformed image
	int width; // Image width
	int height; // Image height
	std::unique_ptr<float[]> data; // Pointer to the original image data
	std::unique_ptr<float[]> dataT; // Pointer to the transformed image data

};