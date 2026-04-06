#pragma once
#include <vector>
#include <string>
#include <fstream>

struct NkImage {
    int w, h;
    std::vector<unsigned char> data; // RGB

    NkImage(int w_, int h_) : w(w_), h(h_), data(w_*h_*3, 255) {}

    void SetPixel(int x, int y, unsigned char r, unsigned char g, unsigned char b) {
        if(x < 0 || x >= w || y < 0 || y >= h) return;
        int idx = (y * w + x) * 3;
        data[idx] = r;
        data[idx+1] = g;
        data[idx+2] = b;
    }

    void SavePPM(const std::string& filename) {
        std::ofstream f(filename, std::ios::binary);
        // Header P6
        f << "P6\n" << w << " " << h << "\n255\n";
        // Écriture directe des données (binaire)
        f.write(reinterpret_cast<const char*>(data.data()), w * h * 3);
    }

    void DrawLine(int x0, int y0, int x1, int y1, unsigned char r = 0, unsigned char g = 0, unsigned char b = 0) {
        int dx = std::abs(x1 - x0), dy = -std::abs(y1 - y0);
        int sx = x0 < x1 ? 1 : -1;
        int sy = y0 < y1 ? 1 : -1;
        int err = dx + dy;
        while(true) {
            SetPixel(x0, y0, r, g, b);
            if(x0 == x1 && y0 == y1) break;
            int e2 = 2 * err;
            if(e2 >= dy) { err += dy; x0 += sx; }
            if(e2 <= dx) { err += dx; y0 += sy; }
        }
    }
};
