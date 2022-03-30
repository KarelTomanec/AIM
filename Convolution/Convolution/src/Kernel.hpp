#pragma once
#include <cmath>
#include <numbers>
#include <iostream>

class GaussianKernel2D {
public:
	GaussianKernel2D(float sigma = 1.0f, int kernelSize = 0) {
		int halfSize;
		if(kernelSize == 0) { 
			halfSize = getKernelHalfWidth(sigma);
		}
		else
		{
			halfSize = kernelSize / 2;
		}
		size = halfSize * 2 + 1;
		kernel = new float[size * size];

		float sum{ 0.0f };

		float r{ 0.0f };
		float s{ 2.0f * sigma * sigma };

		// Generate kernel
		for (int y{ -halfSize }; y <= halfSize; ++y)
		{
			for (int x{ -halfSize }; x <= halfSize; ++x)
			{
				r = sqrtf(x * x + y * y);
				kernel[x + halfSize + (y + halfSize) * size] = expf(-(r * r) / s);
				sum += kernel[x + halfSize + (y + halfSize) * size];
			}
		}

		// Normalize kernel
		for (int y{ 0 }; y < size; ++y)
		{
			for (int x{ 0 }; x < size; ++x)
			{
				kernel[x + y * size] /= sum;
			}
		}
	}

	~GaussianKernel2D() {
		delete[] kernel;
	}

	int GetSize() {
		return size;
	}

	float* KernelPtr() {
		return kernel;
	}

private:

	int getKernelHalfWidth(float sigma)
	{
		return roundf(2.5f * sigma - 0.5f);
	}

	float* kernel;
	int size;

};

class GaussianKernel1D {
public:
	GaussianKernel1D(float sigma = 1.0f, int kernelSize = 0) {
		int halfSize;

		if (kernelSize == 0)
		{
			halfSize = getKernelHalfWidth(sigma);
		}
		else
		{
			halfSize = kernelSize / 2;
		}

		size = halfSize * 2 + 1;
		kernel = new float[size];

		float sum{ 0.0f };

		float r{ 0.0f };
		float s{ 2.0f * sigma * sigma };

		// Generate kernel
		for (int x{ -halfSize }; x <= halfSize; ++x)
		{
			r = x;
			kernel[x + halfSize] = expf(-(r * r) / s);
			sum += kernel[x + halfSize];
		}

		// Normalize kernel

		for (int x{ 0 }; x < size; ++x)
		{
			kernel[x] /= sum;
			std::cout << kernel[x] << " ";
		}
		std::cout << "\n";
	}

	~GaussianKernel1D() {
		delete[] kernel;
	}

	int GetSize() {
		return size;
	}

	float* KernelPtr() {
		return kernel;
	}

private:

	int getKernelHalfWidth(float sigma)
	{
		return roundf(2.5f * sigma - 0.5f);
	}

	float* kernel;
	int size;

};