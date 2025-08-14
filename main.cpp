#include "bmp_image.h"

namespace fs = std::filesystem;

static std::string trim_cr(const std::string &s) {
    if (s.empty()) return s;
    size_t end = s.size();
    while (end > 0 && (s[end-1] == '\r' || s[end-1] == '\n')) --end;
    return s.substr(0,end);
}

int main() {
    fs::path folder = "test_images";
    fs::create_directories(folder);
    std::cout << "Choose input image:\n";
    std::cout << "1) Use generated test image (" << (folder / "generated.bmp").string() << ")\n";
    std::cout << "2) Specify full path to your BMP file\n";
    std::cout << "Enter choice (1 or 2, q to quit): ";
    std::string choice;
    if (!std::getline(std::cin, choice)) return 0;
    choice = trim_cr(choice);
    if (choice.empty()) return 0;
    if (choice == "q" || choice == "Q") return 0;
    std::string inputPath;
    if (choice == "1") {
        fs::path gen = folder / "generated.bmp";
        bool ok = BMPImage::createTestBMP(gen.string(), 64, 32);
        if (!ok) {
            std::cerr << "Failed to create test image.\n";
            return 0;
        }
        inputPath = gen.string();
    } else if (choice == "2") {
        std::cout << "Enter full path to BMP file: ";
        std::string p;
        if (!std::getline(std::cin, p)) return 0;
        p = trim_cr(p);
        if (p.empty()) return 0;
        fs::path src(p);
        if (!fs::exists(src)) {
            std::cerr << "File does not exist: " << p << "\n";
            return 0;
        }
        fs::path dst = folder / "input_user.bmp";
        std::error_code ec;
        fs::copy_file(src, dst, fs::copy_options::overwrite_existing, ec);
        if (ec) {
            inputPath = p;
        } else {
            inputPath = dst.string();
        }
    } else {
        std::cout << "Unknown choice\n";
        return 0;
    }
    BMPImage img;
    if (!img.load(inputPath)) {
        std::cerr << "Failed to load BMP file: " << inputPath << "\n";
        return 0;
    }
    std::cout << "\nOriginal image (" << img.getWidth() << "x" << img.getHeight() << "):\n";
    img.display('#', ' ');
    int w = img.getWidth();
    int h = img.getHeight();
    img.drawLine(0, 0, w - 1, h - 1);
    img.drawLine(w - 1, 0, 0, h - 1);
    std::cout << "\nImage after drawing X:\n";
    img.display('#', ' ');
    std::cout << "\nEnter output BMP file name (will be created inside " << folder.string() << "): ";
    std::string outName;
    if (!std::getline(std::cin, outName)) return 0;
    outName = trim_cr(outName);
    if (outName.empty()) {
        std::cerr << "Empty output name, aborting.\n";
        return 0;
    }
    fs::path outPath = folder / outName;
    if (!img.save(outPath.string())) {
        std::cerr << "Failed to save output file.\n";
        return 0;
    }
    std::cout << "Saved modified image to '" << outPath.string() << "'.\n";
    return 0;
}
