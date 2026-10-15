#include "renderer.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace
{
float clamp(float value, float minValue, float maxValue)
{
    return std::max(minValue, std::min(maxValue, value));
}

bool isInsideViewport(int x, int y, int width, int height)
{
    return x >= 0 && x < width && y >= 0 && y < height;
}

bool barycentricWeights(const Vec2 &p, const Vec2 &a, const Vec2 &b, const Vec2 &c, float &w1, float &w2, float &w3)
{
    const float denom = (b.y - c.y) * (a.x - c.x) + (c.x - b.x) * (a.y - c.y);
    if (std::abs(denom) < 1e-6f)
    {
        return false;
    }

    w1 = ((b.y - c.y) * (p.x - c.x) + (c.x - b.x) * (p.y - c.y)) / denom;
    w2 = ((c.y - a.y) * (p.x - c.x) + (a.x - c.x) * (p.y - c.y)) / denom;
    w3 = 1.0f - w1 - w2;
    return w1 >= 0.0f && w2 >= 0.0f && w3 >= 0.0f;
}

void fillTriangle(const Vec3 &aScreen, const Vec3 &bScreen, const Vec3 &cScreen, const Vec3 &color, Renderer &renderer)
{
    const Vec2 a{aScreen.x, aScreen.y};
    const Vec2 b{bScreen.x, bScreen.y};
    const Vec2 c{cScreen.x, cScreen.y};

    const int minX = std::max(0, static_cast<int>(std::floor(std::min({a.x, b.x, c.x}))));
    const int maxX = std::min(renderer.getWidth() - 1, static_cast<int>(std::ceil(std::max({a.x, b.x, c.x}))));
    const int minY = std::max(0, static_cast<int>(std::floor(std::min({a.y, b.y, c.y}))));
    const int maxY = std::min(renderer.getHeight() - 1, static_cast<int>(std::ceil(std::max({a.y, b.y, c.y}))));

    for (int y = minY; y <= maxY; ++y)
    {
        for (int x = minX; x <= maxX; ++x)
        {
            const Vec2 p{static_cast<float>(x), static_cast<float>(y)};
            float w1 = 0.0f;
            float w2 = 0.0f;
            float w3 = 0.0f;
            if (!barycentricWeights(p, a, b, c, w1, w2, w3))
            {
                continue;
            }

            const float z = w1 * aScreen.z + w2 * bScreen.z + w3 * cScreen.z;
            renderer.drawPoint(x, y, color, z);
        }
    }
}
} // namespace

Renderer::Renderer(int width, int height, SceneManager *scene)
    : width(width),
      height(height),
      pixels(static_cast<size_t>(width) * static_cast<size_t>(height)),
      depthBuffer(static_cast<size_t>(width) * static_cast<size_t>(height), std::numeric_limits<float>::infinity()),
      scene(scene)
{
    clear();
}

int Renderer::indexOf(int x, int y) const
{
    return y * width + x;
}

void Renderer::writePixel(int x, int y, const Vec3 &color, float z)
{
    if (!isInsideViewport(x, y, width, height))
    {
        return;
    }

    const int idx = indexOf(x, y);
    if (z < depthBuffer[idx])
    {
        depthBuffer[idx] = z;
        pixels[idx].r = static_cast<uint8_t>(clamp(color.x * 255.0f, 0.0f, 255.0f));
        pixels[idx].g = static_cast<uint8_t>(clamp(color.y * 255.0f, 0.0f, 255.0f));
        pixels[idx].b = static_cast<uint8_t>(clamp(color.z * 255.0f, 0.0f, 255.0f));
        pixels[idx].z = z;
        pixels[idx].valid = true;
    }
}

void Renderer::clear()
{
    const Pixel skyBlue{135, 206, 235, std::numeric_limits<float>::infinity(), false};
    std::fill(pixels.begin(), pixels.end(), skyBlue);
    std::fill(depthBuffer.begin(), depthBuffer.end(), std::numeric_limits<float>::infinity());
}

void Renderer::drawPoint(int x, int y, const Vec3 &color, float z)
{
    writePixel(x, y, color, z);
}

void Renderer::drawLine(int x0, int y0, int x1, int y1, const Vec3 &color, float z0, float z1)
{
    const int dx = std::abs(x1 - x0);
    const int dy = std::abs(y1 - y0);
    const int sx = x0 < x1 ? 1 : -1;
    const int sy = y0 < y1 ? 1 : -1;
    int err = dx - dy;

    int x = x0;
    int y = y0;

    while (true)
    {
        const float t = dx + dy == 0 ? 0.0f : static_cast<float>(std::abs(x - x0) + std::abs(y - y0)) / static_cast<float>(dx + dy + 1);
        const float z = z0 + (z1 - z0) * t;
        writePixel(x, y, color, z);

        if (x == x1 && y == y1)
        {
            break;
        }

        const int err2 = 2 * err;
        if (err2 > -dy)
        {
            err -= dy;
            x += sx;
        }
        if (err2 < dx)
        {
            err += dx;
            y += sy;
        }
    }
}

void Renderer::render()
{
    clear();

    if (!scene)
    {
        return;
    }

    scene->camera.updateViewMatrix();

    for (const auto &object : scene->getObjects())
    {
        Transformer objectTransform;
        objectTransform.rotate(object.transform.rotation);
        objectTransform.scale(object.transform.scale);
        objectTransform.translate(object.transform.position);
        objectTransform.set_tranformation_matrix();

        for (size_t i = 0; i < object.mesh.indices.size(); i += 3)
        {
            const int aIndex = static_cast<int>(object.mesh.indices[i]);
            const int bIndex = static_cast<int>(object.mesh.indices[i + 1]);
            const int cIndex = static_cast<int>(object.mesh.indices[i + 2]);

            const Vec3 aLocal = object.mesh.vertices[aIndex];
            const Vec3 bLocal = object.mesh.vertices[bIndex];
            const Vec3 cLocal = object.mesh.vertices[cIndex];

            const Vec3 aWorld = objectTransform.transform(aLocal, 1.0f);
            const Vec3 bWorld = objectTransform.transform(bLocal, 1.0f);
            const Vec3 cWorld = objectTransform.transform(cLocal, 1.0f);

            const Vec3 aScreen = scene->camera.projectToScreen(aWorld, width, height);
            const Vec3 bScreen = scene->camera.projectToScreen(bWorld, width, height);
            const Vec3 cScreen = scene->camera.projectToScreen(cWorld, width, height);

            if (aScreen.x < -2000.0f || bScreen.x < -2000.0f || cScreen.x < -2000.0f)
            {
                continue;
            }

            fillTriangle(aScreen, bScreen, cScreen, object.color, *this);

            drawLine(static_cast<int>(std::round(aScreen.x)), static_cast<int>(std::round(aScreen.y)),
                    static_cast<int>(std::round(bScreen.x)), static_cast<int>(std::round(bScreen.y)),
                    Vec3{1.0f, 1.0f, 1.0f}, aScreen.z, bScreen.z);
            drawLine(static_cast<int>(std::round(bScreen.x)), static_cast<int>(std::round(bScreen.y)),
                    static_cast<int>(std::round(cScreen.x)), static_cast<int>(std::round(cScreen.y)),
                    Vec3{1.0f, 1.0f, 1.0f}, bScreen.z, cScreen.z);
            drawLine(static_cast<int>(std::round(cScreen.x)), static_cast<int>(std::round(cScreen.y)),
                    static_cast<int>(std::round(aScreen.x)), static_cast<int>(std::round(aScreen.y)),
                    Vec3{1.0f, 1.0f, 1.0f}, cScreen.z, aScreen.z);
        }
    }
}

const std::vector<Pixel> &Renderer::getPixels() const
{
    return pixels;
}

int Renderer::getWidth() const
{
    return width;
}

int Renderer::getHeight() const
{
    return height;
}
