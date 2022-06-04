#include "Image.hpp"
#include <algorithm>

#define STB_IMAGE_IMPLEMENTATION  
#include <stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION  
#include <stb_image_write.h>


#include <GridGraph_2D_8C.h>
const float C{ 10.0f };
const Color3 RED{ 1.f, 0.f, 0.f };
const Color3 BLUE{ 0.f, 0.f, 1.f };

const float SQRT2{ 1.41421356237f };

Image::Image(Color3* data, int width, int height) : width(width), height(height)
{

    dataT = std::make_unique<Color3[]>(width * height);

    // Make grayscale image and create integer histogram
    for (int i = 0; i < height; i++)
    {
        for (int j = 0; j < width; j++)
        {
            //float brightness = std::clamp(data[i * width + j], 0.0f, 1.0f);
            //histogram[static_cast<int>(255.99f * brightness)] += 1;
            //data[i * width + j] = brightness;
            
            dataT[i * width + j] = data[i * width + j];
        }
    }
}

Image::Image(const char* fileName, bool grayScale) {
    // Load image using stb_image library
    stbi_set_flip_vertically_on_load(true);
    data = std::unique_ptr<Color3[]>(reinterpret_cast<Color3*>(stbi_loadf(
        fileName, &width, &height, &componentsPerPixel, 3)));

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
            if (grayScale)
            {
                data[i * width + j] = Color3(data[i * width + j].Y());
                dataT[i * width + j] = data[i * width + j];
            }
            else
            {

                dataT[i * width + j] = data[i * width + j];
            }
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
            tmp[i * width + j] = dataT[(height - i - 1) * width + j];
        }
    }
    stbi_write_hdr(fileName, width, height, componentsPerPixel, reinterpret_cast<float*>(tmp.get()));
}

void Image::SavePNG(const char* fileName) {
    // Flip vertically
    char* tmp = new char[width * height * 3];
    for (int i = 0; i < height; i++)
    {
        for (int j = 0; j < width; j++)
        {
            tmp[(i * width + j) * 3] = std::sqrtf(dataT[(height - i - 1) * width + j].x) * 255;
            tmp[(i * width + j) * 3 + 1] = std::sqrtf(dataT[(height - i - 1) * width + j].y) * 255;
            tmp[(i * width + j) * 3 + 2] = std::sqrtf(dataT[(height - i - 1) * width + j].z) * 255;
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

Color3* Image::DataPtr() {
    return data.get();
}

Color3 Image::Lookup(int x, int y) const
{
    return data[y * width + x];
}


Color3 Image::LookupT(int x, int y) const
{
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

Color3* Image::ReconstructImage(Color3* gradientX, Color3* gradientY, Image& img2)
{
    Color3* output = new Color3[width * height];
    std::fill(output, output + width * height, 0.0f);
    //std::copy(dataT.get(), dataT.get() + width * height, output);
    //std::copy(img2.DataPtr(), img2.DataPtr() + width * height, output);
    Color3* tmpImage = new Color3[width * height];

    for (int it = 0; it < 100000; it++)
    {
        std::copy(output, output + width * height, tmpImage);
        for (int i = 0; i < height; i++)
        {
            #pragma omp parallel for
            for (int j = 0; j < width; j++)
            {
                output[i * width + j] = 0.0f;
                output[i * width + j] += (i - 1) >= 0 ? tmpImage[(i - 1) * width + j] : img2.DataPtr()[i * width + j];
                output[i * width + j] += (i + 1) < height ? tmpImage[(i + 1) * width + j] : img2.DataPtr()[i * width + j];
                output[i * width + j] += (j - 1) >= 0 ? tmpImage[i * width + j - 1] : data.get()[i * width + j];
                output[i * width + j] += (j + 1) < width ? tmpImage[i * width + j + 1] : img2.DataPtr()[i * width + j];

                output[i * width + j] += gradientX[i * width + j] + gradientY[i * width + j];
                output[i * width + j] -= (j + 1) < width ? gradientX[i * width + j + 1] : 0;
                output[i * width + j] -= (i + 1) < height ? gradientY[(i + 1) * width + j] : 0;

                output[i * width + j] *= 0.25f;
            }
        }
    }

    delete[] tmpImage;

    for (int i = 0; i < height; i++)
    {
        for (int j = 0; j < width; j++)
        {
            output[i * width + j].x = std::clamp(output[i * width + j].x, 0.0f, 1.0f);
            output[i * width + j].y = std::clamp(output[i * width + j].y, 0.0f, 1.0f);
            output[i * width + j].z = std::clamp(output[i * width + j].z, 0.0f, 1.0f);
        }
    }

    return output;
}

void Image::ImageStitching(Image& img2)
{
    // Create image gradients
    Color3* xGrad1 = new Color3[width * height];
    Color3* yGrad1 = new Color3[width * height];
    Color3* xGrad2 = new Color3[width * height];
    Color3* yGrad2 = new Color3[width * height];

    Color3* img1Data = this->DataPtr();
    Color3* img2Data = img2.DataPtr();

    // x gradients
    for (int i = 0; i < height; i++)
    {
        //xGrad1[i * width + width - 1] = img1Data[i * width + width - 1];
        //xGrad2[i * width + width - 1] = img2Data[i * width + width - 1];
        xGrad1[i * width + width - 1] = Color3(0.0f);
        xGrad2[i * width + width - 1] = Color3(0.0f);
        for (int j = 0; j < width - 1; j++)
        {
            xGrad1[i * width + j] = img1Data[i * width + j + 1] - img1Data[i * width + j];
            xGrad2[i * width + j] = img2Data[i * width + j + 1] - img2Data[i * width + j];
        }
    }

    // y gradients
    for (int j = 0; j < width; j++)
    {
        //yGrad1[(height - 1) * width + j] = img1Data[(height - 1) * width + j];
        //yGrad2[(height - 1) * width + j] = img2Data[(height - 1) * width + j];
        yGrad1[(height - 1) * width + j] = Color3(0.0f);
        yGrad2[(height - 1) * width + j] = Color3(0.0f);
        for (int i = 0; i < height - 1; i++)
        {
            yGrad1[i * width + j] = img1Data[(i + 1) * width + j] - img1Data[i * width + j];
            yGrad2[i * width + j] = img2Data[(i + 1) * width + j] - img2Data[i * width + j];
        }
    }



    //Image imgXDerivatives(xGrad2, width, height);
    //imgXDerivatives.SavePNG("../Resources/xGradients.png");
    //Image imgYDerivatives(yGrad2, width, height);
    //imgYDerivatives.SavePNG("../Resources/yGradients.png");

    Color3* modifiedGradientsX = new Color3[width * height];
    Color3* modifiedGradientsY = new Color3[width * height];

    for (int i = 0; i < height; i++)
    {
        for (int j = 0; j < width / 2; j++)
        {
            modifiedGradientsX[i * width + j] = xGrad1[i * width + j];
            modifiedGradientsY[i * width + j] = yGrad1[i * width + j];
        }
        for (int j = width / 2; j < width; j++)
        {
            modifiedGradientsX[i * width + j] = xGrad2[i * width + j];
            modifiedGradientsY[i * width + j] = yGrad2[i * width + j];
        }
    }

    delete[] xGrad1;
    delete[] xGrad2;
    delete[] yGrad1;
    delete[] yGrad2;

    //Image imgXModifiedGrad(modifiedGradientsX, width, height);
    //imgXModifiedGrad.SavePNG("../Resources/xGradModified.png");
    //Image imgYModifiedGrad(modifiedGradientsY, width, height);
    //imgYModifiedGrad.SavePNG("../Resources/yGradModified.png");

    Color3* finalImage = ReconstructImage(modifiedGradientsX, modifiedGradientsY, img2);

    Image output(finalImage, width, height);
    output.SavePNG("../Resources/output.png");

    dataT = std::unique_ptr<Color3[]>(finalImage);

    std::fill(distributionT, distributionT + 256, 0);
    distributionT[0] = 0;
    for (int i = 0; i < height; i++)
    {
        for (int j = 0; j < width; j++)
        {
            int brightness = static_cast<int>(255.99f * std::sqrtf(dataT[i * width + j].Y()));
            histogramT[brightness] += 1;
            histogramMaxT = std::max(histogramMaxT, histogramT[brightness]);
        }
    }
    // Update CDF
    UpdateTransformedCDF();


    delete[] modifiedGradientsX;
    delete[] modifiedGradientsY;


}


void Image::Segmentation(const Image& img)
{
    typedef GridGraph_2D_8C<float, float, float> Grid;

    Grid* grid = new Grid(width, height);

    //float beta = 0.0f;
    //for (int i = 0; i < height; i++)
    //{
    //    for (int j = 0; j < width; j++)
    //    {
    //        if (j < width - 1)
    //        {
    //            float distSq = SquaredLength(Lookup(j, i) - Lookup(j + 1, i));
    //            beta += distSq;
    //        }

    //        if (i < height - 1)
    //        {
    //            float distSq = SquaredLength(Lookup(j, i) - Lookup(j, i + 1));
    //            beta += distSq;
    //        }
    //    }
    //}

    //beta /= width * height;

    //auto weight = [](float rgbDist, float euclDist, float beta)
    //{
    //    const float lambda1 = 5.f;
    //    const float lambda2 = 2000000.f;
    //    return lambda1 + (lambda2 / euclDist) * std::exp(-rgbDist / ( beta));
    //};

    auto weight = [](const Vector3& rgb1, const Vector3& rgb2)
    {
        const float gamma = 0.8f;
        return 1.0f + std::powf(std::min(rgb1.Average(), rgb2.Average()), gamma);
    };

    for (int i = 0; i < height; i++)
    {
        for (int j = 0; j < width; j++)
        {
            grid->set_terminal_cap(grid->node_id(j, i), img.LookupT(j, i).z == 1 ? C : 0, img.LookupT(j, i).x == 1 ? C : 0);

            if (j < width - 1)
            {
                //const int cap = weight(SquaredLength(Lookup(j, i) - Lookup(j + 1, i)), 1, beta);
                const float cap = weight(Lookup(j, i), Lookup(j + 1, i));
                grid->set_neighbor_cap(grid->node_id(j, i),      1, 0, cap);
                grid->set_neighbor_cap(grid->node_id(j + 1, i), -1, 0, cap);
            }

            if (i < height - 1)
            {
                //const int cap = weight(SquaredLength(Lookup(j, i) - Lookup(j, i + 1)), 1, beta);
                const float cap = weight(Lookup(j, i), Lookup(j, i + 1));

                grid->set_neighbor_cap(grid->node_id(j, i),     0,  1, cap);
                grid->set_neighbor_cap(grid->node_id(j, i + 1), 0, -1, cap);
            }

            if (j < width - 1 && i < height - 1)
            {
                //const int cap = weight(SquaredLength(Lookup(j, i) - Lookup(j + 1, i + 1)), SQRT2, beta);
                const float cap = weight(Lookup(j, i), Lookup(j + 1, i + 1));

                grid->set_neighbor_cap(grid->node_id(j, i), 1, 1, cap);
                grid->set_neighbor_cap(grid->node_id(j + 1, i + 1), -1, -1, cap);
            }
            if (j > 0 && i < height - 1)
            {
                //const int cap = weight(SquaredLength(Lookup(j, i) - Lookup(j - 1, i + 1)), SQRT2, beta);
                const float cap = weight(Lookup(j, i), Lookup(j - 1, i + 1));

                grid->set_neighbor_cap(grid->node_id(j, i), -1, 1, cap);
                grid->set_neighbor_cap(grid->node_id(j - 1, i + 1), 1, -1, cap);
            }
        }
    }

    grid->compute_maxflow();

    for (int i = 0; i < height; i++)
    {
        for (int j = 0; j < width; j++)
        {
            dataT[i * width + j] = data[i * width + j] * (grid->get_segment(grid->node_id(j, i)) ? RED : BLUE);
            //dataT[i * width + j] = data[i * width + j] * (grid->get_segment(grid->node_id(j, i)) ? Vector3(.25, 1.f, .25) : 1);
        }
    }

    delete grid;
}