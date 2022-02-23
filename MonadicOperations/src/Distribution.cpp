#include "Distribution.hpp"

Distribution1D::Distribution1D(const int* f, int n) : func(f, f + n), cdf(n + 1) {

	// Compute integral of the function f
	cdf[0] = 0;
	for (int i = 1; i < n + 1; ++i)
	{
		cdf[i] = cdf[i - 1] + func[i - 1] / n;
	}

	// Transform integral into CDF
	funcInt = cdf[n];
	for (int i = 1; i < n + 1; ++i)
	{
		cdf[i] / funcInt;
	}
}

float Distribution1D::GetDistributionValue(int index) {
	return cdf.at(index);
}