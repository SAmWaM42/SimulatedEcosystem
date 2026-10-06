// main c++ file for ecosystem loop
#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#define min_Width 1280
#define min_Height 920
#include <unique.h>
/*
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
*/
int main()
{
    std::cout << "[DEBUG] Program started\n";

    std::cout << "[DEBUG] Creating Viewport...\n";

    Viewport *window = new Viewport();

    std::cout << "[DEBUG] Viewport created successfully\n";

    window->boundary = {0.0f, 0.0f};
    window->width = max_width;
    window->height = max_height;

    std::cout << "[DEBUG] Viewport configured\n";
    std::cout << "        width  = " << window->width << '\n';
    std::cout << "        height = " << window->height << '\n';

    std::vector<Vector3> polygon = {
        {200.0f, 150.0f, 1.0f},
        {500.0f, 150.0f, 1.0f},
        {600.0f, 400.0f, 1.0f},
        {350.0f, 550.0f, 1.0f},
        {150.0f, 400.0f, 1.0f}};

    std::cout << "[DEBUG] Polygon created\n";
    std::cout << "        vertices = " << polygon.size() << '\n';

    shape test_shape;

    std::cout << "[DEBUG] Shape created\n";

    std::cout << "[DEBUG] Calling set_unfiiled_shape...\n";

    test_shape.set_unfiiled_shape(polygon, window);

    std::cout << "[DEBUG] set_unfiiled_shape returned\n";

    std::cout << "[DEBUG] Vertices stored: "
              << test_shape.endpoints.size() << '\n';

    std::cout << "[DEBUG] Edges generated: "
              << test_shape.edges.size() << '\n';

    for (size_t i = 0; i < test_shape.edges.size(); ++i)
    {
        std::cout << "[DEBUG] Inspecting edge " << i << '\n';

        std::cout << "        endpoint count = "
                  << test_shape.edges[i].endpoints.size() << '\n';

        for (size_t j = 0;
             j < test_shape.edges[i].endpoints.size();
             ++j)
        {
            const Vector3 &p = test_shape.edges[i].endpoints[j];

            std::cout << "        point " << j
                      << " = ("
                      << p.x << ", "
                      << p.y << ", "
                      << p.z << ")\n";
        }
    }

    std::cout << "[DEBUG] Test completed successfully\n";
    test_shape.print_framebuffer(window);
    delete window;

    std::cout << "[DEBUG] Viewport destroyed\n";

    return 0;
}