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

#include "ScoliosisUI.h"

#include "ScoliosisServer.h"
#include "ScoliosisQueryNN.h"

#include "ITKQtHelpers.hxx"
#include "ITKFilterFunctions.h"
#include "itkTileImageFilter.h"
#include "itkImageFileWriter.h"

#include <sstream>
#include <iomanip>
#include <string>
#include <time.h> 


#define BOOST_SPIRIT_THREADSAFE
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>


void ScoliosisUI::closeEvent( QCloseEvent *event )
{

#ifdef DEBUG_PRINT
  std::cout << "Close event called" << std::endl;
#endif
  intersonDevice.Stop();
}

ScoliosisUI::ScoliosisUI( int numberOfThreads, int bufferSize, QWidget *parent )
  : QMainWindow( parent ), ui( new Ui::MainWindow ),
  lastRendered( -1 ),
  mmPerPixel( 1 ), nnSocketConnection()
{

  start_server();

  //Setup the graphical layout on this current Widget
  ui->setupUi( this );

  this->setWindowTitle( "Ultrasound Scoliosis Monitoring" );

  //Links buttons and actions
  connect( ui->pushButton_ConnectProbe,
    SIGNAL( clicked() ), this, SLOT( ConnectProbe() ) );
  connect( ui->spinBox_Depth,
    SIGNAL( valueChanged( int ) ), this, SLOT( SetDepth() ) );
  connect( ui->dropDown_Frequency,
    SIGNAL( currentIndexChanged( int ) ), this, SLOT( SetFrequency() ) );
  //connect( ui->checkBox_doubler, SIGNAL( stateChanged(int) ), this,
  //         SLOT( SetDoubler() ) );

  connect(ui->generateIdentifierButton, 
	SIGNAL(clicked()), this, SLOT(SetPatientID()));
  connect(ui->recordButton,
	  SIGNAL(clicked()), this, SLOT(Record()));
  connect(ui->stopButton,
	  SIGNAL(clicked()), this, SLOT(StopRecording()));



  intersonDevice.SetRingBufferSize( bufferSize );




  this->processing = new QTimer( this );
  this->processing->setSingleShot( false );
  this->processing->setInterval( 1000 );
  
  // Timer
  this->timer = new QTimer( this );
  this->timer->setSingleShot( true );
  this->timer->setInterval( 50 );
  this->connect( timer, SIGNAL( timeout() ), SLOT( UpdateImage() ) );

  this->updateConnectionUIsTimer = new QTimer(this);
  this->updateConnectionUIsTimer->setSingleShot(false);

  this->updateConnectionUIsTimer->setInterval(2000);
  this->connect(updateConnectionUIsTimer, SIGNAL ( timeout () ) , SLOT( UpdateConnectionUIs() ));
  this->updateConnectionUIsTimer->start();

  srand(time(NULL)); //for random patient IDs
  this->SetPatientID();
}

ScoliosisUI::~ScoliosisUI()
{
  //this->intersonDevice.Stop();
  delete ui;
}

void ScoliosisUI::ConnectProbe()
{
#ifdef DEBUG_PRINT
  std::cout << "Connect probe called" << std::endl;
#endif
  if( !intersonDevice.ConnectProbe( false ) )
    {
#ifdef DEBUG_PRINT
    std::cout << "Connect probe failed" << std::endl;
#endif
    return;
    //TODO: Show UI message
    }

  IntersonArrayDeviceRF::FrequenciesType fs = intersonDevice.GetFrequencies();
  ui->dropDown_Frequency->clear();
  for( unsigned int i = 0; i < fs.size(); i++ )
    {
    std::ostringstream ftext;
    ftext << std::setprecision( 1 ) << std::setw( 3 ) << std::fixed;
    ftext << fs[ i ] << " hz";
    ui->dropDown_Frequency->addItem( ftext.str().c_str() );
    }

  intersonDevice.GetHWControls().SetNewHardButtonCallback( &ProbeHardButtonCallback, this );

  mmPerPixel = intersonDevice.GetMmPerPixel();
  if( !intersonDevice.Start() )
    {
#ifdef DEBUG_PRINT
    std::cout << "Starting scan failed" << std::endl;
#endif
    return;
    //TODO: Show UI message
    }

  this->USConnected = true;

  this->ui->dropDown_Frequency->setCurrentIndex(1);
  this->SetFrequency();
  this->SetDepth();

  this->ui->l_probeConnected->setText("Probe Connected");

  this->timer->start();
}

void ScoliosisUI::Record() {
	if (this->state == WaitingToRecord) {
		this->scan_metadata.put("frequency", this->ui->dropDown_Frequency->currentIndex());
		this->scan_metadata.put("depth", this->ui->spinBox_Depth->value());

		boost::property_tree::ptree frame_metadata;
		this->scan_metadata.add_child("frame_metadata", frame_metadata);

		this->state = Recording;
		std::cout << "Recording" << std::endl;

	}
}
void ScoliosisUI::StopRecording() {

	if (this->state == Recording) {
		this->state = WritingToDisk;

		typedef itk::Image<IntersonArrayDeviceRF::ImageType::PixelType, 3> StackedImageType;

		typedef itk::TileImageFilter<IntersonArrayDeviceRF::ImageType, StackedImageType> FilterType;

		auto tiler = FilterType::New();
		itk::FixedArray< unsigned int, 3> layout;

		layout[0] = 1;
		layout[1] = 1;
		layout[2] = 0;

		tiler->SetLayout(layout);

		for (int i = 0; i < this->savedImages.size(); i++) {
			std::cerr << i << std::endl;
			tiler->SetInput(i, savedImages[i]);
		}
		IntersonArrayDeviceRF::ImageType::PixelType filler = 0;
		tiler->SetDefaultPixelValue(filler);

		tiler->Update();
		
		typedef itk::ImageFileWriter<StackedImageType> WriterType;

		auto writer = WriterType::New();

		writer->SetInput(tiler->GetOutput());
		writer->SetFileName("data\\" + this->patientID + "_" + std::to_string(++ this->scan_count) +  ".nrrd");

		writer->Update();

		this->savedImages.clear();

		boost::property_tree::write_json("data\\" + this->patientID + "_" + std::to_string(this->scan_count) + ".json", this->scan_metadata);
		this->scan_metadata.clear();

		this->state = WaitingToRecord;
	}
}



void ScoliosisUI::SetPatientID() {
	if (this->state == WaitingToRecord) {
		this->patientID = "------";
		
		for (int j = 0; j < 3; j++) {
			patientID[j] = rand() % 26 + 65;
			patientID[j + 3] = rand() % 10 + 48;
		}

		ui->lineEdit->setText(QString(this->patientID.c_str()));
		this->scan_count = 0;
	}
}

void ScoliosisUI::UpdateImage()
{
  //display bmode image
  int currentIndex = intersonDevice.GetCurrentBModeIndex();
  if( currentIndex >= 0 && currentIndex != lastRendered )
    {
    lastRendered = currentIndex;
    IntersonArrayDeviceRF::ImageType::Pointer bmode =
      intersonDevice.GetBModeImage( currentIndex );

    QImage image = ITKQtHelpers::GetQImageColor<IntersonArrayDeviceRF::ImageType>(
      bmode,
      bmode->GetLargestPossibleRegion(),
      QImage::Format_RGB16
      );

#ifdef DEBUG_PRINT
   // std::cout << "Setting Pixmap " << currentIndex << std::endl;
#endif

    ui->label_BModeImage->setPixmap( QPixmap::fromImage( image ) );
    ui->label_BModeImage->setScaledContents( true );
    ui->label_BModeImage->setSizePolicy( QSizePolicy::Ignored, QSizePolicy::Ignored );
	float theta, quality;
	if (this->NNConnected) {
		NetworkResponse resp = nnSocketConnection.QueryNN(bmode->GetBufferPointer());
		theta = resp.angleInRadians;

		ui->label_estimateCurrent->setText((std::to_string(theta) + " " + std::to_string((int)(100 * resp.stdDevInRadians / 8))).c_str());
		ui->imageQuality->setValue((int)(100 * resp.stdDevInRadians / 8));
		quality = resp.stdDevInRadians;
	}
	if (this->state == Recording) {
		this->savedImages.push_back(bmode);

		boost::property_tree::ptree frame;
		frame.put("accelerometer_angle", server_roll);
		frame.put("nn_angle", theta);
		frame.put("nn_quality", quality);
		this->scan_metadata.get_child("frame_metadata").push_back(std::make_pair("", frame));
	}
	
  }
  double r = server_roll;
  ui->label_ProbeToGround->setText(std::to_string(r).c_str());
  this->timer->start();
}

void ScoliosisUI::UpdateConnectionUIs() {
	if (!this->NNConnected) {
		this->NNConnected = this->nnSocketConnection.Connect();
		if (this->NNConnected) {
			ui->l_nnConnected->setText("Neural Network Connected");
		}
	}

	//todo server

	//todo us_probe

	if (NNConnected && USConnected && phoneConnected && this->state == WaitingForInitialization) {
		this->state = WaitingToRecord;
	}
}

void ScoliosisUI::SetFrequency()
{
  this->intersonDevice.SetFrequency(
    this->ui->dropDown_Frequency->currentIndex() );
}

void ScoliosisUI::SetDepth()
{
  int depth = this->intersonDevice.SetDepth(
    this->ui->spinBox_Depth->value() );
  this->ui->spinBox_Depth->setValue( depth );
}

void ScoliosisUI::SetDoubler()
{
  //this->intersonDevice.SetDoubler( this->ui->checkBox_doubler->isChecked() );
}
