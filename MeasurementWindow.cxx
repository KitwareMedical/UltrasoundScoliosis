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

#include "MeasurementWindow.h"
#include "ITKQtHelpers.hxx"
#include "itkSpectra1DSupportWindowImageFilter.h"
#include "itkMetaDataObject.h"
#include "itkCurvilinearArraySpecialCoordinatesImage.h"

template <class ImageType>
typename ImageType::Pointer CreateImage(int wx, int wy)
{
	ImageType::Pointer image = ImageType::New();

	ImageType::IndexType imageIndex;
	imageIndex.Fill(0);

	ImageType::SizeType imageSize;
	imageSize[0] = wx;
	imageSize[1] = wy;

	ImageType::RegionType imageRegion;
	imageRegion.SetIndex(imageIndex);
	imageRegion.SetSize(imageSize);

	image->SetRegions(imageRegion);
	image->Allocate();

	return image;
}

MeasurementWindow::MeasurementWindow(QGridLayout* my_ui, QLabel* my_graph) {
	m_graph = my_graph;
	m_ui = my_ui;
	m_StatsFilter = StatisticsImageFilterType::New();
	m_BmodeROIFilter = BmodeROIFilterType::New();
	m_RFROIFilter = RFROIFilterType::New();

	m_CastFilter = CastDoubleFilterType::New();
	m_FFTFilter = FFTFilterType::New();
	
	xsize = 128, ysize = 17;

	((QLabel *)(this->m_ui->itemAtPosition(0, 1)->widget()))->setText("---");
	((QLabel *)(this->m_ui->itemAtPosition(1, 1)->widget()))->setText("---");
	((QLabel *)(this->m_ui->itemAtPosition(2, 1)->widget()))->setText("---");
	((QLabel *)(this->m_ui->itemAtPosition(3, 1)->widget()))->setText("---");

}

	

void MeasurementWindow::UpdateMeasurements(IntersonArrayDeviceRF::RFImageType::Pointer rf, ImageType::Pointer bmode) {
    //draw graph
	
	int vres = 70;
	int image_depth = region[1] - region[0];
	int image_width = region[3] - region[2];
	ImageType::Pointer graph = CreateImage<ImageType>(image_depth, vres);

	graph->FillBuffer(127);

	if (graphPowerSpectrum) {

		m_RFROIFilter->SetInput(rf);
		m_CastFilter->SetInput(m_RFROIFilter->GetOutput());

		m_CastFilter->Update();
		/*
		std::cout << "casted image" << std::endl;
		m_CastFilter->GetOutput()->Print(std::cout);
		std::cout << "that was it" << std::endl;
		*/
		m_FFTFilter->SetInput(m_CastFilter->GetOutput());

		m_FFTFilter->Update();
		
		//std::cout << "Spectra:" << std::endl;
		//m_SpectraFilter->GetOutput()->Print(std::cout);

		//m_FFTFilter->GetOutput()->Print(std::cout);
		//std::cout << "noneth loop" << std::endl;
		for (int l = 0; l < image_depth; l++) {
			double sum_power = 0;
			for (int j = 0; j < image_width; j++) {


				auto the_value = m_FFTFilter->GetOutput()->GetPixel({ l / 2, j });
				auto mag = std::abs(the_value) * std::abs(the_value);
				sum_power += mag;
			}
			sum_power = -std::log(sum_power);
			max = std::max(sum_power, max);
			min = std::min(sum_power, min);
		}
		//std::cout << "first loop" << std::endl;
		for (int l = 0; l < image_depth; l++) {
			double sum_power = 0;
			for (int j = 0; j < image_width; j++) {


				auto the_value = m_FFTFilter->GetOutput()->GetPixel({ l / 2, j });
				auto mag = std::abs(the_value) * std::abs(the_value);
				sum_power += mag;
			}
			sum_power = -std::log(sum_power);

			double the_value = vres * (sum_power - min) / (max - min);
			int val = (int)the_value;
			for (int w = 0; w < vres; w++) {
				graph->SetPixel({ l, w }, 255 * (w > val));
			}
		}
		//std::cout << "second loop" << std::endl;


		double sum_power = 0;
		for (int l = image_depth / 4; l < 3 * image_depth / 4; l++) {

			for (int j = 0; j < image_width; j++) {


				auto the_value = m_FFTFilter->GetOutput()->GetPixel({ l / 2, j });
				auto mag = std::abs(the_value) * std::abs(the_value);
				sum_power += mag;
			}
		}

		((QLabel *)(this->m_ui->itemAtPosition(0, 1)->widget()))->setText(std::to_string(sum_power / (image_depth * image_width * 4000)).c_str());

		m_RFROIFilter->SetRegionOfInterest(m_nearITKRegion);

		m_FFTFilter->Update();

		double near_power = 0;
		for (int l = image_depth / 8; l < 3 * image_depth / 8; l++) {

			for (int j = 0; j < image_width; j++) {


				auto the_value = m_FFTFilter->GetOutput()->GetPixel({ l / 2, j });
				auto mag = std::abs(the_value) * std::abs(the_value);
				near_power += mag;
			}
		}
		m_RFROIFilter->SetRegionOfInterest(m_farITKRegion);

		m_FFTFilter->Update();

		double far_power = 0;
		for (int l = image_depth / 8; l < 3 * image_depth / 8; l++) {

			for (int j = 0; j < image_width; j++) {


				auto the_value = m_FFTFilter->GetOutput()->GetPixel({ l / 2, j });
				auto mag = std::abs(the_value) * std::abs(the_value);
				far_power += mag;
			}
		}

		((QLabel *)(this->m_ui->itemAtPosition(3, 1)->widget()))->setText(std::to_string(far_power / near_power).c_str());
		m_RFROIFilter->SetRegionOfInterest(m_ITKRegion);


	} else {
		//cannot compute attenuation
		((QLabel *)(this->m_ui->itemAtPosition(3, 1)->widget()))->setText("---");
		((QLabel *)(this->m_ui->itemAtPosition(0, 1)->widget()))->setText("---");

       //graph literal rf values
		for (int l = region[0]; l < region[1]; l++) {
			double the_value = (double)rf->GetPixel({ l, (region[2] + region[3]) / 2 });
			max = std::max(the_value, max);
			min = std::min(the_value, min);
		}

		for (int l = region[0]; l < region[1]; l++) {
			double the_value = rf->GetPixel({ l, (region[2] + region[3]) / 2 });
			the_value = vres * (the_value - min) / (max - min);
			int val = (int)the_value;
			for (int w = 0; w < vres; w++) {
				graph->SetPixel({ l - region[0], w }, 255 * (w > val));
			}
		}
	}

	//Bmode intensity and stddev
	m_BmodeROIFilter->SetInput(bmode);
	m_StatsFilter->SetInput(m_BmodeROIFilter->GetOutput());
	m_StatsFilter->Update();
	((QLabel * )(this->m_ui->itemAtPosition(2, 1)->widget()))->setText(std::to_string(m_StatsFilter->GetMean()).c_str());

	((QLabel * )(this->m_ui->itemAtPosition(1, 1)->widget()))->setText(std::to_string(m_StatsFilter->GetSigma()).c_str());

	QImage image = ITKQtHelpers::GetQImageColor<ImageType>(
		graph,
		graph->GetLargestPossibleRegion(),
		QImage::Format_RGB16
		);
	//std::cout << "made graph qimage" << std::endl;
	m_graph->setPixmap(QPixmap::fromImage(image));
	m_graph->setScaledContents(true);
	m_graph->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);

}

bool is_near_edge(int min, int max, int val) {
	int width = max - min;
	val = val - min;
	float scaled = val / (float)width;
	return scaled > .92 || scaled < .08;
}

void MeasurementWindow::DrawRectangle(itk::VectorImage<double, 2>::Pointer composite, itk::CurvilinearArraySpecialCoordinatesImage<double, 2>::Pointer curvedImage, int index) {

	for (double i = region[0]; i < region[1]; i+= 2) {
		for (double j = region[2]; j < region[3]; j+= .1) {
			if (is_near_edge(region[0], region[1], i) || is_near_edge(region[2], region[3], j)) {
				if (curvedImage) {
					itk::Point<double, 2> the_point;
					const double idx_pt[2] = { i, j };
					curvedImage->TransformContinuousIndexToPhysicalPoint<double, double>(itk::ContinuousIndex<double, 2>(idx_pt), the_point);
					auto point_index = composite->TransformPhysicalPointToIndex<double>(the_point);

					if (composite->GetLargestPossibleRegion().IsInside(point_index)) {
						auto val = composite->GetPixel(point_index);
						val.SetElement(index, 255);
						composite->SetPixel(point_index, val);
					}
				}
				else {
					///std::cout << composite->GetLargestPossibleRegion() << std::endl;
					//std::cout << "i" << i << std::endl;
					//std::cout << "j" << j << std::endl;
					auto val = composite->GetPixel({ (int)j, (int)i });
					val.SetElement(index, 255);
				}
			}
		}
	}
}

void MeasurementWindow::SetRegion(int x, int y) {
	
    region[0] = std::max(0, x - xsize);
    region[1] = std::min(2048, x + xsize);
    region[2] = std::max(0, y -  ysize);
    region[3] = std::min(127, y +  ysize);

	ImageType::SizeType size;

	size[0] = region[1] - region[0];
	size[1] = region[3] - region[2];

	ImageType::IndexType index;
	index[0] = region[0];
	index[1] = region[2];

	m_ITKRegion.SetIndex(index);
	m_ITKRegion.SetSize(size);

	m_BmodeROIFilter->SetRegionOfInterest(m_ITKRegion);
	m_RFROIFilter->SetRegionOfInterest(m_ITKRegion);

	ImageType::SizeType halfsize;

	halfsize[0] = (region[1] - region[0]) / 2;
	halfsize[1] = region[3] - region[2];

	ImageType::IndexType farIndex;
	farIndex[0] = region[0] + halfsize[0];
	farIndex[1] = region[2];

	m_nearITKRegion.SetIndex(index);
	m_nearITKRegion.SetSize(halfsize);

	m_farITKRegion.SetIndex(farIndex);
	m_farITKRegion.SetSize(halfsize);
	

	//m_StatsFilter->SetRegionOf
}
