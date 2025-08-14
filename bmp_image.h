#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <iostream>
#include <cstring>
#include <filesystem>
#include <fstream>

class BMPImage {
public:
    BMPImage();
    ~BMPImage();
    bool load(const std::string &filename);
    bool save(const std::string &filename) const;
    void display(char blackChar = '#', char whiteChar = ' ') const;
    void drawLine(int x0, int y0, int x1, int y1);
    int getWidth() const;
    int getHeight() const;
    static bool createTestBMP(const std::string &filename, int w = 32, int h = 16);
private:
    int width;
    int height;
    bool topDown;
    int bytesPerPixel;
    std::vector<uint8_t> pixels;
    size_t idx(int x, int y) const;
};
