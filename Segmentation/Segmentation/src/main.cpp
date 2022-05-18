#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "Image.hpp"

// std
#include <algorithm>

// Load image
bool grayScale = false;

//Image img("../Resources/samoyed.jpeg", grayScale);
//Image img1("../Resources/samoyed_brush.png", grayScale);

//Image img("../Resources/sima.jpg", grayScale);
//Image img1("../Resources/sima_brush.png", grayScale);

Image img("../Resources/llama.jpg", grayScale);
Image img1("../Resources/llama_brush.png", grayScale);

//Image img("../Resources/car.png", grayScale);
//Image img1("../Resources/car_brush.png", grayScale);


std::unique_ptr<Color3[]> pixelBuffer;

void updatePixelBuffer() {
    // Update transformed image
    for (int i = 0; i < img.Height(); i++)
    {
        for (int j = 0; j < img.Width(); j++)
        {
            pixelBuffer[img.Height() * img.Width() + i * 2 * img.Width() + j + img.Width()].x = std::sqrtf(img.LookupT(j, i).x);
            pixelBuffer[img.Height() * img.Width() + i * 2 * img.Width() + j + img.Width()].y = std::sqrtf(img.LookupT(j, i).y);
            pixelBuffer[img.Height() * img.Width() + i * 2 * img.Width() + j + img.Width()].z = std::sqrtf(img.LookupT(j, i).z);
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
        img.SavePNG("../Resources/output.png");
        std::cout << "Transformed image saved as 'output.png'\n";
    }

    if (key == GLFW_KEY_I && action == GLFW_PRESS)
    {

        img.Segmentation(img1);
        updatePixelBuffer();
    }

}

int main() {

    std::cout << "Keyboard controls:" << std::endl;
    std::cout << "[T] Threshold" << std::endl;
    std::cout << "[G] Gamma correction" << std::endl;
    std::cout << "[E] Histogram equalization" << std::endl;
    std::cout << "[N] Negative" << std::endl;
    std::cout << "[Q] Quantization" << std::endl;
    std::cout << "[C] Non-linear contrast" << std::endl;
    std::cout << "[S] Save transformed image" << std::endl;
    std::cout << "[I] Image segmentation" << std::endl;
    
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
            pixelBuffer[img.Height() * img.Width() + i * 2 * img.Width() + j].x = std::sqrtf(img.Lookup(j, i).x);
            pixelBuffer[img.Height() * img.Width() + i * 2 * img.Width() + j].y = std::sqrtf(img.Lookup(j, i).y);
            pixelBuffer[img.Height() * img.Width() + i * 2 * img.Width() + j].z = std::sqrtf(img.Lookup(j, i).z);
            pixelBuffer[img.Height() * img.Width() + i * 2 * img.Width() + j + img.Width()].x = std::sqrtf(img.LookupT(j, i).x);
            pixelBuffer[img.Height() * img.Width() + i * 2 * img.Width() + j + img.Width()].y = std::sqrtf(img.LookupT(j, i).y);
            pixelBuffer[img.Height() * img.Width() + i * 2 * img.Width() + j + img.Width()].z = std::sqrtf(img.LookupT(j, i).z);
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