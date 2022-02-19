#pragma once

#include "Vector3.hpp"

class Image {
public:

	Image(const char* fileNName);

	~Image();

	void SaveHDR(const char* fileName);

	void SavePNG(const char* fileName);

	int Width();

	int Height();

	Color3* DataPtr();

	Color3 Lookup(int x, int y);

	Color3 LookupT(int x, int y);

	int HistogramValue(int intensity);

	int HistogramValueT(int intensity);

	int HistogramMax();

	int HistogramMaxT();

	void GammaCorrection();

	void EqualizeHistogram();

	void Threshold();

	void Negative();

private:

	int histogram[256];
	int histogramT[256];
	int distribution[256];
	int componentsPerPixel = 3;
	int histogramMax;
	int histogramMaxT;
	int width;
	int height;
	Color3* data;
	Color3* dataT;

};