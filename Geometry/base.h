#pragma once
struct Point
{
    int x;
    int y;
};
#pragma once

#include <vector>
#include <cstdint>

struct Pixel
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
};

class SoftwareCanvas
{
private:
    int width;
    int height;

    std::vector<Pixel> pixels;

public:
    SoftwareCanvas(int width, int height);

    void clear(Pixel colour);

    void setPixel(int x, int y, Pixel colour);

    Pixel getPixel(int x, int y) const;

    int getWidth() const;
    int getHeight() const;

    const std::vector<Pixel>& getPixels() const;
};