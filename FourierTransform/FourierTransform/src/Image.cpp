#include "Image.hpp"

#define STB_IMAGE_IMPLEMENTATION  
#include <stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION  
#include <stb_image_write.h>


#include <fftw3.h>

Image::Image(const char* fileName) {
	// Load image using stb_image library
	stbi_set_flip_vertically_on_load(true);
	data = std::unique_ptr<float[]>(reinterpret_cast<float*>(stbi_loadf(
		fileName, &width, &height, &componentsPerPixel, 1)));

	if (!data) {
		std::cerr << "ERROR: Could not load texture image file '" << fileName << "'.\n";
		width = height = 0;
		return;
	}

	dataT = std::make_unique<float[]>(width * height);

	// Make grayscale image and create integer histogram
	for (int i = 0; i < height; i++)
	{
		for (int j = 0; j < width; j++)
		{
			float brightness = std::sqrtf(data[i * width + j]);
			histogram[static_cast<int>(255.99f * brightness)] += 1;
			data[i * width + j] = brightness;
			dataT[i * width + j] = data[i * width + j];
		}
	}

	distribution[0] = histogram[0];
	distributionT[0] = histogram[0];

	histogramMax = histogram[0];
	histogramT[0] = histogram[0];
	for (int i = 1; i < 256; i++)
	{
		distribution[i] = distribution[i - 1] + histogram[i];
		histogramT[i] = histogram[i];
		histogramMax = std::max(histogram[i], histogramMax);
	}
	histogramMaxT = histogramMax;

	// Normalize
	float histogramIntegral = distribution[255];
	for (int i = 0; i < 256; i++)
	{
		distribution[i] /= histogramIntegral;
		distributionT[i] = distribution[i];
	}
}

void Image::SaveHDR(const char* fileName) {
	// Flip vertically
	std::unique_ptr<Color3[]> tmp(new Color3[width * height]);
	for (int i = 0; i < height; i++)
	{
		for (int j = 0; j < width; j++)
		{
			tmp[i * width + j].x = dataT[(height - i - 1) * width + j];
			tmp[i * width + j].y = dataT[(height - i - 1) * width + j];
			tmp[i * width + j].z = dataT[(height - i - 1) * width + j];
		}
	}
	stbi_write_hdr(fileName, width, height, 3, reinterpret_cast<float*>(tmp.get()));
}

void Image::SavePNG(const char* fileName) {
	stbi_write_png(fileName, width, height, componentsPerPixel, reinterpret_cast<float*>(dataT.get()), componentsPerPixel * width);
}

int Image::Width() {
	return width;
}

int Image::Height() {
	return height;
}

float* Image::DataPtr() {
	return data.get();
}

float Image::Lookup(int x, int y) {
	return data[y * width + x];
}


float Image::LookupT(int x, int y) {
	return dataT[y * width + x];
}

int Image::HistogramValue(int intensity) {
	return histogram[intensity];
}

int Image::HistogramValueT(int intensity) {
	return histogramT[intensity];
}

float Image::DistributionValue(int intensity) {
	return distribution[intensity];
}

float Image::DistributionValueT(int intensity) {
	return distributionT[intensity];
}

int Image::HistogramMax() {
	return histogramMax;
}

int Image::HistogramMaxT() {
	return histogramMaxT;
}

void Image::GammaCorrection() {
	std::fill(histogramT, histogramT + 256, 0);
	histogramMaxT = 0;
	for (int i = 0; i < height; i++)
	{
		for (int j = 0; j < width; j++)
		{
			dataT[i * width + j] = std::sqrtf(data[i * width + j]);
			int brightness = static_cast<int>(255.99f * dataT[i * width + j]);
			histogramT[brightness] += 1;
			histogramMaxT = std::max(histogramMaxT, histogramT[brightness]);
		}
	}
	// Update CDF
	UpdateTransformedCDF();
}

void Image::EqualizeHistogram() {
	std::fill(histogramT, histogramT + 256, 0);
	histogramMaxT = 0;
	for (int i = 0; i < height; i++)
	{
		for (int j = 0; j < width; j++)
		{
			dataT[i * width + j] = distribution[static_cast<int>(data[i * width + j] * 255.99f)];
			int brightness = static_cast<int>(255.99f * dataT[i * width + j]);
			histogramT[brightness] += 1;
			histogramMaxT = std::max(histogramMaxT, histogramT[brightness]);
		}
	}
	// Update CDF
	UpdateTransformedCDF();
}


void Image::Threshold() {
	std::fill(histogramT, histogramT + 256, 0);
	histogramMaxT = 0;
	for (int i = 0; i < height; i++)
	{
		for (int j = 0; j < width; j++)
		{
			dataT[i * width + j] = data[i * width + j] > 0.5f ? 1.0f : 0.0f;
			int brightness = static_cast<int>(255.99f * dataT[i * width + j]);
			histogramT[brightness] += 1;
			histogramMaxT = std::max(histogramMaxT, histogramT[brightness]);
		}
	}
	// Update CDF
	UpdateTransformedCDF();
}

void Image::Negative() {
	std::fill(histogramT, histogramT + 256, 0);
	histogramMaxT = 0;
	for (int i = 0; i < height; i++)
	{
		for (int j = 0; j < width; j++)
		{
			dataT[i * width + j] = 1.0f - data[i * width + j];
			int brightness = static_cast<int>(255.99f * dataT[i * width + j]);
			histogramT[brightness] += 1;
			histogramMaxT = std::max(histogramMaxT, histogramT[brightness]);
		}
	}
	// Update CDF
	UpdateTransformedCDF();
}

void Image::Quantization() {
	float q = 8.0f;
	std::fill(histogramT, histogramT + 256, 0);
	histogramMaxT = 0;
	for (int i = 0; i < height; i++)
	{
		for (int j = 0; j < width; j++)
		{
			dataT[i * width + j] = std::floor(int(data[i * width + j] * q)) / q;
			int brightness = static_cast<int>(255.99f * dataT[i * width + j]);
			histogramT[brightness] += 1;
			histogramMaxT = std::max(histogramMaxT, histogramT[brightness]);
		}
	}
	// Update CDF
	UpdateTransformedCDF();
}

void Image::NonLinearContrast() {
	float alpha = 0.5f;
	float gamma = 1.0f / (1.0f - alpha);
	std::fill(histogramT, histogramT + 256, 0);
	histogramMaxT = 0;
	for (int i = 0; i < height; i++)
	{
		for (int j = 0; j < width; j++)
		{
			if (data[i * width + j] < 0.5f) {
				dataT[i * width + j] = 0.5f * std::powf(2.0f * data[i * width + j], gamma);
			}
			else {
				dataT[i * width + j] = 1.0f - 0.5f * std::powf(2.0f - 2.0f * data[i * width + j], gamma);
			}

			int brightness = static_cast<int>(255.99f * dataT[i * width + j]);
			histogramT[brightness] += 1;
			histogramMaxT = std::max(histogramMaxT, histogramT[brightness]);
		}
	}
	// Update CDF
	UpdateTransformedCDF();
}

void Image::FFT() {
	std::fill(histogramT, histogramT + 256, 0);
	histogramMaxT = 0;

	// create complex 2D image
	fftw_complex* in = new fftw_complex[width * height];
	fftw_complex* out = new fftw_complex[width * height];
	for (size_t i = 0; i < width * height; i++)
	{
		in[i][0] = double(data[i]);
		in[i][1] = 0;
	}

	fftw_plan planFwd = fftw_plan_dft_2d(width, height, in, out, FFTW_FORWARD, FFTW_ESTIMATE);
	fftw_execute(planFwd);
	fftw_destroy_plan(planFwd);

	// swap quadrants
	int h2 = height / 2;
	int w2 = width / 2;
	for (int i = 0; i < h2; i++)
	{
		for (int j = 0; j < w2; j++)
		{
			auto tmp13R = out[i * width + j][0];
			auto tmp13C = out[i * width + j][1];
			out[i * width + j][0] = out[(i + h2) * width + j + w2][0];
			out[i * width + j][1] = out[(i + h2) * width + j + w2][1];
			out[(i + h2) * width + j + w2][0] = tmp13R;
			out[(i + h2) * width + j + w2][1] = tmp13C;

			auto tmp24R = out[(i + h2) * width + j][0];
			auto tmp24C = out[(i + h2) * width + j][1];
			out[(i + h2) * width + j][0] = out[i * width + j + w2][0];
			out[(i + h2) * width + j][1] = out[i * width + j + w2][1];
			out[i* width + j + w2][0] = tmp24R;
			out[i* width + j + w2][1] = tmp24C;
		}
	}

	// copy the magnitude of the result back into the image
	double clipMag{ double(width) };
	for (size_t i = 0; i < width * height; i++)
	{
		double re = out[i][0];
		double im = out[i][1];
		double mag = sqrt(re * re + im * im);
		double clipped = mag > clipMag ? 1.0 : mag / clipMag;
		dataT[i] = float(sqrt(clipped));

		int brightness = static_cast<int>(255.99f * dataT[i]);
		histogramT[brightness] += 1;
		histogramMaxT = std::max(histogramMaxT, histogramT[brightness]);
	}

	// FFTW cleanup
	fftw_cleanup();
	delete[] in;
	delete[] out;

	// Update CDF
	UpdateTransformedCDF();
	
	//histogramMaxT = 0;
	//for (int i = 0; i < height; i++)
	//{
	//	for (int j = 0; j < width; j++)
	//	{
	//		dataT[i * width + j] = distributionT[static_cast<int>(dataT[i * width + j] * 255.99f)];
	//		int brightness = static_cast<int>(255.99f * dataT[i * width + j]);
	//		histogramT[brightness] += 1;
	//		histogramMaxT = std::max(histogramMaxT, histogramT[brightness]);
	//	}
	//}
	//// Update CDF
	//UpdateTransformedCDF();

}

void Image::HighPassFilter() {
	std::fill(histogramT, histogramT + 256, 0);
	histogramMaxT = 0;

	// create complex 2D image
	fftw_complex* in = new fftw_complex[width * height];
	fftw_complex* out = new fftw_complex[width * height];
	for (size_t i = 0; i < width * height; i++)
	{
		in[i][0] = double(data[i]);
		in[i][1] = 0;
	}

	fftw_plan planFwd = fftw_plan_dft_2d(width, height, in, out, FFTW_FORWARD, FFTW_ESTIMATE);
	fftw_execute(planFwd);
	fftw_destroy_plan(planFwd);

	// high pass filter
	//out[0][0] = 0;
	//out[0][1] = 0;


		// swap quadrants
	int h2 = height / 2;
	int w2 = width / 2;
	for (int i = 0; i < h2; i++)
	{
		for (int j = 0; j < w2; j++)
		{
			auto tmp13R = out[i * width + j][0];
			auto tmp13C = out[i * width + j][1];
			out[i * width + j][0] = out[(i + h2) * width + j + w2][0];
			out[i * width + j][1] = out[(i + h2) * width + j + w2][1];
			out[(i + h2) * width + j + w2][0] = tmp13R;
			out[(i + h2) * width + j + w2][1] = tmp13C;

			auto tmp24R = out[(i + h2) * width + j][0];
			auto tmp24C = out[(i + h2) * width + j][1];
			out[(i + h2) * width + j][0] = out[i * width + j + w2][0];
			out[(i + h2) * width + j][1] = out[i * width + j + w2][1];
			out[i * width + j + w2][0] = tmp24R;
			out[i * width + j + w2][1] = tmp24C;
		}
	}

	// low pass filter (circle)
	int filterSize = 3;
	int centerX = width / 2;
	int centerY = height / 2;
	for (size_t i = 0; i < height; i++)
	{
		for (size_t j = 0; j < width; j++)
		{
			if ((j - centerX) * (j - centerX) + (i - centerY) * (i - centerY) <= filterSize * filterSize) {
				out[i * width + j][0] = 0;
				out[i * width + j][1] = 0;
			}
		}
	}

	// swap quadrants
	for (int i = 0; i < h2; i++)
	{
		for (int j = 0; j < w2; j++)
		{
			auto tmp13R = out[i * width + j][0];
			auto tmp13C = out[i * width + j][1];
			out[i * width + j][0] = out[(i + h2) * width + j + w2][0];
			out[i * width + j][1] = out[(i + h2) * width + j + w2][1];
			out[(i + h2) * width + j + w2][0] = tmp13R;
			out[(i + h2) * width + j + w2][1] = tmp13C;

			auto tmp24R = out[(i + h2) * width + j][0];
			auto tmp24C = out[(i + h2) * width + j][1];
			out[(i + h2) * width + j][0] = out[i * width + j + w2][0];
			out[(i + h2) * width + j][1] = out[i * width + j + w2][1];
			out[i * width + j + w2][0] = tmp24R;
			out[i * width + j + w2][1] = tmp24C;
		}
	}

	fftw_plan planBck = fftw_plan_dft_2d(width, height, out, in, FFTW_BACKWARD, FFTW_ESTIMATE);
	fftw_execute(planBck);
	fftw_destroy_plan(planBck);

	// normalize restored image
	for (size_t i = 0; i < width * height; i++)
	{
		in[i][0] /= double(width * height);
		in[i][1] /= double(width * height);
	}

	for (size_t i = 0; i < width * height; i++)
	{
		double re = in[i][0];
		double im = in[i][1];
		double mag = sqrt(re * re + im * im);
		dataT[i] = float(mag);
		int brightness = static_cast<int>(255.99f * dataT[i]);
		histogramT[brightness] += 1;
		histogramMaxT = std::max(histogramMaxT, histogramT[brightness]);
	}


	// FFTW cleanup
	fftw_cleanup();
	delete[] in;
	delete[] out;

	// Update CDF
	UpdateTransformedCDF();
}

void Image::LowPassFilter() {
	std::fill(histogramT, histogramT + 256, 0);
	histogramMaxT = 0;

	// create complex 2D image
	fftw_complex* in = new fftw_complex[width * height];
	fftw_complex* out = new fftw_complex[width * height];
	for (size_t i = 0; i < width * height; i++)
	{
		in[i][0] = double(data[i]);
		in[i][1] = 0;
	}

	fftw_plan planFwd = fftw_plan_dft_2d(width, height, in, out, FFTW_FORWARD, FFTW_ESTIMATE);
	fftw_execute(planFwd);
	fftw_destroy_plan(planFwd);

	// swap quadrants
	int h2 = height / 2;
	int w2 = width / 2;
	for (int i = 0; i < h2; i++)
	{
		for (int j = 0; j < w2; j++)
		{
			auto tmp13R = out[i * width + j][0];
			auto tmp13C = out[i * width + j][1];
			out[i * width + j][0] = out[(i + h2) * width + j + w2][0];
			out[i * width + j][1] = out[(i + h2) * width + j + w2][1];
			out[(i + h2) * width + j + w2][0] = tmp13R;
			out[(i + h2) * width + j + w2][1] = tmp13C;

			auto tmp24R = out[(i + h2) * width + j][0];
			auto tmp24C = out[(i + h2) * width + j][1];
			out[(i + h2) * width + j][0] = out[i * width + j + w2][0];
			out[(i + h2) * width + j][1] = out[i * width + j + w2][1];
			out[i * width + j + w2][0] = tmp24R;
			out[i * width + j + w2][1] = tmp24C;
		}
	}

	// low pass filter (circle)
	int filterSize = 40;
	int centerX = width / 2;
	int centerY = height / 2;
	for (size_t i = 0; i < height; i++)
	{
		for (size_t j = 0; j < width; j++)
		{
			if ((j - centerX) * (j - centerX) + (i - centerY) * (i - centerY) > filterSize * filterSize) {
				out[i * width + j][0] = 0;
				out[i * width + j][1] = 0;
			}
		}
	}

	// swap quadrants
	for (int i = 0; i < h2; i++)
	{
		for (int j = 0; j < w2; j++)
		{
			auto tmp13R = out[i * width + j][0];
			auto tmp13C = out[i * width + j][1];
			out[i * width + j][0] = out[(i + h2) * width + j + w2][0];
			out[i * width + j][1] = out[(i + h2) * width + j + w2][1];
			out[(i + h2) * width + j + w2][0] = tmp13R;
			out[(i + h2) * width + j + w2][1] = tmp13C;

			auto tmp24R = out[(i + h2) * width + j][0];
			auto tmp24C = out[(i + h2) * width + j][1];
			out[(i + h2) * width + j][0] = out[i * width + j + w2][0];
			out[(i + h2) * width + j][1] = out[i * width + j + w2][1];
			out[i * width + j + w2][0] = tmp24R;
			out[i * width + j + w2][1] = tmp24C;
		}
	}


	fftw_plan planBck = fftw_plan_dft_2d(width, height, out, in, FFTW_BACKWARD, FFTW_ESTIMATE);
	fftw_execute(planBck);
	fftw_destroy_plan(planBck);

	// normalize restored image
	for (size_t i = 0; i < width * height; i++)
	{
		in[i][0] /= double(width * height);
		in[i][1] /= double(width * height);
	}

	for (size_t i = 0; i < width * height; i++)
	{
		double re = in[i][0];
		double im = in[i][1];
		double mag = sqrt(re * re + im * im);
		dataT[i] = float(mag);
		int brightness = static_cast<int>(255.99f * dataT[i]);
		histogramT[brightness] += 1;
		histogramMaxT = std::max(histogramMaxT, histogramT[brightness]);
	}


	// FFTW cleanup
	fftw_cleanup();
	delete[] in;
	delete[] out;

	// Update CDF
	UpdateTransformedCDF();
}

void Image::RemoveArtifactsCameraMan() {
	std::fill(histogramT, histogramT + 256, 0);
	histogramMaxT = 0;

	// create complex 2D image
	fftw_complex* in = new fftw_complex[width * height];
	fftw_complex* out = new fftw_complex[width * height];
	for (size_t i = 0; i < width * height; i++)
	{
		in[i][0] = double(data[i]);
		in[i][1] = 0;
	}

	fftw_plan planFwd = fftw_plan_dft_2d(width, height, in, out, FFTW_FORWARD, FFTW_ESTIMATE);
	fftw_execute(planFwd);
	fftw_destroy_plan(planFwd);


	// swap quadrants
	int h2 = height / 2;
	int w2 = width / 2;
	for (int i = 0; i < h2; i++)
	{
		for (int j = 0; j < w2; j++)
		{
			auto tmp13R = out[i * width + j][0];
			auto tmp13C = out[i * width + j][1];
			out[i * width + j][0] = out[(i + h2) * width + j + w2][0];
			out[i * width + j][1] = out[(i + h2) * width + j + w2][1];
			out[(i + h2) * width + j + w2][0] = tmp13R;
			out[(i + h2) * width + j + w2][1] = tmp13C;

			auto tmp24R = out[(i + h2) * width + j][0];
			auto tmp24C = out[(i + h2) * width + j][1];
			out[(i + h2) * width + j][0] = out[i * width + j + w2][0];
			out[(i + h2) * width + j][1] = out[i * width + j + w2][1];
			out[i * width + j + w2][0] = tmp24R;
			out[i * width + j + w2][1] = tmp24C;
		}
	}

	int stepX = width / 8;
	int stepY = height / 8;
	//for (size_t i = 0; i < height; i += stepY)
	//{
	//	for (size_t j = 0; j < width; j += stepX)
	//	{
	//		out[i * width + j][0] = 0.0;
	//		out[i * width + j][1] = 0.0;
	//	}
	//}
	for (size_t i = 0; i < height; i += stepY)
	{
		for (size_t j = 0; j < width; j += stepX/4)
		{
			out[i * width + j][0] = 0.0;
			out[i * width + j][1] = 0.0;
		}
	}
	for (size_t i = 0; i < height; i += stepY/4)
	{
		for (size_t j = 0; j < width; j += stepX)
		{
			out[i * width + j][0] = 0.0;
			out[i * width + j][1] = 0.0;
		}
	}

	//// copy the magnitude of the result back into the image
	//double clipMag{ double(width) };
	//for (size_t i = 0; i < width * height; i++)
	//{
	//	double re = out[i][0];
	//	double im = out[i][1];
	//	double mag = sqrt(re * re + im * im);
	//	double clipped = mag > clipMag ? 1.0 : mag / clipMag;
	//	dataT[i] = float(sqrt(clipped));

	//	int brightness = static_cast<int>(255.99f * dataT[i]);
	//	histogramT[brightness] += 1;
	//	histogramMaxT = std::max(histogramMaxT, histogramT[brightness]);
	//}

	// swap quadrants
	for (int i = 0; i < h2; i++)
	{
		for (int j = 0; j < w2; j++)
		{
			auto tmp13R = out[i * width + j][0];
			auto tmp13C = out[i * width + j][1];
			out[i * width + j][0] = out[(i + h2) * width + j + w2][0];
			out[i * width + j][1] = out[(i + h2) * width + j + w2][1];
			out[(i + h2) * width + j + w2][0] = tmp13R;
			out[(i + h2) * width + j + w2][1] = tmp13C;

			auto tmp24R = out[(i + h2) * width + j][0];
			auto tmp24C = out[(i + h2) * width + j][1];
			out[(i + h2) * width + j][0] = out[i * width + j + w2][0];
			out[(i + h2) * width + j][1] = out[i * width + j + w2][1];
			out[i * width + j + w2][0] = tmp24R;
			out[i * width + j + w2][1] = tmp24C;
		}
	}

	fftw_plan planBck = fftw_plan_dft_2d(width, height, out, in, FFTW_BACKWARD, FFTW_ESTIMATE);
	fftw_execute(planBck);
	fftw_destroy_plan(planBck);

	// normalize restored image
	for (size_t i = 0; i < width * height; i++)
	{
		in[i][0] /= double(width * height);
		in[i][1] /= double(width * height);
	}

	for (size_t i = 0; i < width * height; i++)
	{
		double re = in[i][0];
		double im = in[i][1];
		double mag = sqrt(re * re + im * im);
		dataT[i] = float(mag);
		int brightness = static_cast<int>(255.99f * dataT[i]);
		histogramT[brightness] += 1;
		histogramMaxT = std::max(histogramMaxT, histogramT[brightness]);
	}


	// FFTW cleanup
	fftw_cleanup();
	delete[] in;
	delete[] out;

	// Update CDF
	UpdateTransformedCDF();
}

void Image::UpdateTransformedCDF() {
	std::fill(distributionT, distributionT + 256, 0);
	distributionT[0] = histogramT[0];
	for (int i = 1; i < 256; i++)
	{
		distributionT[i] = distributionT[i - 1] + histogramT[i];
	}
	// Normalize
	float histogramIntegral = distributionT[255];
	for (int i = 0; i < 256; i++)
	{
		distributionT[i] /= histogramIntegral;
	}
}