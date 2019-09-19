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

#ifndef SCOLIOSISUI_H
#define SCOLIOSISUI_H

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
#include "ui_Scoliosis.h"

#include "ScoliosisQueryNN.h"

#include "itkImageDuplicator.h"

//Forward declaration of Ui::MainWindow;
namespace Ui
{
  class MainWindow;
}

enum ProgramState {
  WaitingForInitialization,
  WaitingToGenerateIdentifier,
  Recording,
  WaitingToRecord,
  WritingToDisk
};

  //Declaration of OpticNerveUI
class ScoliosisUI : public QMainWindow
{
  Q_OBJECT

public:
  ScoliosisUI( int numberOfThreads, int bufferSize, QWidget *parent = nullptr );
  ~ScoliosisUI();

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

  void UpdateConnectionUIs();

private:
  NeuralNetworkSocketConnection nnSocketConnection;

  

  ProgramState state = WaitingForInitialization;
  bool NNConnected = false, USConnected = false, trackerConnected=false;

  /** Layout for the Window */
  Ui::MainWindow *ui;
  QTimer *timer;

  QTimer *processing;
  
  QTimer *updateConnectionUIsTimer;

  IntersonArrayDeviceRF intersonDevice;
  float mmPerPixel;


  int lastRendered;

  std::string patientID;
  int scan_count;
  itk::ImageDuplicator<IntersonArrayDeviceRF::ImageType>::Pointer duplicator 
    = itk::ImageDuplicator<IntersonArrayDeviceRF::ImageType>::New();
  std::vector<IntersonArrayDeviceRF::ImageType::Pointer> savedImages;

  //replace this with non boost component
  //  boost::property_tree::ptree scan_metadata;

  static void __stdcall ProbeHardButtonCallback( void *instance )
  {
    ScoliosisUI *oui = ( ScoliosisUI* )instance;
    std::cout << "ding!" << std::endl;
    //oui->ToggleEstimation();
  };

};

#endif SCOLIOSISUI_H
