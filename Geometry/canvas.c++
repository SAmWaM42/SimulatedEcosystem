#include "base.h"

SoftwareCanvas::SoftwareCanvas(int width, int height)
    : width(width),
      height(height),
      pixels(width * height)
{
}

void SoftwareCanvas::clear(Pixel colour)
{
    for (Pixel& pixel : pixels)
    {
        pixel = colour;
    }
}

void SoftwareCanvas::setPixel(int x, int y, Pixel colour)
{
    if (x < 0 || x >= width ||
        y < 0 || y >= height)
    {
        return;
    }

    pixels[y * width + x] = colour;
}

Pixel SoftwareCanvas::getPixel(int x, int y) const
{
    if (x < 0 || x >= width ||
        y < 0 || y >= height)
    {
        return {0, 0, 0, 0};
    }

    return pixels[y * width + x];
}

int SoftwareCanvas::getWidth() const
{
    return width;
}

int SoftwareCanvas::getHeight() const
{
    return height;
}

const std::vector<Pixel>& SoftwareCanvas::getPixels() const
{
    return pixels;
}