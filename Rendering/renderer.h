#pragma once

#include <cstdint>
#include <vector>

#include "../Managers/scene_manager.h"

struct Pixel
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
    float z;
    bool valid;
};

class Renderer
{
public:
    Renderer(int width, int height, SceneManager *scene);

    void clear();
    void render();
    void drawPoint(int x, int y, const Vec3 &color, float z);
    void drawLine(int x0, int y0, int x1, int y1, const Vec3 &color, float z0, float z1);

    const std::vector<Pixel> &getPixels() const;
    int getWidth() const;
    int getHeight() const;

private:
    int width;
    int height;
    std::vector<Pixel> pixels;
    std::vector<float> depthBuffer;
    SceneManager *scene;

    int indexOf(int x, int y) const;
    void writePixel(int x, int y, const Vec3 &color, float z);
};
