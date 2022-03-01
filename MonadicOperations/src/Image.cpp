#include "Image.hpp"

#define STB_IMAGE_IMPLEMENTATION  
#include <stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION  
#include <stb_image_write.h>

Image::Image(const char* fileName) {
    // Load image using stb_image library
    stbi_set_flip_vertically_on_load(true);
    data = std::unique_ptr<Color3[]>(reinterpret_cast<Color3*>(stbi_loadf(
        fileName, &width, &height, &componentsPerPixel, 0)));

    if (!data) {
        std::cerr << "ERROR: Could not load texture image file '" << fileName << "'.\n";
        width = height = 0;
        return;
    }

    dataT = std::make_unique<Color3[]>(width * height);

    // Make grayscale image and create integer histogram
    for (int i = 0; i < height; i++)
    {
        for (int j = 0; j < width; j++)
        {
            float brightness = std::sqrtf(data[i * width + j].Y());
            histogram[static_cast<int>(255.99f * brightness)] += 1;
            data[i * width + j] = Color3(brightness);
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
    stbi_write_hdr(fileName, width, height, componentsPerPixel, reinterpret_cast<float*>(dataT.get()));
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

Color3* Image::DataPtr() {
    return data.get();
}

Color3 Image::Lookup(int x, int y) {
    return data[y * width + x];
}


Color3 Image::LookupT(int x, int y) {
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
            dataT[i * width + j].x = std::sqrtf(data[i * width + j].x);
            dataT[i * width + j].y = std::sqrtf(data[i * width + j].y);
            dataT[i * width + j].z = std::sqrtf(data[i * width + j].z);
            int brightness = static_cast<int>(255.99f * dataT[i * width + j].x);
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
            dataT[i * width + j].x = distribution[static_cast<int>(data[i * width + j].x * 255.99f)];
            dataT[i * width + j].y = distribution[static_cast<int>(data[i * width + j].y * 255.99f)];
            dataT[i * width + j].z = distribution[static_cast<int>(data[i * width + j].z * 255.99f)];
            int brightness = static_cast<int>(255.99f * dataT[i * width + j].x);
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
            dataT[i * width + j].x = data[i * width + j].x > 0.5f ? 1.0f : 0.0f;
            dataT[i * width + j].y = data[i * width + j].y > 0.5f ? 1.0f : 0.0f;
            dataT[i * width + j].z = data[i * width + j].z > 0.5f ? 1.0f : 0.0f;
            int brightness = static_cast<int>(255.99f * dataT[i * width + j].x);
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
            dataT[i * width + j].x = 1.0f - data[i * width + j].x;
            dataT[i * width + j].y = 1.0f - data[i * width + j].y;
            dataT[i * width + j].z = 1.0f - data[i * width + j].z;
            int brightness = static_cast<int>(255.99f * dataT[i * width + j].x);
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
            dataT[i * width + j].x = std::floor(int(data[i * width + j].x * q)) / q;
            dataT[i * width + j].y = std::floor(int(data[i * width + j].y * q)) / q;
            dataT[i * width + j].z = std::floor(int(data[i * width + j].z * q)) / q;
            int brightness = static_cast<int>(255.99f * dataT[i * width + j].x);
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
            if (data[i * width + j].x < 0.5f) {
                dataT[i * width + j].x = 0.5f * std::powf(2.0f * data[i * width + j].x, gamma);
            }
            else {
                dataT[i * width + j].x = 1.0f - 0.5f * std::powf(2.0f - 2.0f * data[i * width + j].x, gamma);
            }

            if (data[i * width + j].y < 0.5f) {
                dataT[i * width + j].y = 0.5f * std::powf(2.0f * data[i * width + j].y, gamma);
            }
            else {
                dataT[i * width + j].y = 1.0f - 0.5f * std::powf(2.0f - 2.0f * data[i * width + j].y, gamma);
            }

            if (data[i * width + j].z < 0.5f) {
                dataT[i * width + j].z = 0.5f * std::powf(2.0f * data[i * width + j].z, gamma);
            }
            else {
                dataT[i * width + j].z = 1.0f - 0.5f * std::powf(2.0f - 2.0f * data[i * width + j].z, gamma);
            }

            int brightness = static_cast<int>(255.99f * dataT[i * width + j].x);
            histogramT[brightness] += 1;
            histogramMaxT = std::max(histogramMaxT, histogramT[brightness]);
        }
    }
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