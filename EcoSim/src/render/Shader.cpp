// Shader compilation/linking and uniform helpers. This is the only place that
// knows about OpenGL shader object types.
#include "render/Shader.h"

#include <algorithm>
#include <cstddef>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>

#include <glad/gl.h>
#include <glm/gtc/type_ptr.hpp>

namespace {
// Reads a whole text file or throws with the failing path.
std::string readFile(const std::string& path) {
    std::ifstream file(path);
    if (!file) {
        throw std::runtime_error("Failed to open shader file: " + path);
    }
    std::ostringstream contents;
    contents << file.rdbuf();
    return contents.str();
}

// Compiles one shader stage and returns its GL handle.
GLuint compileShader(GLenum type, const std::string& source,
                     const std::string& path) {
    const GLuint shader = glCreateShader(type);
    const char* sourceText = source.c_str();
    glShaderSource(shader, 1, &sourceText, nullptr);
    glCompileShader(shader);

    GLint success = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (success == GL_FALSE) {
        GLint logLength = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
        std::string log(static_cast<std::size_t>(std::max(1, logLength)), '\0');
        glGetShaderInfoLog(shader, logLength, nullptr, log.data());
        glDeleteShader(shader);
        throw std::runtime_error(path + ":\n" + log);
    }
    return shader;
}
}  // namespace

Shader::Shader(const std::string& vertPath, const std::string& fragPath) {
    const std::string vertexSource = readFile(vertPath);
    const std::string fragmentSource = readFile(fragPath);

    GLuint vertexShader = 0;
    GLuint fragmentShader = 0;
    GLuint program = 0;
    try {
        vertexShader = compileShader(GL_VERTEX_SHADER, vertexSource, vertPath);
        fragmentShader =
            compileShader(GL_FRAGMENT_SHADER, fragmentSource, fragPath);

        program = glCreateProgram();
        glAttachShader(program, vertexShader);
        glAttachShader(program, fragmentShader);
        glLinkProgram(program);

        GLint success = GL_FALSE;
        glGetProgramiv(program, GL_LINK_STATUS, &success);
        if (success == GL_FALSE) {
            GLint logLength = 0;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
            std::string log(static_cast<std::size_t>(std::max(1, logLength)),
                            '\0');
            glGetProgramInfoLog(program, logLength, nullptr, log.data());
            throw std::runtime_error("Shader link failed:\n" + log);
        }

        glDetachShader(program, vertexShader);
        glDetachShader(program, fragmentShader);
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        programId = program;
    } catch (...) {
        if (program != 0) {
            glDeleteProgram(program);
        }
        if (vertexShader != 0) {
            glDeleteShader(vertexShader);
        }
        if (fragmentShader != 0) {
            glDeleteShader(fragmentShader);
        }
        throw;
    }
}

Shader::~Shader() {
    if (programId != 0) {
        glDeleteProgram(programId);
    }
}

Shader::Shader(Shader&& other) noexcept : programId(other.programId) {
    other.programId = 0;
}

Shader& Shader::operator=(Shader&& other) noexcept {
    if (this != &other) {
        if (programId != 0) {
            glDeleteProgram(programId);
        }
        programId = other.programId;
        other.programId = 0;
    }
    return *this;
}

void Shader::use() const {
    glUseProgram(programId);
}

void Shader::setMat4(const std::string& name, const glm::mat4& value) const {
    glUniformMatrix4fv(glGetUniformLocation(programId, name.c_str()), 1,
                       GL_FALSE, glm::value_ptr(value));
}

void Shader::setMat3(const std::string& name, const glm::mat3& value) const {
    glUniformMatrix3fv(glGetUniformLocation(programId, name.c_str()), 1,
                       GL_FALSE, glm::value_ptr(value));
}

void Shader::setVec3(const std::string& name, const glm::vec3& value) const {
    glUniform3fv(glGetUniformLocation(programId, name.c_str()), 1,
                 glm::value_ptr(value));
}

void Shader::setVec4(const std::string& name, const glm::vec4& value) const {
    glUniform4fv(glGetUniformLocation(programId, name.c_str()), 1,
                 glm::value_ptr(value));
}

void Shader::setFloat(const std::string& name, float value) const {
    glUniform1f(glGetUniformLocation(programId, name.c_str()), value);
}

void Shader::setInt(const std::string& name, int value) const {
    glUniform1i(glGetUniformLocation(programId, name.c_str()), value);
}
