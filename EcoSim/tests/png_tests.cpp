// Tests for the dependency-free PNG writer: file structure, checksums, stored
// deflate blocks, filter bytes, and exact pixel round-trip.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "tools/PngWriter.h"

namespace {
std::uint32_t readBigEndian(const std::vector<std::uint8_t>& data,
                            std::size_t offset) {
    return (static_cast<std::uint32_t>(data[offset]) << 24) |
           (static_cast<std::uint32_t>(data[offset + 1]) << 16) |
           (static_cast<std::uint32_t>(data[offset + 2]) << 8) |
           static_cast<std::uint32_t>(data[offset + 3]);
}

std::uint32_t crc32(const std::vector<std::uint8_t>& data) {
    std::uint32_t crc = 0xffffffffu;
    for (std::uint8_t byte : data) {
        crc ^= byte;
        for (int bit = 0; bit < 8; ++bit) {
            const std::uint32_t mask =
                static_cast<std::uint32_t>(-(static_cast<int>(crc & 1u)));
            crc = (crc >> 1) ^ (0xedb88320u & mask);
        }
    }
    return crc ^ 0xffffffffu;
}

std::uint32_t adler32(const std::vector<std::uint8_t>& data) {
    std::uint32_t a = 1;
    std::uint32_t b = 0;
    for (std::uint8_t byte : data) {
        a = (a + byte) % 65521u;
        b = (b + a) % 65521u;
    }
    return (b << 16) | a;
}
}  // namespace

TEST_CASE("PNG writer emits checked stored blocks and exact pixels") {
    constexpr int width = 5;
    constexpr int height = 3;
    std::vector<std::uint8_t> pixels(
        static_cast<std::size_t>(width * height * 3));
    for (std::size_t i = 0; i < pixels.size(); ++i) {
        pixels[i] = static_cast<std::uint8_t>((i * 37u + 11u) & 0xffu);
    }

    const std::string path = "ecosim_png_writer_test.png";
    std::remove(path.c_str());
    REQUIRE(writePng(path, width, height, pixels));

    std::ifstream file(path, std::ios::binary);
    REQUIRE(file.good());
    const std::vector<std::uint8_t> bytes(
        (std::istreambuf_iterator<char>(file)),
        (std::istreambuf_iterator<char>()));
    file.close();
    std::remove(path.c_str());

    const std::array<std::uint8_t, 8> signature = {
        137, 80, 78, 71, 13, 10, 26, 10};
    REQUIRE(bytes.size() > signature.size());
    for (std::size_t i = 0; i < signature.size(); ++i) {
        CHECK(bytes[i] == signature[i]);
    }

    std::size_t offset = signature.size();
    std::vector<std::uint8_t> idat;
    bool sawHeader = false;
    bool sawEnd = false;
    std::uint32_t headerWidth = 0;
    std::uint32_t headerHeight = 0;

    while (offset + 12 <= bytes.size()) {
        const std::uint32_t length = readBigEndian(bytes, offset);
        REQUIRE(offset + 12 + length <= bytes.size());
        const std::string type(
            reinterpret_cast<const char*>(bytes.data() + offset + 4), 4);
        const std::size_t dataOffset = offset + 8;
        const std::vector<std::uint8_t> crcData(
            bytes.begin() + static_cast<std::ptrdiff_t>(offset + 4),
            bytes.begin() +
                static_cast<std::ptrdiff_t>(offset + 8 + length));
        CHECK(crc32(crcData) ==
              readBigEndian(bytes, offset + 8 + length));

        if (type == "IHDR") {
            REQUIRE(length == 13);
            sawHeader = true;
            headerWidth = readBigEndian(bytes, dataOffset);
            headerHeight = readBigEndian(bytes, dataOffset + 4);
            CHECK(bytes[dataOffset + 8] == 8);
            CHECK(bytes[dataOffset + 9] == 2);
            CHECK(bytes[dataOffset + 10] == 0);
            CHECK(bytes[dataOffset + 11] == 0);
            CHECK(bytes[dataOffset + 12] == 0);
        } else if (type == "IDAT") {
            idat.insert(idat.end(),
                        bytes.begin() +
                            static_cast<std::ptrdiff_t>(dataOffset),
                        bytes.begin() +
                            static_cast<std::ptrdiff_t>(dataOffset + length));
        } else if (type == "IEND") {
            CHECK(length == 0);
            sawEnd = true;
            offset += 12 + length;
            break;
        }
        offset += 12 + length;
    }

    CHECK(sawHeader);
    CHECK(sawEnd);
    CHECK(offset == bytes.size());
    CHECK(headerWidth == static_cast<std::uint32_t>(width));
    CHECK(headerHeight == static_cast<std::uint32_t>(height));
    REQUIRE(idat.size() >= 6);
    CHECK(idat[0] == 0x78);
    CHECK(idat[1] == 0x01);

    std::size_t zlibOffset = 2;
    std::vector<std::uint8_t> raw;
    bool finalBlock = false;
    while (!finalBlock) {
        REQUIRE(zlibOffset < idat.size());
        const std::uint8_t header = idat[zlibOffset++];
        finalBlock = (header & 1u) != 0;
        CHECK(((header >> 1) & 3u) == 0);
        REQUIRE(zlibOffset + 4 <= idat.size());
        const std::uint16_t length = static_cast<std::uint16_t>(
            static_cast<std::uint16_t>(idat[zlibOffset]) |
            (static_cast<std::uint16_t>(idat[zlibOffset + 1]) << 8));
        const std::uint16_t inverse = static_cast<std::uint16_t>(
            static_cast<std::uint16_t>(idat[zlibOffset + 2]) |
            (static_cast<std::uint16_t>(idat[zlibOffset + 3]) << 8));
        CHECK(static_cast<std::uint16_t>(length ^ 0xffffu) == inverse);
        zlibOffset += 4;
        REQUIRE(zlibOffset + length <= idat.size());
        raw.insert(raw.end(),
                   idat.begin() +
                       static_cast<std::ptrdiff_t>(zlibOffset),
                   idat.begin() +
                       static_cast<std::ptrdiff_t>(zlibOffset + length));
        zlibOffset += length;
    }

    REQUIRE(zlibOffset + 4 <= idat.size());
    CHECK(readBigEndian(idat, zlibOffset) == adler32(raw));
    CHECK(zlibOffset + 4 == idat.size());

    const std::size_t rowBytes = static_cast<std::size_t>(width) * 3;
    REQUIRE(raw.size() ==
            static_cast<std::size_t>(height) * (rowBytes + 1));
    std::vector<std::uint8_t> rebuilt;
    rebuilt.reserve(static_cast<std::size_t>(width * height * 3));
    std::size_t rawOffset = 0;
    for (int y = 0; y < height; ++y) {
        REQUIRE(raw[rawOffset] == 0);
        ++rawOffset;
        rebuilt.insert(
            rebuilt.end(),
            raw.begin() + static_cast<std::ptrdiff_t>(rawOffset),
            raw.begin() +
                static_cast<std::ptrdiff_t>(rawOffset + rowBytes));
        rawOffset += rowBytes;
    }
    CHECK(rawOffset == raw.size());
    CHECK(rebuilt == pixels);
}
