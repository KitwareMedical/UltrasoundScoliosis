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

#include "ITKQtHelpers.hxx"
#include "ITKFilterFunctions.h"

#include <sstream>
#include <iomanip>
#include <string>
#include <time.h> 


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

  //Links buttons and actions
  connect( ui->pushButton_ConnectProbe,
    SIGNAL( clicked() ), this, SLOT( ConnectProbe() ) );
  connect( ui->spinBox_Depth,
    SIGNAL( valueChanged( int ) ), this, SLOT( SetDepth() ) );
  connect( ui->dropDown_Frequency,
    SIGNAL( currentIndexChanged( int ) ), this, SLOT( SetFrequency() ) );
  //connect( ui->checkBox_doubler, SIGNAL( stateChanged(int) ), this,
  //         SLOT( SetDoubler() ) );





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

  srand(time(NULL)); //for random patient IDs
  this->SetPatientID();
}

MeasurementExplorerUI::~MeasurementExplorerUI()
{
  //this->intersonDevice.Stop();
  delete ui;
}

void MeasurementExplorerUI::ConnectProbe()
{
#ifdef DEBUG_PRINT
  std::cout << "Connect probe called" << std::endl;
#endif
  if( !intersonDevice.ConnectProbe( true ) )
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


  this->ui->dropDown_Frequency->setCurrentIndex(1);
  this->SetFrequency();
  this->SetDepth();

  

  this->timer->start();
}

void MeasurementExplorerUI::Record() {
	

	
}
void MeasurementExplorerUI::StopRecording() {

}



void MeasurementExplorerUI::SetPatientID() {
	
		this->patientID = "------";
		
		for (int j = 0; j < 3; j++) {
			patientID[j] = rand() % 26 + 65;
			patientID[j + 3] = rand() % 10 + 48;
		}

		ui->lineEdit->setText(QString(this->patientID.c_str()));
}

void MeasurementExplorerUI::UpdateImage()
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
	
	int intensity = bmode->GetPixel({ pointForProcessing.x(), pointForProcessing.y() });
	ui->label_intensity->setText(std::to_string(intensity).c_str());
	
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

void MeasurementExplorerUI::OnUSClicked(QPoint pos) {
	pointForProcessing = pos;
}