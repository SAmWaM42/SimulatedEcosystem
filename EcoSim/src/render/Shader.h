// Compiled GLSL program wrapper with small uniform-setting helpers; move-only
// so the OpenGL program has a single owner.
#pragma once

#include <string>

#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

// Owns one linked OpenGL shader program.
class Shader {
public:
    // Loads, compiles, and links the vertex/fragment files; throws with the log.
    Shader(const std::string& vertPath, const std::string& fragPath);
    // Deletes the linked program.
    ~Shader();

    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;
    // Transfers ownership of the GL program.
    Shader(Shader&& other) noexcept;
    Shader& operator=(Shader&& other) noexcept;

    // Binds this program for subsequent draws.
    void use() const;
    void setMat4(const std::string& name, const glm::mat4& value) const;
    void setMat3(const std::string& name, const glm::mat3& value) const;
    void setVec3(const std::string& name, const glm::vec3& value) const;
    void setVec4(const std::string& name, const glm::vec4& value) const;
    void setFloat(const std::string& name, float value) const;
    void setInt(const std::string& name, int value) const;

private:
    unsigned int programId = 0;
};
