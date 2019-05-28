#ifndef ClickableUltrasound_H
#define ClickableUltrasound_H

#include <QWidget>
#include <QLabel>

class MeasurementExplorerUI;

class ClickableUltrasound : public QLabel {
	Q_OBJECT

public:
	ClickableUltrasound(QWidget *parent = 0);

	MeasurementExplorerUI *mainWindow;

	void (*callback)(QPoint) = 0;

protected:
	void mousePressEvent(QMouseEvent *event) override;

    
};

#endif