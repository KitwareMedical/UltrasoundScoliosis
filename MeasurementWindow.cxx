#include "MeasurementWindow.h"

MeasurementWindow::MeasurementWindow(QGridLayout* my_ui) {
    m_ui = my_ui;
}
void MeasurementWindow::DrawRectangle() {

}

void MeasurementWindow::SetRegion(int x, int y) {
    int size = 10;
    region[0] = x - size;
    region[1] = x + size;
    region[2] = y - size;
    region[3] = y + size;
}
