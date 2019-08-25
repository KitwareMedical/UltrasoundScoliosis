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
#include <qfiledialog.h>

#include "itkButterworthBandpass1DFilterFunction.h"
#include "itkWindowedSincInterpolateImageFunction.h"
#include "itkResampleImageFilter.h"
#include "itkCurvilinearArraySpecialCoordinatesImage.h"
#include "itkRescaleIntensityImageFilter.h"
#include "itkComposeImageFilter.h"
#include "itkBModeImageFilter.h"
#include "itkCastImageFilter.h"
#include "itkRGBPixel.h"

#include "IntersonArrayDeviceRF.hxx"
#include "ui_MeasurementExplorer.h"
#include "MeasurementWindow.h"

//Forward declaration of Ui::MainWindow;
namespace Ui
{
  class MainWindow;
}

//Declaration of MeasurementUI
class MeasurementExplorerUI : public QMainWindow
{
  Q_OBJECT

public:
  MeasurementExplorerUI( int numberOfThreads, int bufferSize, QWidget *parent = nullptr );
  ~MeasurementExplorerUI();

  void OnUSClicked(QPoint pos);

protected:
  void  closeEvent( QCloseEvent * event );

  void MeasurementExplorerUI::ConnectFiltersForCurvedImage();
  void MeasurementExplorerUI::ConnectFiltersForLinearImage();

protected slots:
  void ConnectProbe();
  void SetPowerSpectrum();
  void SetFrequency();
  void SetDepth();
  void SetDoubler();

  void SetActive0(); void SetActive1(); void SetActive2();

  void Record();

  void FreezeButtonClicked();

  /** Update the images displayed from the probe */
  void UpdateImage();

private:
  /** Layout for the Window */
  Ui::MainWindow *ui;
  QTimer *timer;

  QTimer *processing;
 
  IntersonArrayDeviceRF intersonDevice;
  float mmPerPixel;
  typedef itk::Image<double, 2> ImageType;
  typedef IntersonArrayDeviceRF::RFImageType  RFImageType;

  //BMode filtering
  typedef itk::CastImageFilter<RFImageType, ImageType> CastFilter;
  CastFilter::Pointer m_CastFilter;

  typedef itk::CastImageFilter<ImageType, IntersonArrayDeviceRF::ImageType> CastToIntersonImageTypeFilter;
  CastToIntersonImageTypeFilter::Pointer m_CastToIntersonImageTypeFilter;

  typedef itk::BModeImageFilter< ImageType >  BModeImageFilter;
  BModeImageFilter::Pointer m_BModeFilter;

  typedef itk::ButterworthBandpass1DFilterFunction ButterworthBandpassFilter;
  ButterworthBandpassFilter::Pointer m_BandpassFilter;

  typedef itk::ButterworthBandpass1DFilterFunction ButterworthBandpassFilterRF;
  ButterworthBandpassFilterRF::Pointer m_BandpassFilterRF;

  typedef itk::ComposeImageFilter<ImageType> ComposeImageFilter;
  ComposeImageFilter::Pointer m_ComposeFilter;

  typedef itk::RescaleIntensityImageFilter<ImageType> RescaleFilter;
  RescaleFilter::Pointer m_RescaleFilter;


  typedef itk::CurvilinearArraySpecialCoordinatesImage<double, 2>   CurvedImageType;
  CurvedImageType::Pointer m_CurvedImage;


  typedef itk::ResampleImageFilter < CurvedImageType, ImageType>  ResampleImageFilterType;
  ResampleImageFilterType::Pointer m_ResampleFilter;

  typedef itk::WindowedSincInterpolateImageFunction< CurvedImageType, 2 > WindowedSincInterpolatorType;
  
  WindowedSincInterpolatorType::Pointer m_Interpolator;




  //ui state
  bool is_frozen = false;
  bool is_curved;
  int lastRendered;

  MeasurementWindow *measurement_windows[3];
  int active_measurement_window = 0;

  static void __stdcall ProbeHardButtonCallback( void *instance )
    {
    MeasurementExplorerUI *oui = ( MeasurementExplorerUI* )instance;
    std::cout << "ding!" << std::endl;
    oui->Record();
    };

};

#endif MEASUREMENTEXPLORERUI_H
