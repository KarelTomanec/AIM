#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "Image.hpp"

// std
#include <algorithm>

// Load image
Image img("../Resources/Lenna.png");

Color3* pixelBuffer;

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
        for (int j = 0; j < 0.5 * img.Height() * img.DistributionValueT(i) / distributionMax; j++)
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
}

int main() {

    std::cout << "Keyboard controls:" << std::endl;
    std::cout << "[T] Threshold" << std::endl;
    std::cout << "[G] Gamma correction" << std::endl;
    std::cout << "[E] Histogram equalization" << std::endl;
    std::cout << "[N] Negative" << std::endl;
    std::cout << "[Q] Quantization" << std::endl;
    std::cout << "[C] Non-linear contrast" << std::endl;
    
    pixelBuffer = new Vector3[(2 * img.Width()) * (1.5 * img.Height())];

    // Load image and save it
    //img.SaveHDR("../Resources/test.hdr");

    // Initialize the library
    if (!glfwInit()) return -1;

    // Set all the required options for GLFW 
    //glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    //glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

    //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    //glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

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

    // Set this to true so GLEW knows to use a modern approach to retrieving function pointers and extensions 
    //glewExperimental = GL_TRUE;

    //if (GLEW_OK != glewInit())
    //{
    //    std::cout << "Failed to initialize GLEW" << std::endl;
    //    return EXIT_FAILURE;
    //}

    // Define the viewport dimensions 
    //glViewport(0, 0, screenWidth, screenHeight);


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
        for (int j = 0; j < 0.5 * img.Height() * img.DistributionValue(i) / distributionMax; j++)
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
            pixelBuffer[int(img.Height()) * img.Width() + i * 2 * img.Width() + j] = img.Lookup(j, i);
            pixelBuffer[int(img.Height()) * img.Width() + i * 2 * img.Width() + j + img.Width()] = img.LookupT(j, i);
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
        //glDrawPixels(img.Width(), img.Height(), GL_RGB, GL_UNSIGNED_BYTE, pixelBuffer);
        glDrawPixels(2 * img.Width(), 1.5 * img.Height(), GL_RGB, GL_FLOAT, pixelBuffer);

        glfwSwapBuffers(window);
    }

    glfwTerminate();

    delete[] pixelBuffer;

    return EXIT_SUCCESS;
}