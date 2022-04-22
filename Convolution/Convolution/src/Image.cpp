#include "Image.hpp"

#define STB_IMAGE_IMPLEMENTATION  
#include <stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION  
#include <stb_image_write.h>


#include <fftw3.h>
#include <vector>
#include <algorithm>
#include <chrono>

using namespace std::chrono;

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
	// Flip vertically
	char* tmp = new char[width * height * 3];
	for (int i = 0; i < height; i++)
	{
		for (int j = 0; j < width; j++)
		{
			char val = dataT[(height - i - 1) * width + j] * 255;
			tmp[(i * width + j) * 3] = val;
			tmp[(i * width + j) * 3 + 1] = val;
			tmp[(i * width + j) * 3 + 2] = val;
		}
	}
	stbi_write_png(fileName, width, height, 3, static_cast<void*>(tmp), width * 3);
	delete[] tmp;
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
	double max = 0.0;
	for (size_t i = 0; i < width * height; i++)
	{
		double re = out[i][0];
		double im = out[i][1];
		double mag = log(1 + sqrt(re * re + im * im));
		if (mag > max) {
			max = mag;
		}

		dataT[i] = mag;

	}

	for (size_t i = 0; i < width * height; i++)
	{
		dataT[i] /= max;
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


	//float* output = new float[width * height / 2];
	//for (size_t i = 0; i < height / 2; i++)
	//{
	//	for (size_t j = 0; j < width / 2; j++)
	//	{
	//		output[i * width + j] = dataT[(i + height / 2) * height + j + width / 2];
	//	}
	//}

	//stbi_write_hdr("../Resources/output.hdr", width / 2, height / 2, 1, output);
	//delete[] output;

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
	fftw_complex* outT = new fftw_complex[width * height];
	std::memcpy(outT, out, width * height * sizeof(fftw_complex));

	std::vector<double> vR;
	vR.reserve(9);
	std::vector<double> vC;
	vC.reserve(9);
	for (int i = 1; i < height - 1; i++)
	{
		for (int j = 1; j < width - 1; j++)
		{
			double sumR = 0.0;
			double sumC = 0.0;
			for (int y = -1; y <= 1; y++) {
				for (int x = -1; x <= 1; x++) {
					if (y == 0 && x == 0) continue;
					vR.push_back(out[(i + y) * width + j + x][0]);
					vC.push_back(out[(i + y) * width + j + x][1]);
					sumR += out[(i + y) * width + j + x][0];
					sumC += out[(i + y) * width + j + x][1];
				}
			}
			//std::sort(vR.begin(), vR.end());
			//std::sort(vC.begin(), vC.end());
			//outT[i * width + j][0] = vR.at(4);
			//outT[i * width + j][1] = vC.at(4);
			outT[i * width + j][0] = sumR / 8.0;
			outT[i * width + j][1] = sumC / 8.0;
			vR.clear();
			vC.clear();
		}
	}
	//for (size_t i = 0; i < height; i += stepY/12)
	//{
	//	for (size_t j = 0; j < width; j += stepX)
	//	{
	//		out[i * width + j][0] = 0.0;
	//		out[i * width + j][1] = 0.0;
	//	}
	//}

	// copy the magnitude of the result back into the image
	//double clipMag{ double(width) };
	//for (size_t i = 0; i < width * height; i++)
	//{
	//	double re = outT[i][0];
	//	double im = outT[i][1];
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
			auto tmp13R = outT[i * width + j][0];
			auto tmp13C = outT[i * width + j][1];
			outT[i * width + j][0] = outT[(i + h2) * width + j + w2][0];
			outT[i * width + j][1] = outT[(i + h2) * width + j + w2][1];
			outT[(i + h2) * width + j + w2][0] = tmp13R;
			outT[(i + h2) * width + j + w2][1] = tmp13C;

			auto tmp24R = outT[(i + h2) * width + j][0];
			auto tmp24C = outT[(i + h2) * width + j][1];
			outT[(i + h2) * width + j][0] = outT[i * width + j + w2][0];
			outT[(i + h2) * width + j][1] = outT[i * width + j + w2][1];
			outT[i * width + j + w2][0] = tmp24R;
			outT[i * width + j + w2][1] = tmp24C;
		}
	}

	fftw_plan planBck = fftw_plan_dft_2d(width, height, outT, in, FFTW_BACKWARD, FFTW_ESTIMATE);
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
	delete[] outT;

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

bool Image::RangeCheck(int x, int y)
{
	if (x < 0 || x > width || y < 0 || y > height) return false;
	return true;
}

void Image::ApplyGaussianFilter(GaussianKernel2D& kernel) {
	int size = kernel.GetSize();
	float* kernelPtr = kernel.KernelPtr();
	int halfSize = size / 2;

	std::fill(histogramT, histogramT + 256, 0);
	histogramMaxT = 0;


	auto start = high_resolution_clock::now();

	for (int i = 0; i < height; ++i)
	{
		for (int j = 0; j < width; ++j)
		{
			float val = 0.0f;
			for (int y = -halfSize; y <= halfSize; ++y)
			{
				for (int x = -halfSize; x <= halfSize; ++x)
				{
					//if (!RangeCheck(j - x, i - y)) continue;
					float k = kernelPtr[x + halfSize + (y + halfSize) * size];

					val += data[std::clamp(i - y, 0, height - 1) * width + std::clamp(j - x, 0, width - 1)] * k;
				}
			}
			dataT[i * width + j] = val;
			int brightness = static_cast<int>(255.99f * dataT[i * width + j]);
			histogramT[brightness] += 1;
			histogramMaxT = std::max(histogramMaxT, histogramT[brightness]);
		}
	}


	auto stop = high_resolution_clock::now();
	auto duration = duration_cast<milliseconds>(stop - start);

	std::cout << "Gaussian filter:             " << duration.count() << " [ms]\n";

	// Update CDF
	UpdateTransformedCDF();
}

void Image::ApplySeparableGaussianFilter(GaussianKernel1D& kernel) {

	float* tmpDataT = new float[width * height];
	memcpy(tmpDataT, data.get(), width * height * sizeof(float));

	int size = kernel.GetSize();
	float* kernelPtr = kernel.KernelPtr();
	int halfSize = size / 2;

	std::fill(histogramT, histogramT + 256, 0);
	histogramMaxT = 0;

	auto start = high_resolution_clock::now();

	// convolve separable along y direction
	for (int i = 0; i < height; ++i)
	{
		for (int j = 0; j < width; ++j)
		{
			float val = 0.0f;
			for (int y = -halfSize; y <= halfSize; ++y)
			{
				float k = kernelPtr[y + halfSize];
				val += data[std::clamp((i - y), 0, height - 1) * width + j] * k;
			}
			tmpDataT[i * width + j] = val;
		}
	}

	// convolve separable along x direction
	for (int i = 0; i < height; ++i)
	{
		for (int j = 0; j < width; ++j)
		{
			float val = 0.0f;
			for (int x = -halfSize; x <= halfSize; ++x)
			{
				float k = kernelPtr[x + halfSize];
				val += tmpDataT[i * width + std::clamp(j - x, 0, width - 1)] * k;
			}
			dataT[i * width + j] = val;
			int brightness = static_cast<int>(255.99f * dataT[i * width + j]);
			histogramT[brightness] += 1;
			histogramMaxT = std::max(histogramMaxT, histogramT[brightness]);
		}
	}

	auto stop = high_resolution_clock::now();
	auto duration = duration_cast<milliseconds>(stop - start);

	std::cout << "Gaussian filter (separable): " << duration.count() << " [ms]\n";

	// Update CDF
	UpdateTransformedCDF();
}


void Image::OriginalImage()
{
	std::fill(histogramT, histogramT + 256, 0);
	histogramMaxT = 0;
	for (int i = 0; i < height; i++)
	{
		for (int j = 0; j < width; j++)
		{
			dataT[i * width + j] = data[i * width + j];
			int brightness = static_cast<int>(255.99f * dataT[i * width + j]);
			histogramT[brightness] += 1;
			histogramMaxT = std::max(histogramMaxT, histogramT[brightness]);
		}
	}
	// Update CDF
	UpdateTransformedCDF();
}