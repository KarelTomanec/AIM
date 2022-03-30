#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "Image.hpp"

// std
#include <algorithm>

// Load image
Image img("../Resources/Lenna.png");
//Image img("../Resources/5_cam_original.png");
//Image img("../Resources/5_input_cam.png");
//Image img("../Resources/circle.png");
//Image img("../Resources/square.png");

GaussianKernel2D gaussianKernel2D{3.0f};
GaussianKernel1D gaussianKernel1D{3.0f};

std::unique_ptr<Color3[]> pixelBuffer;

void updatePixelBuffer() {
    // Update transformed image
    for (int i = 0; i < img.Height(); i++)
    {
        for (int j = 0; j < img.Width(); j++)
        {
            pixelBuffer[int(img.Height()) * img.Width() + i * 2 * img.Width() + j + img.Width()] = img.LookupT(j, i);
        }
    }
    // Clear transformed histogram area
    for (int i = 0; i < img.Height() * 0.5; i++)
    {
        for (int j = img.Width(); j < img.Width() * 2; j++)
        {
            pixelBuffer[i * 2 * img.Width() + j] = Vector3(1.0f);
        }
    }

    // Show transformed histogram
    float histogramMaxT = img.HistogramMaxT();
    for (int i = 0; i < 256; i++)
    {
        for (int j = 0; j < 0.5 * img.Height() * img.HistogramValueT(i) / histogramMaxT; j++)
        {
            pixelBuffer[j * img.Width() * 2 + i + img.Width()] = Vector3(0, 0, 1);
        }
    }

    // Show transformed distribution
    float distributionMax = img.Height() * img.Width();
    for (int i = 0; i < 256; i++)
    {
        for (int j = 0; j < 0.5 * img.Height() * img.DistributionValueT(i); j++)
        {
            pixelBuffer[j * img.Width() * 2 + i + img.Width() + int(img.Width() * 0.5)] = Vector3(1, 0, 0);
        }
    }

}


void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_E && action == GLFW_PRESS) {
        img.EqualizeHistogram();
        updatePixelBuffer();
    }
    if (key == GLFW_KEY_G && action == GLFW_PRESS) {
        img.GammaCorrection();
        updatePixelBuffer();
    }
    if (key == GLFW_KEY_T && action == GLFW_PRESS) {
        img.Threshold();
        updatePixelBuffer();
    }
    if (key == GLFW_KEY_N && action == GLFW_PRESS) {
        img.Negative();
        updatePixelBuffer();
    }
    if (key == GLFW_KEY_Q && action == GLFW_PRESS) {
        img.Quantization();
        updatePixelBuffer();
    }
    if (key == GLFW_KEY_C && action == GLFW_PRESS) {
        img.NonLinearContrast();
        updatePixelBuffer();
    }
    if (key == GLFW_KEY_S && action == GLFW_PRESS) {
        img.SaveHDR("../Resources/output.hdr");
        std::cout << "Transformed image saved as 'output.hdr'\n";
    }
    if (key == GLFW_KEY_F && action == GLFW_PRESS) {
        img.FFT();
        updatePixelBuffer();
    }
    if (key == GLFW_KEY_H && action == GLFW_PRESS) {
        img.HighPassFilter();
        updatePixelBuffer();
    }
    if (key == GLFW_KEY_L && action == GLFW_PRESS) {
        img.LowPassFilter();
        updatePixelBuffer();
    }

    if (key == GLFW_KEY_A && action == GLFW_PRESS) {
        img.RemoveArtifactsCameraMan();
        updatePixelBuffer();
    }

    if (key == GLFW_KEY_W && action == GLFW_PRESS) {
        img.ApplyGaussianFilter(gaussianKernel2D);
        updatePixelBuffer();
    }

    if (key == GLFW_KEY_P && action == GLFW_PRESS) {
        img.ApplySeparableGaussianFilter(gaussianKernel1D);
        updatePixelBuffer();
    }

    if (key == GLFW_KEY_O && action == GLFW_PRESS)
    {
        img.OriginalImage();
        updatePixelBuffer();
    }

}

int main() {

    std::cout << "Keyboard controls:" << std::endl;
    std::cout << "[O] Original image" << std::endl;
    std::cout << "[T] Threshold" << std::endl;
    std::cout << "[G] Gamma correction" << std::endl;
    std::cout << "[E] Histogram equalization" << std::endl;
    std::cout << "[N] Negative" << std::endl;
    std::cout << "[Q] Quantization" << std::endl;
    std::cout << "[C] Non-linear contrast" << std::endl;
    std::cout << "[F] Fourier transform" << std::endl;
    std::cout << "[H] High-pass filtering" << std::endl;
    std::cout << "[L] Low-pass filtering" << std::endl;
    //std::cout << "[A] Remove artifacts (cameraman picture)" << std::endl;
    std::cout << "[S] Save transformed image" << std::endl;
    std::cout << "[W] Apply gaussian filter" << std::endl;
    std::cout << "[P] Apply separable gaussian filter" << std::endl;
    
    pixelBuffer = std::make_unique<Color3[]>((2 * img.Width()) * (1.5 * img.Height()));


    // Initialize the library
    if (!glfwInit()) return -1;

    // Create a windowed mode window and its OpenGL context
    GLFWwindow* window = glfwCreateWindow(2 * img.Width(), 1.5 * img.Height(), "AIM: Monadic Operations", nullptr, nullptr);

    if (window == nullptr)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return EXIT_FAILURE;
    }

    int screenWidth, screenHeight;
    glfwGetFramebufferSize(window, &screenWidth, &screenHeight);

    // Make the window's context current
    glfwMakeContextCurrent(window);

    glfwSetKeyCallback(window, key_callback);


    // Clear histogram area
    for (int i = 0; i < img.Height() * 0.5; i++)
    {
        for (int j = 0; j < img.Width() * 2; j++)
        {
            pixelBuffer[i * 2 * img.Width() + j] = Vector3(1.0f);
        }
    }

    // Show histograms
    float histogramMax = img.HistogramMax();
    for (int i = 0; i < 256; i++)
    {
        for (int j = 0; j < 0.5 * img.Height() * img.HistogramValue(i) / histogramMax; j++)
        {
            pixelBuffer[j * img.Width() * 2 + i] = Vector3(0, 0, 1);
            pixelBuffer[j * img.Width() * 2 + i + img.Width()] = Vector3(0, 0, 1);
        }
    }

    // Show distributions
    float distributionMax = img.Height() * img.Width();
    for (int i = 0; i < 256; i++)
    {
        for (int j = 0; j < 0.5 * img.Height() * img.DistributionValue(i); j++)
        {
            pixelBuffer[j * img.Width() * 2 + i + int(img.Width() * 0.5)] = Vector3(1, 0, 0);
            pixelBuffer[j * img.Width() * 2 + i + img.Width() + int(img.Width() * 0.5)] = Vector3(1, 0, 0);
        }
    }

    // Copy images into pixel buffer
    for (int i = 0; i < img.Height(); i++)
    {
        for (int j = 0; j < img.Width(); j++)
        {
            pixelBuffer[int(img.Height()) * img.Width() + i * 2 * img.Width() + j] = Color3(img.Lookup(j, i));
            pixelBuffer[int(img.Height()) * img.Width() + i * 2 * img.Width() + j + img.Width()] = Color3(img.LookupT(j, i));
        }
    }

    // Loop until the user closes the window
    while (!glfwWindowShouldClose(window))
    {
        // Check if any events have been activiated (key pressed, 
        //mouse moved etc.) and call corresponding response functions 
        glfwPollEvents();

        // Render 
        // Clear the colorbuffer 
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Draw OpenGL 
        glDrawPixels(2 * img.Width(), 1.5 * img.Height(), GL_RGB, GL_FLOAT, pixelBuffer.get());

        glfwSwapBuffers(window);
    }

    glfwTerminate();

    return EXIT_SUCCESS;
}