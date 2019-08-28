/*=========================================================================

Library:   UltrasoundSpectroscopyRecorder

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

// Interson Array probe
//const double IntersonFrequencies[] = { 5.0, 7.5, 10.0 };
// Interson probe
//const double frequencies[] = { 2.5, 3.5, 5.0 };

#include "MeasurementExplorerUI.h"
#include "ClickableUltrasound.h"

#include "qsettings.h"
#include <QStandardPaths>
#include <qmessagebox.h>

#include "ITKFilterFunctions.h"

#include "itkCurvilinearArraySpecialCoordinatesImage.h"
#include "itkResampleImageFilter.h"
#include "itkWindowedSincInterpolateImageFunction.h"

#include "itkBModeImageFilter.h"
#include "itkTileImageFilter.h"
#include "itkImageFileWriter.h"
#include "itkImageFileReader.h"
#include "ITKQtHelpers.hxx"

#include <sstream>
#include <iomanip>

void MeasurementExplorerUI::closeEvent( QCloseEvent *event )
{
#ifdef DEBUG_PRINT
  std::cout << "Close event called" << std::endl;
#endif
  intersonDevice.Stop();
}

MeasurementExplorerUI::MeasurementExplorerUI( int numberOfThreads, int bufferSize, QWidget *parent )
  : QMainWindow( parent ), ui( new Ui::MainWindow ),
  lastRendered( -1 ),
  mmPerPixel( 1 )
{
  //Setup the graphical layout on this current Widget
  ui->setupUi( this );

  this->setWindowTitle( "Ultrasound Measurement Explorer" );

  this->outputFilename = QSettings("Kitware", "MeasurementExplorer").value("outputFilename", QStandardPaths::DesktopLocation + "new_ultrasound").toString();
  this->ui->label_outputfile->setText(QFileInfo(this->outputFilename).baseName());


  //Links buttons and actions
  connect( ui->pushButton_ConnectProbe,
    SIGNAL( clicked() ), this, SLOT( ConnectProbe() ) );
  connect(ui->recordButton,
	  SIGNAL(clicked()), this, SLOT(Record()));
  connect( ui->spinBox_Depth,
    SIGNAL( valueChanged( int ) ), this, SLOT( SetDepth() ) );
  connect( ui->dropDown_Frequency,
    SIGNAL( currentIndexChanged( int ) ), this, SLOT( SetFrequency() ) );

  connect( ui->radioButtonPowerSpectrum,
	  SIGNAL( clicked() ), this, SLOT( SetPowerSpectrum() ) );
  connect(ui->resetGraphAxes,
	  SIGNAL(clicked()), this, SLOT( SetPowerSpectrum() ));

  connect( ui->moveRed,
	  SIGNAL( clicked() ), this, SLOT( SetActive0() ) );
  connect(ui->moveGreen,
	  SIGNAL(clicked()), this, SLOT(SetActive1()));
  connect(ui->moveBlue,
	  SIGNAL(clicked()), this, SLOT(SetActive2()));

  connect(ui->freezePlayButton,
	  SIGNAL(clicked()), this, SLOT(FreezeButtonClicked()));
  connect(ui->setOutputFile,
	  SIGNAL(clicked()), this, SLOT(SetOutputFile()));
  connect(ui->pushbutton_LoadCine,
	  SIGNAL(clicked()), this, SLOT(LoadCine()));



  intersonDevice.SetRingBufferSize( bufferSize );

  ui->label_BModeImage->mainWindow = this;

  this->processing = new QTimer( this );
  this->processing->setSingleShot( false );
  this->processing->setInterval( 1000 );
  
  // Timer
  this->timer = new QTimer( this );
  this->timer->setSingleShot( true );
  this->timer->setInterval( 50 );
  this->connect( timer, SIGNAL( timeout() ), SLOT( UpdateImage() ) );

  //Set up b-mode rendering, a la SpectroscopyUI

  m_CastFilter = CastFilter::New();

  m_CastToIntersonImageTypeFilter = CastToIntersonImageTypeFilter::New();
  m_BModeFilter = BModeImageFilter::New();
  //add frequency filter
  m_BandpassFilter = ButterworthBandpassFilter::New();

  this->m_BandpassFilter->SetUpperFrequency(.9);

  this->m_BandpassFilter->SetLowerFrequency(.1);

  typedef BModeImageFilter::FrequencyFilterType  FrequencyFilterType;
  FrequencyFilterType::Pointer freqFilter = FrequencyFilterType::New();
  freqFilter->SetFilterFunction(m_BandpassFilter);

  m_BModeFilter->SetFrequencyFilter(freqFilter);

  m_BModeFilter->SetInput(m_CastFilter->GetOutput());

  m_RescaleFilter = RescaleFilter::New();
  m_RescaleFilter->SetOutputMinimum(0);
  m_RescaleFilter->SetOutputMaximum(255);
  m_RescaleFilter->SetInput(m_BModeFilter->GetOutput());

  m_WindowIntensityFilter = WindowIntensityFilter::New();
  m_WindowIntensityFilter->SetFunctor([](double x) {return x; });

  measurement_windows[0] = new MeasurementWindow(ui->measurement_window_0, ui->redGraph);
  measurement_windows[1] = new MeasurementWindow(ui->measurement_window_2, ui->greenGraph);
  measurement_windows[2] = new MeasurementWindow(ui->measurement_window_3, ui->blueGraph);

  measurement_windows[0]->SetRegion(430, 43);
  measurement_windows[1]->SetRegion(460, 43);
  measurement_windows[2]->SetRegion(430, 46);

  m_TransposeFilter = TransposeFilter::New();
  TransposeFilter::PermuteOrderArrayType order;
  order[0] = 1;
  order[1] = 0;

  m_TransposeFilter->SetOrder(order);

  m_TransposeFilter->SetInput(m_RescaleFilter->GetOutput());

  m_CurvedImage = CurvedImageType::New();
  CurvedImageType::IndexType imageIndex;
  imageIndex.Fill(0);

  CurvedImageType::SizeType imageSize;
  imageSize[0] = 2048;
  imageSize[1] = 127;

  CurvedImageType::RegionType imageRegion;
  imageRegion.SetIndex(imageIndex);
  imageRegion.SetSize(imageSize);

  m_CurvedImage->SetRegions(imageRegion);
  m_CurvedImage->Allocate();

  m_CurvedImage->FillBuffer(128);
  
  const double lateralAngularSeparation = (vnl_math::pi / 3) /
	  (imageSize[1] - 1);
  //std::cout << "lateral angular separation: " << lateralAngularSeparation << std::endl;
  m_CurvedImage->SetLateralAngularSeparation(lateralAngularSeparation);
  const double radiusStart = 46.4;

  m_CurvedImage->SetFirstSampleDistance(radiusStart);
  mmPerPixel = (1. / (30000000.)) * 1540. * 1000.;
  m_CurvedImage->SetRadiusSampleSize(mmPerPixel);

  m_CurvedImage->Print(std::cout);
  
  std::cout << "initialized" << std::endl;

  m_ResampleFilter = ResampleImageFilterType::New();
  m_ResampleFilter->SetInput(m_CurvedImage);

  
  CurvedImageType::SizeType outputSize = m_CurvedImage->GetLargestPossibleRegion().GetSize();
  outputSize[0] = 800;
  outputSize[1] = 800;
  
  m_ResampleFilter->SetSize(outputSize);
  
  CurvedImageType::SpacingType outputSpacing;
  outputSpacing.Fill(0.15);
  m_ResampleFilter->SetOutputSpacing(outputSpacing);
  CurvedImageType::PointType outputOrigin;
  outputOrigin[0] = outputSize[0] * outputSpacing[0] / -2.0;
  outputOrigin[1] = radiusStart * std::cos(vnl_math::pi / 6.0);
  m_ResampleFilter->SetOutputOrigin(outputOrigin);
  
  m_ComposeFilter = ComposeImageFilter::New();

  m_ComposeFilter->SetInput(0, m_ResampleFilter->GetOutput());
  m_ComposeFilter->SetInput(1, m_ResampleFilter->GetOutput());
  m_ComposeFilter->SetInput(2, m_ResampleFilter->GetOutput());

  //m_ComposeFilter = ComposeImageFilter::New();

  //m_ComposeFilter->SetInput(0, m_RescaleFilter->GetOutput());
 // m_ComposeFilter->SetInput(1, m_RescaleFilter->GetOutput());
//  m_ComposeFilter->SetInput(2, m_RescaleFilter->GetOutput());
}

void MeasurementExplorerUI::ConnectFiltersForCurvedImage() {
	m_ComposeFilter = ComposeImageFilter::New();

	m_ComposeFilter->SetInput(0, m_ResampleFilter->GetOutput());
	m_ComposeFilter->SetInput(1, m_ResampleFilter->GetOutput());
	m_ComposeFilter->SetInput(2, m_ResampleFilter->GetOutput());

	is_curved = true;

}

void MeasurementExplorerUI::ConnectFiltersForLinearImage() {
	m_ComposeFilter = ComposeImageFilter::New();
	m_ComposeFilter->SetInput(0, m_TransposeFilter->GetOutput());
	m_ComposeFilter->SetInput(1, m_TransposeFilter->GetOutput());
	m_ComposeFilter->SetInput(2, m_TransposeFilter->GetOutput());

	is_curved = false;
}



MeasurementExplorerUI::~MeasurementExplorerUI()
{
  this->intersonDevice.Stop();
  delete ui;
}

void MeasurementExplorerUI::SetActive0() { this->active_measurement_window = 0; }
void MeasurementExplorerUI::SetActive1() { this->active_measurement_window = 1; }
void MeasurementExplorerUI::SetActive2() { this->active_measurement_window = 2; }

void MeasurementExplorerUI::ConnectProbe()
{
#ifdef DEBUG_PRINT
	std::cout << "Connect probe called" << std::endl;
#endif
	if (!intersonDevice.ConnectProbe(true))
	{
#ifdef DEBUG_PRINT
		std::cout << "Connect probe failed" << std::endl;
#endif
		return;
		//TODO: Show UI message
	}

	IntersonArrayDeviceRF::FrequenciesType fs = intersonDevice.GetFrequencies();
	ui->dropDown_Frequency->clear();
	for (unsigned int i = 0; i < fs.size(); i++)
	{
		std::ostringstream ftext;
		ftext << std::setprecision(1) << std::setw(3) << std::fixed;
		ftext << fs[i] << " hz";
		ui->dropDown_Frequency->addItem(ftext.str().c_str());
	}

	intersonDevice.GetHWControls().SetNewHardButtonCallback(&ProbeHardButtonCallback, this);

	if (!intersonDevice.Start())
	{
#ifdef DEBUG_PRINT
		std::cout << "Starting scan failed" << std::endl;
#endif
		return;
		//TODO: Show UI message
	}

	this->ui->dropDown_Frequency->setCurrentIndex(1);
	this->SetFrequency();
	this->SetDepth();

	this->timer->start();

	if (this->intersonDevice.GetProbeId() == this->intersonDevice.GetHWControls().ID_CA_5_0MHz) {
		this->ConnectFiltersForCurvedImage();
	}
	else {
		this->ConnectFiltersForLinearImage();
	}


}


// The following function is creative commons from https://stackoverflow.com/questions/18073378/add-unique-suffix-to-file-name
// form - "path/to/file.tar.gz", "path/to/file (1).tar.gz",
// "path/to/file (2).tar.gz", etc.
QString addUniqueSuffix(const QString &fileName)
{
	// If the file doesn't exist return the same name.
	if (!QFile::exists(fileName)) {
		return fileName;
	}

	QFileInfo fileInfo(fileName);
	QString ret;

	// Split the file into 2 parts - dot+extension, and everything else. For
	// example, "path/file.tar.gz" becomes "path/file"+".tar.gz", while
	// "path/file" (note lack of extension) becomes "path/file"+"".
	QString secondPart = fileInfo.completeSuffix();
	QString firstPart;
	if (!secondPart.isEmpty()) {
		secondPart = "." + secondPart;
		firstPart = fileName.left(fileName.size() - secondPart.size());
	}
	else {
		firstPart = fileName;
	}

	// Try with an ever-increasing number suffix, until we've reached a file
	// that does not yet exist.
	for (int ii = 1; ; ii++) {
		// Construct the new file name by adding the unique number between the
		// first and second part.
		ret = QString("%1 (%2)%3").arg(firstPart).arg(ii).arg(secondPart);
		// If no file exists with the new name, return it.
		if (!QFile::exists(ret)) {
			return ret;
		}
	}
}


void MeasurementExplorerUI::Record() {
	
	if (!this->is_frozen) {
		this->FreezeButtonClicked();
	}

	if (outputFilename == "") {
		this->SetOutputFile();
	}
		
	QPixmap originalPixmap;

	originalPixmap = QPixmap::grabWidget(ui->centralwidget);

		
	char* format = "png";

	QString filename = addUniqueSuffix(this->outputFilename + "." + format);
	originalPixmap.save(filename, format);


	typedef itk::Image<IntersonArrayDeviceRF::RFImageType::PixelType, 3> StackedImageType;

	typedef itk::TileImageFilter<IntersonArrayDeviceRF::RFImageType, StackedImageType> FilterType;

	auto tiler = FilterType::New();
	itk::FixedArray< unsigned int, 3> layout;

	layout[0] = 1;
	layout[1] = 1;
	layout[2] = 0;

	tiler->SetLayout(layout);

	for (int i = 0; i < intersonDevice.GetRingBufferSize(); i++) {
		std::cerr << i << std::endl;
		tiler->SetInput(i, this->intersonDevice.GetRFImage((i + intersonDevice.GetCurrentRFIndex()) % intersonDevice.GetRingBufferSize()));
	}
	IntersonArrayDeviceRF::ImageType::PixelType filler = 0;
	tiler->SetDefaultPixelValue(filler);

	tiler->Update();

	typedef itk::ImageFileWriter<StackedImageType> WriterType;

	auto writer = WriterType::New();

	this->ui->label_outputfile->setText(QFileInfo(addUniqueSuffix(this->outputFilename + "." + format)).baseName());

	writer->SetInput(tiler->GetOutput());
	writer->SetFileName((filename.chopped(4) + ".nrrd").toStdString());

	writer->Update();
}

void MeasurementExplorerUI::UpdateImage()
{

  // O(n^2) but also O(9). Needs to be fixed if we ever have thousands of measurement windows?
  for (int i = 0; i < 3; i++) {
	  for (int j = 0; j < 3; j++) {

		  this->measurement_windows[i]->max = std::max(this->measurement_windows[i]->max, this->measurement_windows[j]->max);
		  this->measurement_windows[i]->min = std::min(this->measurement_windows[i]->min, this->measurement_windows[j]->min);
	  }
  }

  //display bmode image
  int currentIndex = intersonDevice.GetCurrentRFIndex();
  if( is_frozen ){
	  currentIndex -= intersonDevice.GetRingBufferSize() - this->ui->frameSlider->value();

	  while (currentIndex < 0)
		  currentIndex += this->intersonDevice.GetRingBufferSize();
	  

  }
  if( currentIndex >= 0 && currentIndex != lastRendered )
    {
    lastRendered = currentIndex;
    IntersonArrayDeviceRF::RFImageType::Pointer rf =
      intersonDevice.GetRFImage( currentIndex );

	if (is_curved) {
		for (int i = 0; i < 127; i++) {
			rf->SetPixel({ 2047, i }, rf->GetPixel({ 2046, i })); 
		}
	}

	m_CastFilter->SetInput(rf);
	m_RescaleFilter->Update();

	for (int i = 0; i < 3; i++) {
		this->measurement_windows[i]->UpdateMeasurements(rf, m_BModeFilter->GetOutput());
	}

	if (is_curved) {

		// Copy data into Curvilinear Image
		const ImageType::RegionType & largestRegion =
			m_RescaleFilter->GetOutput()->GetLargestPossibleRegion();
		const ImageType::SizeType imageSize = largestRegion.GetSize();

		double *imageBuffer = m_RescaleFilter->GetOutput()->GetPixelContainer()->GetBufferPointer();
		double *curvedImageBuffer = m_CurvedImage->GetPixelContainer()->GetBufferPointer();

		std::memcpy(curvedImageBuffer, imageBuffer,
			imageSize[0] * imageSize[1] * sizeof(double));
		// Done Copying

		m_CurvedImage->Modified();
	}
	m_ComposeFilter->Update();

	ComposeImageFilter::OutputImagePointer composite = m_ComposeFilter->GetOutput();

	for (int i = 0; i < 3; i++) {
		this->measurement_windows[i]->DrawRectangle(composite, is_curved ? m_CurvedImage : nullptr, i);
	}

	QImage image = ITKQtHelpers::GetQImageColor_Vector<ComposeImageFilter::OutputImageType>(
		composite,
		composite->GetLargestPossibleRegion(),
		QImage::Format_RGB16
		);

    ui->label_BModeImage->setPixmap( QPixmap::fromImage( image ) );
    ui->label_BModeImage->setScaledContents( true );
    ui->label_BModeImage->setSizePolicy( QSizePolicy::Ignored, QSizePolicy::Ignored );
  }
  this->timer->start();
}

void MeasurementExplorerUI::SetFrequency()
{
  this->intersonDevice.SetFrequency(
    this->ui->dropDown_Frequency->currentIndex() );
}

void MeasurementExplorerUI::SetDepth()
{
  int depth = this->intersonDevice.SetDepth(
    this->ui->spinBox_Depth->value() );
  this->ui->spinBox_Depth->setValue( depth );
}

void MeasurementExplorerUI::SetDoubler()
{
  //this->intersonDevice.SetDoubler( this->ui->checkBox_doubler->isChecked() );
}

void MeasurementExplorerUI::SetPowerSpectrum()
{
	for (int i = 0; i < 3; i++) {
		this->measurement_windows[i]->graphPowerSpectrum = this->ui->radioButtonPowerSpectrum->isChecked();
		this->measurement_windows[i]->max = -9999999999;
		this->measurement_windows[i]->min = 99999999999;
	}
	this->lastRendered = -1;
	this->UpdateImage();
	this->lastRendered = -1;
	this->UpdateImage();
}


void MeasurementExplorerUI::ResetGraphLimits(){
	for (int i = 0; i < 3; i++) {
		this->measurement_windows[i]->max = -9999999999;
		this->measurement_windows[i]->min = 99999999999;
	}
	this->lastRendered = -2;
	this->UpdateImage();
	this->lastRendered = -1;
	this->UpdateImage();
}


void MeasurementExplorerUI::OnUSClicked(QPoint pos) {
	//std::cout << active_measurement_window << std::endl;
	if (is_curved) {
		itk::Point<double, 2> the_point;
		const double idx_pt[2] = { pos.x(), pos.y() };
		m_ComposeFilter->GetOutput()->TransformContinuousIndexToPhysicalPoint<double, double>(itk::ContinuousIndex<double, 2>(idx_pt), the_point);
		itk::Index<2> rf_idx;
		m_CurvedImage->TransformPhysicalPointToIndex<double>(the_point, rf_idx);

		if (m_CurvedImage->GetLargestPossibleRegion().IsInside(rf_idx)) {

			//std::cout << "region" << rf_idx[0] << " " << rf_idx[1] << std::endl;

			measurement_windows[active_measurement_window]->SetRegion(rf_idx[0], rf_idx[1]);
		}
	}
	else {
		measurement_windows[active_measurement_window]->SetRegion(pos.y() / 800. * 2048. , pos.x() / 800. * 128.);
	}
	this->lastRendered = -1;
	this->UpdateImage();
	this->lastRendered = -1;
	this->UpdateImage();
}

void MeasurementExplorerUI::FreezeButtonClicked() {

	this->ui->frameSlider->setValue(99);
	if (this->is_frozen) {
		this->intersonDevice.Start();
		this->ui->frameSlider->setEnabled(false);
		this->ui->freezePlayButton->setText("Freeze");
	} else {
		this->intersonDevice.Stop();
		this->ui->frameSlider->setEnabled(true);
		this->ui->freezePlayButton->setText("Start");
	}
	this->is_frozen = !this->is_frozen;
}

bool currently_asking_for_file_this_var_is_sin = false;
void MeasurementExplorerUI::SetOutputFile() {
	if (!currently_asking_for_file_this_var_is_sin) {
		currently_asking_for_file_this_var_is_sin = true;


		QString fileName = QFileDialog::getSaveFileName(this, "name for file", outputFilename, "*.png");
		QSettings("Kitware", "MeasurementExplorer").setValue("outputFilename", fileName);
		this->outputFilename = fileName;
		this->ui->label_outputfile->setText(QFileInfo(fileName).baseName());
		currently_asking_for_file_this_var_is_sin = false;
	}

	Record();
}

void MeasurementExplorerUI::LoadCine() {
	if (!this->is_frozen)
		this->FreezeButtonClicked();
	if (!currently_asking_for_file_this_var_is_sin) {
		currently_asking_for_file_this_var_is_sin = true;

		QString fileName = QFileDialog::getOpenFileName(this, "Cine to load", QFileInfo(outputFilename).path(), "*.nrrd");

		if (!QFileInfo(fileName).isFile()) {
			currently_asking_for_file_this_var_is_sin = false;
			return;
		}
			
		typedef itk::Image<IntersonArrayDeviceRF::RFImageType::PixelType, 3> StackedImageType;
		using ReaderType = itk::ImageFileReader< StackedImageType >;
		ReaderType::Pointer reader = ReaderType::New();

		reader->SetFileName(fileName.toStdString());

		reader->Update();


		auto image = reader->GetOutput();

		if (image->GetLargestPossibleRegion() != StackedImageType::RegionType({ 0, 0, 0 }, { (uint)intersonDevice.GetRFModeDepthResolution(), (uint)intersonDevice.GetNumberOfLines(), (uint)intersonDevice.GetRingBufferSize() })) {
			QMessageBox::warning(this, "File incompatible", "The requested file was not able to be read as a Cine taken with the currently connected probe.");
			currently_asking_for_file_this_var_is_sin = false;
			return;
		}
		
		for (int i = 0; i < intersonDevice.GetRingBufferSize(); i++) {
			this->intersonDevice.AddRFImageToBuffer(image->GetBufferPointer() + i * intersonDevice.GetRFModeDepthResolution() * intersonDevice.GetNumberOfLines());
		}
		this->lastRendered = -1;
		this->UpdateImage();
		this->lastRendered = -1;
		this->UpdateImage();
		currently_asking_for_file_this_var_is_sin = false;
	}
}


















