#pragma once
#include <cmath>
#include <numbers>
#include <iostream>

class GaussianKernel2D {
public:
	GaussianKernel2D(int kernelSize = 5, float sigma = 1.0f) {
		int halfSize{ kernelSize / 2 };
		size = kernelSize;
		kernel = new float[kernelSize * kernelSize];

		float sum{ 0.0f };

		float r{ 0.0f };
		float s{ 2.0f * sigma * sigma };

		// Generate kernel
		for (int y{ -halfSize }; y <= halfSize; ++y)
		{
			for (int x{ -halfSize }; x <= halfSize; ++x)
			{
				r = sqrtf(x * x + y * y);
				kernel[x + halfSize + (y + halfSize) * kernelSize] = expf(-(r * r) / s);
				sum += kernel[x + halfSize + (y + halfSize) * kernelSize];
			}
		}

		// Normalize kernel
		for (int y{ 0 }; y < kernelSize; ++y)
		{
			for (int x{ 0 }; x < kernelSize; ++x)
			{
				kernel[x + y * kernelSize] /= sum;
				//std::cout << kernel[x + y * kernelSize] << " ";
			}
			//std::cout << "\n";
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
	float* kernel;
	int size;

};

class GaussianKernel1D {
public:
	GaussianKernel1D(int kernelSize = 5, float sigma = 1.0f) {
		int halfSize{ kernelSize / 2 };
		size = kernelSize;
		kernel = new float[kernelSize];

		float sum{ 0.0f };

		float r{ 0.0f };
		float s{ 2.0f * sigma * sigma };

		// Generate kernel
		for (int x{ -halfSize }; x <= halfSize; ++x)
		{
			r = sqrtf(x * x);
			kernel[x + halfSize] = expf(-(r * r) / s);
			sum += kernel[x + halfSize];
		}

		// Normalize kernel

		for (int x{ 0 }; x < kernelSize; ++x)
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
	float* kernel;
	int size;

};