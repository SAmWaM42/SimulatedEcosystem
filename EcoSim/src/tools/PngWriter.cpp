// PNG writer implementation: signature, IHDR/IDAT/IEND chunks, CRC-32, and a
// zlib stream built from uncompressed stored deflate blocks.
#include "tools/PngWriter.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

namespace {
// Appends a value in network byte order.
void appendBigEndian(std::vector<uint8_t>& out, std::uint32_t value) {
    out.push_back(static_cast<uint8_t>((value >> 24) & 0xffu));
    out.push_back(static_cast<uint8_t>((value >> 16) & 0xffu));
    out.push_back(static_cast<uint8_t>((value >> 8) & 0xffu));
    out.push_back(static_cast<uint8_t>(value & 0xffu));
}

// Standard CRC-32 used by PNG chunks.
std::uint32_t crc32(const std::uint8_t* data, std::size_t size) {
    std::uint32_t crc = 0xffffffffu;
    for (std::size_t i = 0; i < size; ++i) {
        crc ^= data[i];
        for (int bit = 0; bit < 8; ++bit) {
            const std::uint32_t mask =
                static_cast<std::uint32_t>(-(static_cast<int>(crc & 1u)));
            crc = (crc >> 1) ^ (0xedb88320u & mask);
        }
    }
    return crc ^ 0xffffffffu;
}

// Adler-32 checksum of the uncompressed zlib payload.
std::uint32_t adler32(const std::vector<uint8_t>& data) {
    std::uint32_t a = 1;
    std::uint32_t b = 0;
    for (std::uint8_t byte : data) {
        a = (a + byte) % 65521u;
        b = (b + a) % 65521u;
    }
    return (b << 16) | a;
}

// Appends one PNG chunk: length, type, data, and CRC over type + data.
void appendChunk(std::vector<uint8_t>& out, const char type[4],
                 const std::vector<uint8_t>& data) {
    appendBigEndian(out, static_cast<std::uint32_t>(data.size()));
    const std::size_t typeOffset = out.size();
    for (int i = 0; i < 4; ++i) {
        out.push_back(static_cast<std::uint8_t>(type[i]));
    }
    out.insert(out.end(), data.begin(), data.end());
    const std::uint32_t crc =
        crc32(out.data() + typeOffset, 4 + data.size());
    appendBigEndian(out, crc);
}

// Wraps raw scanlines in zlib using only stored (uncompressed) deflate blocks.
std::vector<uint8_t> buildZlibStream(const std::vector<uint8_t>& raw) {
    std::vector<uint8_t> zlib;
    zlib.reserve(raw.size() + raw.size() / 65535 + 16);
    zlib.push_back(0x78);  // CMF: deflate, 32K window
    zlib.push_back(0x01);  // FLG: no dictionary, fastest compression

    std::size_t offset = 0;
    do {
        const std::size_t blockSize =
            std::min<std::size_t>(65535, raw.size() - offset);
        const bool finalBlock = offset + blockSize == raw.size();
        zlib.push_back(finalBlock ? 1 : 0);  // BFINAL, BTYPE=00 (stored)
        const std::uint16_t length = static_cast<std::uint16_t>(blockSize);
        const std::uint16_t inverse = static_cast<std::uint16_t>(~length);
        zlib.push_back(static_cast<uint8_t>(length & 0xffu));
        zlib.push_back(static_cast<uint8_t>((length >> 8) & 0xffu));
        zlib.push_back(static_cast<uint8_t>(inverse & 0xffu));
        zlib.push_back(static_cast<uint8_t>((inverse >> 8) & 0xffu));
        zlib.insert(zlib.end(), raw.begin() + static_cast<std::ptrdiff_t>(offset),
                    raw.begin() +
                        static_cast<std::ptrdiff_t>(offset + blockSize));
        offset += blockSize;
    } while (offset < raw.size());

    appendBigEndian(zlib, adler32(raw));
    return zlib;
}
}  // namespace

bool writePng(const std::string& path, int width, int height,
              const std::vector<uint8_t>& rgb) {
    if (width <= 0 || height <= 0) {
        return false;
    }
    const std::size_t rowBytes = static_cast<std::size_t>(width) * 3;
    const std::size_t expectedBytes =
        rowBytes * static_cast<std::size_t>(height);
    if (rgb.size() < expectedBytes) {
        return false;
    }

    // Each row starts with filter byte 0 (None), followed by RGB bytes.
    std::vector<uint8_t> raw;
    raw.reserve(expectedBytes + static_cast<std::size_t>(height));
    for (int y = 0; y < height; ++y) {
        raw.push_back(0);
        const std::size_t offset =
            static_cast<std::size_t>(y) * rowBytes;
        raw.insert(raw.end(),
                   rgb.begin() + static_cast<std::ptrdiff_t>(offset),
                   rgb.begin() +
                       static_cast<std::ptrdiff_t>(offset + rowBytes));
    }

    std::vector<uint8_t> png = {137, 80, 78, 71, 13, 10, 26, 10};

    std::vector<uint8_t> ihdr;
    appendBigEndian(ihdr, static_cast<std::uint32_t>(width));
    appendBigEndian(ihdr, static_cast<std::uint32_t>(height));
    ihdr.push_back(8);  // bit depth
    ihdr.push_back(2);  // colour type: truecolour RGB
    ihdr.push_back(0);  // compression: deflate
    ihdr.push_back(0);  // filter method: adaptive
    ihdr.push_back(0);  // interlace: none
    appendChunk(png, "IHDR", ihdr);

    appendChunk(png, "IDAT", buildZlibStream(raw));
    appendChunk(png, "IEND", std::vector<uint8_t>{});

    std::ofstream file(path, std::ios::binary);
    if (!file) {
        return false;
    }
    file.write(reinterpret_cast<const char*>(png.data()),
               static_cast<std::streamsize>(png.size()));
    return file.good();
}
