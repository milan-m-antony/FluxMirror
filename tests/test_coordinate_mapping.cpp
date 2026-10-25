#include <cassert>
#include <cmath>
#include <iostream>
#include "../src/input/InputController.hpp"

int main() {
    InputController controller;
    const int phoneW = 1080;
    const int phoneH = 2340;
    const QRectF viewport(0, 0, 800, 600); // Wide container

    // In 800x600 with ratio 1080/2340:
    // renderWidth = 600 * (1080/2340) = 276.923
    // xOffset = (800 - 276.923) / 2 = 261.538

    // Center of screen
    QPointF centerMouse(400.0, 300.0);
    QPointF mappedCenter = controller.mapToDevice(centerMouse, viewport, phoneW, phoneH);

    assert(std::abs(mappedCenter.x() - (phoneW / 2.0)) < 2.0);
    assert(std::abs(mappedCenter.y() - (phoneH / 2.0)) < 2.0);
    std::cout << "[PASS] Test 1: Viewport center maps to device center (" << mappedCenter.x() << ", " << mappedCenter.y() << ")\n";

    // Outside left border (clamped to 0)
    QPointF outsideLeft(50.0, 300.0);
    QPointF mappedLeft = controller.mapToDevice(outsideLeft, viewport, phoneW, phoneH);
    assert(mappedLeft.x() <= 0.0);
    std::cout << "[PASS] Test 2: Outside click clamps safely to edge.\n";

    std::cout << "All Coordinate Mapping tests PASSED!\n";
    return 0;
}
