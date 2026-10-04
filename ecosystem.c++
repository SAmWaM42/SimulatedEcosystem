// main c++ file for ecosystem loop
#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#define min_Width 1280
#define min_Height 920
void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
    glViewport(0, 0, width, height);
}
void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

int main()
{ // setup

    glfwInit(); // initializes opengl before anything needs to be freed
    // setting confings
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    // creating the window obj
    GLFWwindow *window = glfwCreateWindow(min_Width, min_Height, "test", NULL, NULL);
    if (!window)
    {std::cout << "failed to create window\n";
        glfwTerminate();
        return -1;};
    // setting the current context to the window
    glfwMakeContextCurrent(window);
    // setting up glad to manage function pointers for openGl
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    { std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;}
    // setting up the view port
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    while (!glfwWindowShouldClose(window))
    {
        // read inputs here (before rendering code)
        processInput(window);
        // rending logic should happen here

        glClearColor(0.2f, 0.7f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        // handles updates
        glfwPollEvents();
        glfwSwapBuffers(window);
    }

    glfwTerminate();
    return 0;
}
