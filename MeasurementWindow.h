#ifndef MEASUREMENTWINDOW_H
#define MEASUREMENTWINDOW_H

#include <QGridLayout>
#include <iostream>

class MeasurementWindow
{
public:
    MeasurementWindow(QGridLayout* my_ui);
    void DrawRectangle();
    void SetRegion(int x, int y);

private:
    QGridLayout* m_ui;
    int region[4];
};

#endif