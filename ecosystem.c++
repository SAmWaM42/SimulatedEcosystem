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
#include <iostream>
#include <vector>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <unique.h>

// ------------------------------------------------------------
// Simple shader for displaying our software framebuffer
// ------------------------------------------------------------

const char *vertex_shader_source = R"(
#version 330 core

layout (location = 0) in vec2 position;
layout (location = 1) in vec2 texCoord;

out vec2 TexCoord;

void main()
{
    gl_Position = vec4(position, 0.0, 1.0);
    TexCoord = texCoord;
}
)";

const char *fragment_shader_source = R"(
#version 330 core

in vec2 TexCoord;

out vec4 FragColor;

uniform sampler2D screenTexture;

void main()
{
    FragColor = texture(screenTexture, TexCoord);
}
)";

// ------------------------------------------------------------
// Compile shader
// ------------------------------------------------------------

GLuint compile_shader(GLenum type, const char *source)
{
    GLuint shader = glCreateShader(type);

    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

    if (!success)
    {
        char info[512];

        glGetShaderInfoLog(
            shader,
            512,
            nullptr,
            info);

        std::cerr << "Shader compilation failed:\n";
        std::cerr << info << '\n';
    }

    return shader;
}

// ------------------------------------------------------------
// Create shader program
// ------------------------------------------------------------

GLuint create_shader_program()
{
    GLuint vertex =
        compile_shader(
            GL_VERTEX_SHADER,
            vertex_shader_source);

    GLuint fragment =
        compile_shader(
            GL_FRAGMENT_SHADER,
            fragment_shader_source);

    GLuint program = glCreateProgram();

    glAttachShader(program, vertex);
    glAttachShader(program, fragment);

    glLinkProgram(program);

    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);

    if (!success)
    {
        char info[512];

        glGetProgramInfoLog(
            program,
            512,
            nullptr,
            info);

        std::cerr << "Shader linking failed:\n";
        std::cerr << info << '\n';
    }

    glDeleteShader(vertex);
    glDeleteShader(fragment);

    return program;
}

// ------------------------------------------------------------
// Convert your framebuffer into RGB data
// ------------------------------------------------------------

void convert_framebuffer(
    Viewport *window,
    std::vector<float> &display_buffer)
{
    for (int y = 0; y < window->height; y++)
    {
        for (int x = 0; x < window->width; x++)
        {
            pixel &p =
                window->frame_buffer[y][x];

            int index =
                (y * window->width + x) * 3;

            if (p.valid)
            {
                display_buffer[index + 0] = p.rgb.x;
                display_buffer[index + 1] = p.rgb.y;
                display_buffer[index + 2] = p.rgb.z;
            }
            else
            {
                display_buffer[index + 0] = 0.05f;
                display_buffer[index + 1] = 0.05f;
                display_buffer[index + 2] = 0.05f;
            }
        }
    }
}

int main()
{
    // ========================================================
    // GLFW
    // ========================================================

    if (!glfwInit())
    {
        std::cerr << "Failed to initialise GLFW\n";
        return -1;
    }

    glfwWindowHint(
        GLFW_CONTEXT_VERSION_MAJOR,
        3);

    glfwWindowHint(
        GLFW_CONTEXT_VERSION_MINOR,
        3);

    glfwWindowHint(
        GLFW_OPENGL_PROFILE,
        GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow *gl_window =
        glfwCreateWindow(
            1280,
            920,
            "Software Renderer",
            nullptr,
            nullptr);

    if (!gl_window)
    {
        std::cerr << "Failed to create GLFW window\n";

        glfwTerminate();

        return -1;
    }

    glfwMakeContextCurrent(gl_window);

    // ========================================================
    // GLAD
    // ========================================================

    if (!gladLoadGLLoader(
            (GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "Failed to initialise GLAD\n";

        glfwDestroyWindow(gl_window);
        glfwTerminate();

        return -1;
    }

    std::cout
        << "OpenGL version: "
        << glGetString(GL_VERSION)
        << '\n';

    // ========================================================
    // Your software framebuffer
    // ========================================================

    Viewport *window = new Viewport();

    window->boundary = {0.0f, 0.0f};

    window->width = 1280;
    window->height = 920;

    // ========================================================
    // Transformer
    // ========================================================

    Transformer transformer;

    // ========================================================
    // Camera
    // ========================================================

    window->cam.transform.position =
        {0.0f, 0.0f, 0.0f};

    window->cam.transform.rotation =
        {0.0f, 0.0f, 0.0f};

    // ========================================================
    // Test object
    // ========================================================

    std::vector<Vector3> polygon =
        {
            {-2.0f, -1.0f, 5.0f},
            {2.0f, -1.0f, 5.0f},
            {2.0f, 1.0f, 5.0f},
            {-2.0f, 1.0f, 5.0f}};

    Transform object_transform;

    object_transform.position =
        {0.0f, 0.0f, 0.0f};

    object_transform.rotation =
        {0.0f, 0.0f, 0.0f};

    object_transform.scale =
        {1.0f, 1.0f, 1.0f};

    shape test_shape(
        polygon,
        object_transform,
        transformer);

    // ========================================================
    // Render using YOUR renderer
    // ========================================================

    test_shape.set_fiiled_shape({0.0f,1.0f,0.0f},
        window,
        transformer);

    // ========================================================
    // Create RGB buffer for OpenGL
    // ========================================================

    std::vector<float> display_buffer(
        window->width *
        window->height *
        3);

    convert_framebuffer(
        window,
        display_buffer);

    // ========================================================
    // OpenGL texture
    // ========================================================

    GLuint texture;

    glGenTextures(
        1,
        &texture);

    glBindTexture(
        GL_TEXTURE_2D,
        texture);

    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_MIN_FILTER,
        GL_NEAREST);

    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_MAG_FILTER,
        GL_NEAREST);

    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_WRAP_S,
        GL_CLAMP_TO_EDGE);

    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_WRAP_T,
        GL_CLAMP_TO_EDGE);

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGB32F,
        window->width,
        window->height,
        0,
        GL_RGB,
        GL_FLOAT,
        display_buffer.data());

    // ========================================================
    // Fullscreen quad
    // ========================================================

    float vertices[] =
        {
            // position      // texture coordinate

            -1.0f, -1.0f, 0.0f, 0.0f,
            1.0f, -1.0f, 1.0f, 0.0f,
            1.0f, 1.0f, 1.0f, 1.0f,

            -1.0f, -1.0f, 0.0f, 0.0f,
            1.0f, 1.0f, 1.0f, 1.0f,
            -1.0f, 1.0f, 0.0f, 1.0f};

    GLuint VAO;
    GLuint VBO;

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);

    glBindBuffer(
        GL_ARRAY_BUFFER,
        VBO);

    glBufferData(
        GL_ARRAY_BUFFER,
        sizeof(vertices),
        vertices,
        GL_STATIC_DRAW);

    // position

    glVertexAttribPointer(
        0,
        2,
        GL_FLOAT,
        GL_FALSE,
        4 * sizeof(float),
        (void *)0);

    glEnableVertexAttribArray(0);

    // texture coordinate

    glVertexAttribPointer(
        1,
        2,
        GL_FLOAT,
        GL_FALSE,
        4 * sizeof(float),
        (void *)(2 * sizeof(float)));

    glEnableVertexAttribArray(1);

    // ========================================================
    // Shader
    // ========================================================

    GLuint shader_program =
        create_shader_program();

    glUseProgram(shader_program);

    glUniform1i(
        glGetUniformLocation(
            shader_program,
            "screenTexture"),
        0);

    // ========================================================
    // Display
    // ========================================================

    while (!glfwWindowShouldClose(gl_window))
    {
        glfwPollEvents();

        // ----------------------------------------------------
        // Clear OpenGL window
        // ----------------------------------------------------

        glClear(GL_COLOR_BUFFER_BIT);

        // ----------------------------------------------------
        // Display our software framebuffer
        // ----------------------------------------------------

        glActiveTexture(GL_TEXTURE0);

        glBindTexture(
            GL_TEXTURE_2D,
            texture);

        glUseProgram(
            shader_program);

        glBindVertexArray(VAO);

        glDrawArrays(
            GL_TRIANGLES,
            0,
            6);

        glfwSwapBuffers(
            gl_window);
    }

    // ========================================================
    // Cleanup
    // ========================================================

    glDeleteTextures(
        1,
        &texture);

    glDeleteVertexArrays(
        1,
        &VAO);

    glDeleteBuffers(
        1,
        &VBO);

    glDeleteProgram(
        shader_program);

    delete window;

    glfwDestroyWindow(
        gl_window);

    glfwTerminate();

    return 0;
}

// Assumption: all projected geometry has z != 0
// and points behind the camera are not passed to the rasterization pipeline.