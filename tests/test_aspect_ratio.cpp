#include <cassert>
#include <cmath>
#include <iostream>

struct Rect {
    double x;
    double y;
    double width;
    double height;
};

Rect calculateRenderRect(double containerW, double containerH, double targetRatio) {
    double containerRatio = containerW / containerH;
    if (containerRatio > targetRatio) {
        // Pillarbox
        double renderWidth = containerH * targetRatio;
        double xOffset = (containerW - renderWidth) / 2.0;
        return Rect{xOffset, 0.0, renderWidth, containerH};
    } else {
        // Letterbox
        double renderHeight = containerW / targetRatio;
        double yOffset = (containerH - renderHeight) / 2.0;
        return Rect{0.0, yOffset, containerW, renderHeight};
    }
}

int main() {
    const double phoneRatio = 1080.0 / 2340.0; // ~0.461538

    // Test 1: Wide window (dragged horizontally wider)
    {
        Rect r = calculateRenderRect(800.0, 600.0, phoneRatio);
        double actualRatio = r.width / r.height;
        assert(std::abs(actualRatio - phoneRatio) < 1e-4);
        assert(r.x > 0); // Must be pillarboxed
        assert(r.y == 0.0);
        std::cout << "[PASS] Test 1: Wide container pillarboxing preserves aspect ratio.\n";
    }

    // Test 2: Tall window (dragged vertically taller)
    {
        Rect r = calculateRenderRect(300.0, 900.0, phoneRatio);
        double actualRatio = r.width / r.height;
        assert(std::abs(actualRatio - phoneRatio) < 1e-4);
        assert(r.y > 0); // Must be letterboxed
        assert(r.x == 0.0);
        std::cout << "[PASS] Test 2: Tall container letterboxing preserves aspect ratio.\n";
    }

    // Test 3: Exact ratio container
    {
        Rect r = calculateRenderRect(1080.0, 2340.0, phoneRatio);
        assert(std::abs(r.width - 1080.0) < 1e-4);
        assert(std::abs(r.height - 2340.0) < 1e-4);
        assert(r.x == 0.0 && r.y == 0.0);
        std::cout << "[PASS] Test 3: Exact ratio container fits 100% with zero borders.\n";
    }

    std::cout << "All Aspect Ratio tests PASSED!\n";
    return 0;
}
