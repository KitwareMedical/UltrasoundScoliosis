#include "ClickableUltrasound.h"

#include "MeasurementExplorerUI.h"
#include <iomanip>
#include <QMouseEvent>
#include <iostream>


ClickableUltrasound::ClickableUltrasound(QWidget *parent) 
: QLabel(parent) {
	std::cout << "made clickableultrasond" << std::endl;
}

void ClickableUltrasound::mousePressEvent(QMouseEvent * event) {
	QPoint pos = event->pos();
	//std::cout << pos.x() << ", " << pos.y() << std::endl;
	if (mainWindow) {
		mainWindow->OnUSClicked(pos);
	}
}