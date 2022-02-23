#pragma once

// std
#include <vector>

class Distribution1D {
public:

	Distribution1D() = default;

	Distribution1D(const int* f, int n);

	float GetDistributionValue(int index);

private:
	std::vector<float> func; // piecewise-constant function
	std::vector<float> cdf; // Cumulative density function of func
	float funcInt; // Integral of the func
};