/*=========================================================================

Library:   UltrasoundIntersonApps

Copyright 2010 Kitware Inc. 28 Corporate Drive,
Clifton Park, NY, 12065, USA.

All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

=========================================================================*/

#pragma once

#ifndef MEASUREMENTEXPLORERUI_H
#define MEASUREMENTEXPLORERUI_H

// Print debug statements
// Using print because I am to inept to setup visual studio
//#define DEBUG_PRINT

#include <QMainWindow>
#include <QTimer>
#include <QTime>
#include <QCheckBox>
#include <qgroupbox.h>
#include <QCloseEvent>


#include "IntersonArrayDeviceRF.hxx"
#include "ui_MeasurementExplorer.h"

//Forward declaration of Ui::MainWindow;
namespace Ui
{
  class MainWindow;
}

  //Declaration of OpticNerveUI
class MeasurementExplorerUI : public QMainWindow
{
  Q_OBJECT

public:
  MeasurementExplorerUI( int numberOfThreads, int bufferSize, QWidget *parent = nullptr );
  ~MeasurementExplorerUI();

  void OnUSClicked(QPoint pos);

protected:
  void  closeEvent( QCloseEvent * event );



protected slots:
    /** Quit the application */
  void ConnectProbe();
  /** Start the application */
  void SetFrequency();
  void SetDepth();
  void SetDoubler();
  void SetPatientID();

  void Record();
  void StopRecording();

  /** Update the images displayed from the probe */
  void UpdateImage();

private:

	QPoint pointForProcessing{ 0, 0 };

  /** Layout for the Window */
  Ui::MainWindow *ui;
  QTimer *timer;

  QTimer *processing;

  std::string patientID;
  

  IntersonArrayDeviceRF intersonDevice;
  float mmPerPixel;


  int lastRendered;


  static void __stdcall ProbeHardButtonCallback( void *instance )
    {
    MeasurementExplorerUI *oui = ( MeasurementExplorerUI* )instance;
    std::cout << "ding!" << std::endl;
    //oui->ToggleEstimation();
    };

};

#endif MEASUREMENTEXPLORERUI_H
