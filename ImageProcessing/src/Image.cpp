#include "Image.hpp"

#define STB_IMAGE_IMPLEMENTATION  
#include <stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION  
#include <stb_image_write.h>

Image::Image(const char* fileName) {
    stbi_set_flip_vertically_on_load(true);
    data = reinterpret_cast<Color3*>(stbi_loadf(
        fileName, &width, &height, &componentsPerPixel, 0));
    dataT = new Color3[width * height];

    // Make grayscale image
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
    for (int i = 1; i < 256; i++)
    {
        distribution[i] = distribution[i - 1] + histogram[i];
    }

    if (!data) {
        std::cerr << "ERROR: Could not load texture image file '" << fileName << "'.\n";
        width = height = 0;
        return;
    }
}

Image::~Image() {
    delete[] data;
    delete[] dataT;
}


void Image::SaveHDR(const char* fileName) {
    stbi_write_hdr(fileName, width, height, componentsPerPixel, reinterpret_cast<float*>(data));
}

void Image::SavePNG(const char* fileName) {
    stbi_write_png(fileName, width, height, componentsPerPixel, reinterpret_cast<float*>(data), componentsPerPixel * width);
}

int Image::Width() {
    return width;
}

int Image::Height() {
    return height;
}

Color3* Image::DataPtr() {
    return data;
}

Color3 Image::Lookup(int x, int y) {
    return data[y * width + x];
}


Color3 Image::LookupT(int x, int y) {
    return dataT[y * width + x];
}


void Image::GammaCorrection() {
    for (int i = 0; i < height; i++)
    {
        for (int j = 0; j < width; j++)
        {
            dataT[i * width + j].x = std::sqrtf(data[i * width + j].x);
            dataT[i * width + j].y = std::sqrtf(data[i * width + j].y);
            dataT[i * width + j].z = std::sqrtf(data[i * width + j].z);
        }
    }
}

void Image::EqualizeHistogram() {
    float pixelCount = width * height;
    for (int i = 0; i < height; i++)
    {
        for (int j = 0; j < width; j++)
        {
            dataT[i * width + j].x = static_cast<float>(distribution[static_cast<int>(data[i * width + j].x * 255.99f)]) / pixelCount;
            dataT[i * width + j].y = static_cast<float>(distribution[static_cast<int>(data[i * width + j].y * 255.99f)]) / pixelCount;
            dataT[i * width + j].z = static_cast<float>(distribution[static_cast<int>(data[i * width + j].z * 255.99f)]) / pixelCount;
        }
    }
}


void Image::Threshold() {
    for (int i = 0; i < height; i++)
    {
        for (int j = 0; j < width; j++)
        {
            dataT[i * width + j].x = data[i * width + j].x > 0.5f ? 1.0f : 0.0f;
            dataT[i * width + j].y = data[i * width + j].y > 0.5f ? 1.0f : 0.0f;
            dataT[i * width + j].z = data[i * width + j].z > 0.5f ? 1.0f : 0.0f;
        }
    }
}


void Image::Negative() {
    for (int i = 0; i < height; i++)
    {
        for (int j = 0; j < width; j++)
        {
            dataT[i * width + j].x = 1.0f - data[i * width + j].x;
            dataT[i * width + j].y = 1.0f - data[i * width + j].y;
            dataT[i * width + j].z = 1.0f - data[i * width + j].z;
        }
    }
}