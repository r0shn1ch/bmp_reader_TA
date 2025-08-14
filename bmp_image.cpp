#include "bmp_image.h"

using std::uint8_t;
using std::uint16_t;
using std::uint32_t;
using std::int32_t;

static uint16_t readU16(const uint8_t* buf) {
    return static_cast<uint16_t>(buf[0]) | (static_cast<uint16_t>(buf[1]) << 8);
}
static uint32_t readU32(const uint8_t* buf) {
    return static_cast<uint32_t>(buf[0]) |
           (static_cast<uint32_t>(buf[1]) << 8) |
           (static_cast<uint32_t>(buf[2]) << 16) |
           (static_cast<uint32_t>(buf[3]) << 24);
}

BMPImage::BMPImage() : width(0), height(0), topDown(false), bytesPerPixel(0) {}
BMPImage::~BMPImage() = default;

size_t BMPImage::idx(int x, int y) const {
    return (static_cast<size_t>(y) * static_cast<size_t>(width) + static_cast<size_t>(x)) * static_cast<size_t>(bytesPerPixel);
}

bool BMPImage::load(const std::string &filename) {
    std::ifstream in(filename, std::ios::binary);
    if (!in) {
        std::cerr << "Error: cannot open file '" << filename << "' for reading.\n";
        return false;
    }
    uint8_t fhBuf[14];
    in.read(reinterpret_cast<char*>(fhBuf), 14);
    if (in.gcount() != 14) {
        std::cerr << "Error: failed to read BMP file header.\n";
        return false;
    }
    uint16_t bfType = readU16(fhBuf);
    uint32_t bfOffBits = readU32(fhBuf + 10);
    if (bfType != 0x4D42) {
        std::cerr << "Error: not a BMP file.\n";
        return false;
    }
    uint8_t dibSizeBuf[4];
    in.read(reinterpret_cast<char*>(dibSizeBuf), 4);
    if (in.gcount() != 4) {
        std::cerr << "Error: failed to read DIB size.\n";
        return false;
    }
    uint32_t dibSize = readU32(dibSizeBuf);
    if (dibSize < 40) {
        std::cerr << "Error: unsupported DIB header size.\n";
        return false;
    }
    std::vector<uint8_t> dibBuf(dibSize);
    std::memcpy(dibBuf.data(), dibSizeBuf, 4);
    in.read(reinterpret_cast<char*>(dibBuf.data() + 4), static_cast<std::streamsize>(dibSize - 4));
    if (!in) {
        std::cerr << "Error: failed to read DIB header.\n";
        return false;
    }
    uint32_t biSize = readU32(dibBuf.data());
    int32_t biWidth = static_cast<int32_t>(readU32(dibBuf.data() + 4));
    int32_t biHeight = static_cast<int32_t>(readU32(dibBuf.data() + 8));
    uint16_t biPlanes = readU16(dibBuf.data() + 12);
    uint16_t biBitCount = readU16(dibBuf.data() + 14);
    uint32_t biCompression = readU32(dibBuf.data() + 16);
    if (biCompression != 0) {
        std::cerr << "Error: compressed BMP not supported.\n";
        return false;
    }
    if (biBitCount != 24 && biBitCount != 32) {
        std::cerr << "Error: only 24-bit and 32-bit BMP supported. Found: " << biBitCount << ".\n";
        return false;
    }
    if (biPlanes != 1) {
        std::cerr << "Error: unsupported biPlanes value.\n";
        return false;
    }
    width = biWidth;
    topDown = false;
    if (biHeight < 0) {
        topDown = true;
        height = -biHeight;
    } else {
        height = biHeight;
    }
    bytesPerPixel = biBitCount / 8;
    if (width <= 0 || height <= 0) {
        std::cerr << "Error: invalid image dimensions.\n";
        return false;
    }
    in.seekg(static_cast<std::streamoff>(bfOffBits), std::ios::beg);
    if (!in) {
        std::cerr << "Error: failed to seek to pixel data.\n";
        return false;
    }
    pixels.assign(static_cast<size_t>(width) * static_cast<size_t>(height) * static_cast<size_t>(bytesPerPixel), 0);
    if (bytesPerPixel == 3) {
        int rowSizeNoPad = width * 3;
        int padding = (4 - (rowSizeNoPad % 4)) % 4;
        std::vector<uint8_t> rowBuf(static_cast<size_t>(rowSizeNoPad + padding));
        for (int row = 0; row < height; ++row) {
            in.read(reinterpret_cast<char*>(rowBuf.data()), static_cast<std::streamsize>(rowBuf.size()));
            if (!in) {
                std::cerr << "Error: failed to read pixel row.\n";
                return false;
            }
            int dstRow = topDown ? row : (height - 1 - row);
            for (int x = 0; x < width; ++x) {
                size_t sIdx = static_cast<size_t>(x) * 3;
                size_t dIdx = idx(x, dstRow);
                pixels[dIdx + 0] = rowBuf[sIdx + 0];
                pixels[dIdx + 1] = rowBuf[sIdx + 1];
                pixels[dIdx + 2] = rowBuf[sIdx + 2];
            }
        }
    } else {
        std::vector<uint8_t> rowBuf(static_cast<size_t>(width) * 4);
        for (int row = 0; row < height; ++row) {
            in.read(reinterpret_cast<char*>(rowBuf.data()), static_cast<std::streamsize>(rowBuf.size()));
            if (!in) {
                std::cerr << "Error: failed to read pixel row.\n";
                return false;
            }
            int dstRow = topDown ? row : (height - 1 - row);
            for (int x = 0; x < width; ++x) {
                size_t sIdx = static_cast<size_t>(x) * 4;
                size_t dIdx = idx(x, dstRow);
                pixels[dIdx + 0] = rowBuf[sIdx + 0];
                pixels[dIdx + 1] = rowBuf[sIdx + 1];
                pixels[dIdx + 2] = rowBuf[sIdx + 2];
                pixels[dIdx + 3] = rowBuf[sIdx + 3];
            }
        }
    }
    return true;
}

void BMPImage::display(char blackChar, char whiteChar) const {
    if (width <= 0 || height <= 0) {
        std::cout << "(no image loaded)\n";
        return;
    }
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            size_t p = idx(x, y);
            uint8_t b = pixels[p + 0];
            uint8_t g = pixels[p + 1];
            uint8_t r = pixels[p + 2];
            bool isBlack = (r <= 10 && g <= 10 && b <= 10);
            if (isBlack) std::cout << blackChar;
            else std::cout << whiteChar;
        }
        std::cout << '\n';
    }
}

void BMPImage::drawLine(int x0, int y0, int x1, int y1) {
    auto setBlack = [&](int x, int y){
        if (x < 0 || x >= width || y < 0 || y >= height) return;
        size_t p = idx(x,y);
        pixels[p + 0] = 0;
        pixels[p + 1] = 0;
        pixels[p + 2] = 0;
        if (bytesPerPixel == 4) pixels[p + 3] = 255;
    };
    int dx = std::abs(x1 - x0);
    int sx = x0 < x1 ? 1 : -1;
    int dy = -std::abs(y1 - y0);
    int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    int x = x0;
    int y = y0;
    while (true) {
        setBlack(x,y);
        if (x == x1 && y == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x += sx; }
        if (e2 <= dx) { err += dx; y += sy; }
    }
}

bool BMPImage::save(const std::string &filename) const {
    if (width <= 0 || height <= 0) {
        std::cerr << "Error: no image to save.\n";
        return false;
    }
    std::ofstream out(filename, std::ios::binary);
    if (!out) {
        std::cerr << "Error: cannot open file for writing.\n";
        return false;
    }
    uint16_t bfType = 0x4D42;
    uint32_t rowNoPad;
    uint32_t padding = 0;
    if (bytesPerPixel == 3) {
        rowNoPad = static_cast<uint32_t>(width) * 3u;
        padding = (4u - (rowNoPad % 4u)) % 4u;
    } else {
        rowNoPad = static_cast<uint32_t>(width) * 4u;
    }
    uint32_t rowSizeWithPad = rowNoPad + padding;
    uint32_t imageSize = rowSizeWithPad * static_cast<uint32_t>(height);
    uint32_t bfOffBits = 14 + 40;
    uint32_t bfSize = bfOffBits + imageSize;
    uint8_t fhOut[14];
    fhOut[0] = static_cast<uint8_t>(bfType & 0xFF);
    fhOut[1] = static_cast<uint8_t>((bfType >> 8) & 0xFF);
    fhOut[2] = static_cast<uint8_t>(bfSize & 0xFF);
    fhOut[3] = static_cast<uint8_t>((bfSize >> 8) & 0xFF);
    fhOut[4] = static_cast<uint8_t>((bfSize >> 16) & 0xFF);
    fhOut[5] = static_cast<uint8_t>((bfSize >> 24) & 0xFF);
    fhOut[6] = 0;
    fhOut[7] = 0;
    fhOut[8] = 0;
    fhOut[9] = 0;
    fhOut[10] = static_cast<uint8_t>(bfOffBits & 0xFF);
    fhOut[11] = static_cast<uint8_t>((bfOffBits >> 8) & 0xFF);
    fhOut[12] = static_cast<uint8_t>((bfOffBits >> 16) & 0xFF);
    fhOut[13] = static_cast<uint8_t>((bfOffBits >> 24) & 0xFF);
    out.write(reinterpret_cast<char*>(fhOut), 14);
    uint8_t ihOut[40];
    std::memset(ihOut, 0, 40);
    uint32_t biSize = 40;
    ihOut[0] = static_cast<uint8_t>(biSize & 0xFF);
    ihOut[1] = static_cast<uint8_t>((biSize >> 8) & 0xFF);
    ihOut[2] = static_cast<uint8_t>((biSize >> 16) & 0xFF);
    ihOut[3] = static_cast<uint8_t>((biSize >> 24) & 0xFF);
    uint32_t w = static_cast<uint32_t>(width);
    ihOut[4] = static_cast<uint8_t>(w & 0xFF);
    ihOut[5] = static_cast<uint8_t>((w >> 8) & 0xFF);
    ihOut[6] = static_cast<uint8_t>((w >> 16) & 0xFF);
    ihOut[7] = static_cast<uint8_t>((w >> 24) & 0xFF);
    int32_t hSigned = topDown ? -height : height;
    uint32_t hUnsigned = static_cast<uint32_t>(hSigned);
    ihOut[8] = static_cast<uint8_t>(hUnsigned & 0xFF);
    ihOut[9] = static_cast<uint8_t>((hUnsigned >> 8) & 0xFF);
    ihOut[10] = static_cast<uint8_t>((hUnsigned >> 16) & 0xFF);
    ihOut[11] = static_cast<uint8_t>((hUnsigned >> 24) & 0xFF);
    ihOut[12] = 1;
    ihOut[13] = 0;
    uint16_t bits = static_cast<uint16_t>(bytesPerPixel * 8);
    ihOut[14] = static_cast<uint8_t>(bits & 0xFF);
    ihOut[15] = static_cast<uint8_t>((bits >> 8) & 0xFF);
    uint32_t comp = 0;
    ihOut[16] = static_cast<uint8_t>(comp & 0xFF);
    ihOut[17] = static_cast<uint8_t>((comp >> 8) & 0xFF);
    ihOut[18] = static_cast<uint8_t>((comp >> 16) & 0xFF);
    ihOut[19] = static_cast<uint8_t>((comp >> 24) & 0xFF);
    uint32_t si = imageSize;
    ihOut[20] = static_cast<uint8_t>(si & 0xFF);
    ihOut[21] = static_cast<uint8_t>((si >> 8) & 0xFF);
    ihOut[22] = static_cast<uint8_t>((si >> 16) & 0xFF);
    ihOut[23] = static_cast<uint8_t>((si >> 24) & 0xFF);
    out.write(reinterpret_cast<char*>(ihOut), 40);
    std::vector<uint8_t> padBuf(padding, 0);
    if (bytesPerPixel == 3) {
        for (int row = 0; row < height; ++row) {
            int fileRow = topDown ? row : (height - 1 - row);
            for (int x = 0; x < width; ++x) {
                size_t p = idx(x, fileRow);
                out.put(static_cast<char>(pixels[p + 0]));
                out.put(static_cast<char>(pixels[p + 1]));
                out.put(static_cast<char>(pixels[p + 2]));
            }
            if (padding) out.write(reinterpret_cast<char*>(padBuf.data()), static_cast<std::streamsize>(padBuf.size()));
        }
    } else {
        for (int row = 0; row < height; ++row) {
            int fileRow = topDown ? row : (height - 1 - row);
            for (int x = 0; x < width; ++x) {
                size_t p = idx(x, fileRow);
                out.put(static_cast<char>(pixels[p + 0]));
                out.put(static_cast<char>(pixels[p + 1]));
                out.put(static_cast<char>(pixels[p + 2]));
                out.put(static_cast<char>(pixels[p + 3]));
            }
        }
    }
    out.flush();
    if (!out) {
        std::cerr << "Error: failed while writing file.\n";
        return false;
    }
    return true;
}

bool BMPImage::createTestBMP(const std::string &filename, int w, int h) {
    if (w <= 0 || h <= 0) return false;
    std::filesystem::path p(filename);
    std::filesystem::create_directories(p.parent_path());
    int bytesPerPixelLocal = 3;
    uint32_t rowNoPad = static_cast<uint32_t>(w) * 3u;
    uint32_t padding = (4u - (rowNoPad % 4u)) % 4u;
    uint32_t rowSizeWithPad = rowNoPad + padding;
    uint32_t imageSize = rowSizeWithPad * static_cast<uint32_t>(h);
    uint32_t bfOffBits = 14 + 40;
    uint32_t bfSize = bfOffBits + imageSize;
    std::ofstream out(filename, std::ios::binary);
    if (!out) return false;
    uint16_t bfType = 0x4D42;
    uint8_t fhOut[14];
    fhOut[0] = static_cast<uint8_t>(bfType & 0xFF);
    fhOut[1] = static_cast<uint8_t>((bfType >> 8) & 0xFF);
    fhOut[2] = static_cast<uint8_t>(bfSize & 0xFF);
    fhOut[3] = static_cast<uint8_t>((bfSize >> 8) & 0xFF);
    fhOut[4] = static_cast<uint8_t>((bfSize >> 16) & 0xFF);
    fhOut[5] = static_cast<uint8_t>((bfSize >> 24) & 0xFF);
    fhOut[6] = 0;
    fhOut[7] = 0;
    fhOut[8] = 0;
    fhOut[9] = 0;
    fhOut[10] = static_cast<uint8_t>(bfOffBits & 0xFF);
    fhOut[11] = static_cast<uint8_t>((bfOffBits >> 8) & 0xFF);
    fhOut[12] = static_cast<uint8_t>((bfOffBits >> 16) & 0xFF);
    fhOut[13] = static_cast<uint8_t>((bfOffBits >> 24) & 0xFF);
    out.write(reinterpret_cast<char*>(fhOut), 14);
    uint8_t ihOut[40];
    std::memset(ihOut, 0, 40);
    uint32_t biSize = 40;
    ihOut[0] = static_cast<uint8_t>(biSize & 0xFF);
    ihOut[1] = static_cast<uint8_t>((biSize >> 8) & 0xFF);
    ihOut[2] = static_cast<uint8_t>((biSize >> 16) & 0xFF);
    ihOut[3] = static_cast<uint8_t>((biSize >> 24) & 0xFF);
    uint32_t W = static_cast<uint32_t>(w);
    ihOut[4] = static_cast<uint8_t>(W & 0xFF);
    ihOut[5] = static_cast<uint8_t>((W >> 8) & 0xFF);
    ihOut[6] = static_cast<uint8_t>((W >> 16) & 0xFF);
    ihOut[7] = static_cast<uint8_t>((W >> 24) & 0xFF);
    int32_t hSigned = h;
    uint32_t H = static_cast<uint32_t>(hSigned);
    ihOut[8] = static_cast<uint8_t>(H & 0xFF);
    ihOut[9] = static_cast<uint8_t>((H >> 8) & 0xFF);
    ihOut[10] = static_cast<uint8_t>((H >> 16) & 0xFF);
    ihOut[11] = static_cast<uint8_t>((H >> 24) & 0xFF);
    ihOut[12] = 1;
    ihOut[13] = 0;
    uint16_t bits = static_cast<uint16_t>(bytesPerPixelLocal * 8);
    ihOut[14] = static_cast<uint8_t>(bits & 0xFF);
    ihOut[15] = static_cast<uint8_t>((bits >> 8) & 0xFF);
    uint32_t comp = 0;
    ihOut[16] = static_cast<uint8_t>(comp & 0xFF);
    ihOut[17] = static_cast<uint8_t>((comp >> 8) & 0xFF);
    ihOut[18] = static_cast<uint8_t>((comp >> 16) & 0xFF);
    ihOut[19] = static_cast<uint8_t>((comp >> 24) & 0xFF);
    uint32_t si = imageSize;
    ihOut[20] = static_cast<uint8_t>(si & 0xFF);
    ihOut[21] = static_cast<uint8_t>((si >> 8) & 0xFF);
    ihOut[22] = static_cast<uint8_t>((si >> 16) & 0xFF);
    ihOut[23] = static_cast<uint8_t>((si >> 24) & 0xFF);
    out.write(reinterpret_cast<char*>(ihOut), 40);
    std::vector<uint8_t> padBuf(padding, 0);
    for (int row = 0; row < h; ++row) {
        for (int x = 0; x < w; ++x) {
            bool black = ((x / 4 + row / 4) % 2) == 0;
            uint8_t r = black ? 0 : 255;
            uint8_t g = r;
            uint8_t b = r;
            out.put(static_cast<char>(b));
            out.put(static_cast<char>(g));
            out.put(static_cast<char>(r));
        }
        if (padding) out.write(reinterpret_cast<char*>(padBuf.data()), static_cast<std::streamsize>(padBuf.size()));
    }
    out.flush();
    return static_cast<bool>(out);
}

int BMPImage::getWidth() const { return width; }
int BMPImage::getHeight() const { return height; }
