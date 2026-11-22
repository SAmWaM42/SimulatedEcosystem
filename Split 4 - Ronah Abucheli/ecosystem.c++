#include <glad/glad.h>
#include <GLFW/glfw3.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include <iostream>
#include <vector>

#include "Rendering/renderer.h"
#include "Managers/scene_manager.h"

namespace
{
const char *vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;
out vec2 vTexCoord;
void main()
{
    gl_Position = vec4(aPos, 0.0, 1.0);
    vTexCoord = aTexCoord;
}
)";

const char *fragmentShaderSource = R"(
#version 330 core
in vec2 vTexCoord;
out vec4 FragColor;
uniform sampler2D uTexture;
void main()
{
    FragColor = texture(uTexture, vTexCoord);
}
)";

GLuint compileShader(GLenum type, const char *source)
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        char buffer[512];
        glGetShaderInfoLog(shader, 512, nullptr, buffer);
        std::cerr << "Shader compile error: " << buffer << '\n';
    }
    return shader;
}

GLuint createProgram()
{
    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    GLint success = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success)
    {
        char buffer[512];
        glGetProgramInfoLog(program, 512, nullptr, buffer);
        std::cerr << "Program link error: " << buffer << '\n';
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return program;
}

void uploadTexture(GLuint textureId, int width, int height, const std::vector<Pixel> &pixels)
{
    std::vector<unsigned char> image(width * height * 3);
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            const int srcIndex = y * width + x;
            const int dstIndex = (y * width + x) * 3;
            image[dstIndex + 0] = pixels[srcIndex].r;
            image[dstIndex + 1] = pixels[srcIndex].g;
            image[dstIndex + 2] = pixels[srcIndex].b;
        }
    }

    glBindTexture(GL_TEXTURE_2D, textureId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image.data());
    glGenerateMipmap(GL_TEXTURE_2D);
}
} // namespace

int main()
{
    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    GLFWwindow *window = glfwCreateWindow(1280, 720, "World Space Camera Renderer", nullptr, nullptr);
    if (!window)
    {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "Failed to initialize GLAD\n";
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    SceneManager scene;
    scene.camera.position = Vec3{0.0f, 0.0f, -6.0f};
    scene.camera.rotation = Vec3{0.0f, 0.0f, 0.0f};
    scene.camera.fovY = 60.0f * static_cast<float>(M_PI) / 180.0f;
    scene.camera.aspectRatio = 1280.0f / 720.0f;
    scene.camera.nearPlane = 0.1f;
    scene.camera.farPlane = 50.0f;

    // Simplified test scenario: a single cube placed in world space so the
    // camera-to-object transform relationship can be evaluated visually.
    scene.addCube(Vec3{0.0f, 0.0f, 0.0f}, 1.6f, Vec3{1.0f, 0.4f, 0.2f});
    scene.addCube(Vec3{2.2f, 0.0f, -1.0f}, 1.2f, Vec3{0.25f, 0.7f, 1.0f});
    scene.addCube(Vec3{-2.0f, 0.5f, -2.0f}, 1.4f, Vec3{0.7f, 0.9f, 0.2f});

    // Optional camera/object test: rotate and translate the test object around
    // the origin to observe world-to-camera transformation behavior.
    scene.objects[0].transform.rotation = Vec3{20.0f, 35.0f, 0.0f};
    scene.objects[0].transform.position = Vec3{0.0f, 0.0f, 0.0f};

    Renderer renderer(1280, 720, &scene);

    GLuint textureId = 0;
    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_2D, textureId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    uploadTexture(textureId, renderer.getWidth(), renderer.getHeight(), renderer.getPixels());

    // Fullscreen quad setup.
    float quadVertices[] = {
        -1.0f, -1.0f, 0.0f, 0.0f,
         1.0f, -1.0f, 1.0f, 0.0f,
         1.0f,  1.0f, 1.0f, 1.0f,
        -1.0f,  1.0f, 0.0f, 1.0f
    };

    GLuint vao = 0;
    GLuint vbo = 0;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), reinterpret_cast<void *>(2 * sizeof(float)));

    GLuint program = createProgram();
    glUseProgram(program);
    glUniform1i(glGetUniformLocation(program, "uTexture"), 0);

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        // Camera movement for the world-space test object.
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        {
            scene.camera.position.z += 0.05f;
        }
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        {
            scene.camera.position.z -= 0.05f;
        }
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        {
            scene.camera.position.x -= 0.05f;
        }
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        {
            scene.camera.position.x += 0.05f;
        }
        if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        {
            scene.camera.rotation.y -= 1.0f;
        }
        if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        {
            scene.camera.rotation.y += 1.0f;
        }
        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
        {
            scene.camera.rotation.y -= 1.0f;
        }
        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
        {
            scene.camera.rotation.y += 1.0f;
        }
        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
        {
            scene.camera.rotation.x -= 1.0f;
        }
        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
        {
            scene.camera.rotation.x += 1.0f;
        }

        // Animate the test cube slowly so the world-to-camera transform is easy to
        // inspect during an interactive test run.
        scene.objects[0].transform.rotation.y += 0.5f;
        scene.objects[0].transform.rotation.x += 0.25f;

        renderer.render();
        uploadTexture(textureId, renderer.getWidth(), renderer.getHeight(), renderer.getPixels());

        glClearColor(0.53f, 0.80f, 0.92f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureId);
        glUseProgram(program);
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

        glfwSwapBuffers(window);
    }

    glDeleteTextures(1, &textureId);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    glDeleteProgram(program);
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
