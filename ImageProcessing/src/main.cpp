#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "Image.hpp"

// std
#include <algorithm>

// Load image
Image img("../Resources/Lenna.png");

Color3* pixelBuffer;

void updatePixelBuffer() {
    for (int i = 0; i < img.Height(); i++)
    {
        for (int j = 0; j < img.Width(); j++)
        {
            pixelBuffer[i * 2 * img.Width() + j + img.Width()] = img.LookupT(j, i);
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
}

int main() {
    
    pixelBuffer = new Vector3[(2 * img.Width()) * img.Height()];

    // Load image and save it
    img.SaveHDR("../Resources/test.hdr");

    // Initialize the library
    if (!glfwInit())
        return -1;

    // Set all the required options for GLFW 
    //glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    //glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

    //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    //glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

    // Create a windowed mode window and its OpenGL context
    GLFWwindow* window = glfwCreateWindow(2 * img.Width(), img.Height(), "AIM: Monadic Operations", nullptr, nullptr);

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

    // Copy images into pixel buffer
    for (int i = 0; i < img.Height(); i++)
    {
        for (int j = 0; j < img.Width(); j++)
        {
            pixelBuffer[i * 2 * img.Width() + j] = img.Lookup(j, i);
            pixelBuffer[i * 2 * img.Width() + j + img.Width()] = img.LookupT(j, i);
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
        glDrawPixels(2 * img.Width(), img.Height(), GL_RGB, GL_FLOAT, pixelBuffer);

        glfwSwapBuffers(window);
    }

    glfwTerminate();

    delete[] pixelBuffer;

    return EXIT_SUCCESS;
}