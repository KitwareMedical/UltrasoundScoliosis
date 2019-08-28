/*

Library:   UltrasoundIntersonApps

Copyright 2010 Kitware Inc. 28 Corporate Drive,
Clifton Park, NY, 12065, USA.

All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http ://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

======================================================================== = */

#ifndef MEASUREMENTWINDOW_H
#define MEASUREMENTWINDOW_H

#include <QGridLayout>
#include <QLabel>
#include <iostream>
#include "itkVectorImage.h"
#include "itkImage.h"
#include "itkStatisticsImageFilter.h"
#include "IntersonArrayDeviceRF.hxx"
#include "itkRegionOfInterestImageFilter.h"
#include "itkRescaleIntensityImageFilter.h"
#include "itkPermuteAxesImageFilter.h"
#include "itkForward1DFFTImageFilter.h"
#include "itkCurvilinearArraySpecialCoordinatesImage.h"
#include "itkCastImageFilter.h"

class MeasurementWindow
{
public:
	typedef itk::Image<double, 2>                                  ImageType;
	typedef itk::VectorImage<double, 2>                            VectorImageType;
	typedef IntersonArrayDeviceRF::RFImageType                     RFImageType;
	typedef itk::CastImageFilter<RFImageType, ImageType>           CastDoubleFilterType;

	typedef itk::Forward1DFFTImageFilter<ImageType>                FFTFilterType;

	typedef itk::StatisticsImageFilter<ImageType>                  StatisticsImageFilterType;
	typedef itk::RegionOfInterestImageFilter<ImageType, ImageType> BmodeROIFilterType;
	typedef itk::RegionOfInterestImageFilter<
		IntersonArrayDeviceRF::RFImageType,
		IntersonArrayDeviceRF::RFImageType>                        RFROIFilterType;


    MeasurementWindow(QGridLayout* my_ui, QLabel* my_graph);
	void UpdateMeasurements(IntersonArrayDeviceRF::RFImageType::Pointer input, ImageType::Pointer bmode);
    void DrawRectangle(VectorImageType::Pointer composite, itk::CurvilinearArraySpecialCoordinatesImage<double, 2>::Pointer curvedImage, int index);
    void SetRegion(int x, int y);

	void DrawMMode(IntersonArrayDeviceRF& intersonDevice, int index, CastDoubleFilterType::Pointer input, itk::PermuteAxesImageFilter<ImageType>::Pointer output, QLabel* mModeLabel);

	bool graphPowerSpectrum = false;

	double max = -999999999;
	double min = 999999999;

private:
    QGridLayout* m_ui;
	QLabel* m_graph;
	QLabel* m_label;

	StatisticsImageFilterType::Pointer m_StatsFilter;
	ImageType::RegionType m_ITKRegion;
	ImageType::RegionType m_nearITKRegion;
	ImageType::RegionType m_farITKRegion;
	BmodeROIFilterType::Pointer m_BmodeROIFilter;
	RFROIFilterType::Pointer m_RFROIFilter;

	CastDoubleFilterType::Pointer m_CastFilter;
	FFTFilterType::Pointer m_FFTFilter;

	bool abort = false;
    int region[4];
	int xsize, ysize;

	RFImageType::Pointer m_mode_buffer;

	

	
};

#endif