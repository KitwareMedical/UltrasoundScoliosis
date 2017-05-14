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

#include "OpticNerveUI.h"
#include "ITKFilterFunctions.h"

#include <sstream>
#include <iomanip>

void OpticNerveUI::closeEvent (QCloseEvent *event){

#ifdef DEBUG_PRINT
     std::cout << "Close event called" << std::endl;
#endif
   opticNerveCalculator.Stop();
   intersonDevice.Stop();
}

OpticNerveUI::OpticNerveUI(int numberOfThreads, int bufferSize, QWidget *parent) 
	: QMainWindow(parent), ui(new Ui::MainWindow), 
          lastRendered(-1), lastOverlayRendered(-1)
{

	//Setup the graphical layout on this current Widget
	ui->setupUi(this);

	this->setWindowTitle("Ultrasound Optic Nerve Estimation");

	//Links buttons and actions
	connect( ui->pushButton_ConnectProbe, 
                 SIGNAL(clicked()), this, SLOT(ConnectProbe()));
	connect( ui->pushButton_Estimation, 
                 SIGNAL(clicked()), this, SLOT(ToggleEstimation()));
	connect( ui->spinBox_Depth, 
                 SIGNAL(valueChanged(int)), this, SLOT(SetDepth()));
	connect( ui->dropDown_Frequency, 
                 SIGNAL(currentIndexChanged(int)), this, SLOT(SetFrequency()));
        connect( ui->spinBox_NerveTop, SIGNAL( valueChanged(int) ), this,
                 SLOT( SetNerveTop() ) ); 
        connect( ui->spinBox_NerveDepth, SIGNAL( valueChanged(int) ), this,
                 SLOT( SetNerveDepth() ) ); 
        
        intersonDevice.SetRingBufferSize( bufferSize );

        opticNerveCalculator.SetNumberOfThreads( numberOfThreads ); 
 
               

	// Timer
	this->timer = new QTimer(this);
	this->timer->setSingleShot(true);
	this->timer->setInterval(50);
	this->connect(timer, SIGNAL(timeout()), SLOT(UpdateImage()));
}

OpticNerveUI::~OpticNerveUI()
{
	//this->intersonDevice.Stop();
	delete ui;
}

void OpticNerveUI::ConnectProbe(){
#ifdef DEBUG_PRINT
  std::cout << "Connect probe called" << std::endl;
#endif
  if ( !intersonDevice.ConnectProbe() ){
#ifdef DEBUG_PRINT
    std::cout << "Connect probe failed" << std::endl;
#endif
    return;
    //TODO: Show UI message
  }
  
  IntersonArrayDevice::FrequenciesType fs = intersonDevice.GetFrequencies();
  ui->dropDown_Frequency->clear();
  for(int i=0; i<fs.size(); i++){
     std::ostringstream ftext;   
     ftext << std::setprecision(1) << std::setw(3) << std::fixed;
     ftext << fs[i] << " hz";
     ui->dropDown_Frequency->addItem( ftext.str().c_str() );
  }

  if ( !intersonDevice.Start() ){
#ifdef DEBUG_PRINT
    std::cout << "Starting scan failed" << std::endl;
#endif
    return;
    //TODO: Show UI message
  }

    
  this->timer->start();
}



void OpticNerveUI::UpdateImage(){

   //display bmode image
  int currentIndex = intersonDevice.GetCurrentIndex();
  if( currentIndex >= 0 && currentIndex != lastRendered ){
     lastRendered = currentIndex;
     IntersonArrayDevice::ImageType::Pointer bmode = 
                                 intersonDevice.GetImage( currentIndex); 
 
/*    
     ITKFilterFunctions<IntersonArrayDevice::ImageType>::FlipArray flip;
     flip[0] = false;
     flip[1] = true;
     bmode = ITKFilterFunctions<IntersonArrayDevice::ImageType>::FlipImage(bmode , flip);
  */    
     ITKFilterFunctions< IntersonArrayDevice::ImageType >::PermuteArray order;
     order[0] = 1;
     order[1] = 0;
     bmode= ITKFilterFunctions< IntersonArrayDevice::ImageType>::PermuteImage(bmode, order);

     QImage image = ITKQtHelpers::GetQImageColor<IntersonArrayDevice::ImageType>( 
                          bmode,
                          bmode->GetLargestPossibleRegion(), 
                          QImage::Format_RGB16 
                       );
  
#ifdef DEBUG_PRINT
   // std::cout << "Setting Pixmap " << currentIndex << std::endl;
#endif
    ui->label_BModeImage->setPixmap(QPixmap::fromImage(image));
    ui->label_BModeImage->setScaledContents( true );
    ui->label_BModeImage->setSizePolicy( QSizePolicy::Ignored, QSizePolicy::Ignored );
  }


  //display estimate image
  int currentOverlayIndex = opticNerveCalculator.GetCurrentIndex();
  if( currentOverlayIndex >= 0 && currentOverlayIndex != lastOverlayRendered ){
#ifdef DEBUG_PRINT
     std::cout << "Display overlay image" << std::endl;
#endif
     lastOverlayRendered = currentOverlayIndex;
     //Set overlay image to display
     typedef OpticNerveEstimator::RGBImageType RGBImageType;
     RGBImageType::Pointer overlay = 
            opticNerveCalculator.GetImage( currentOverlayIndex );

/*
     typedef itk::PermuteAxesImageFilter<RGBImageType> PermuteFilter;
     typedef typename PermuteFilter::Pointer PermuteFilterPointer;
     typedef typename PermuteFilter::PermuteOrderArrayType PermuteArray;
     PermuteFilterPointer permute = PermuteFilter::New();
     PermuteArray order;
     order[0] = 1;
     order[1] = 0;
     permute->SetOrder(  order );
     permute->SetInput( overlay ); 
     permute->Update();
     overlay = permute->GetOutput();
*/

     QImage qimage = ITKQtHelpers::GetQImageColor_Vector< RGBImageType>( 
                           overlay,
                           overlay->GetLargestPossibleRegion(), 
                           QImage::Format_RGB16 );

     ui->label_OpticNerveImage->setPixmap(QPixmap::fromImage(qimage));
     ui->label_OpticNerveImage->setScaledContents( true );
     ui->label_OpticNerveImage->setSizePolicy( QSizePolicy::Ignored, QSizePolicy::Ignored );
  }

  float mm = intersonDevice.GetMmPerPixel();
  //display the estimates
  std::ostringstream cestimate;   
  cestimate << std::setprecision(1) << std::setw(3) << std::fixed;
  cestimate << opticNerveCalculator.GetCurrentEstimate() * mm << " mm";
  ui->label_estimateCurrent->setText( cestimate.str().c_str() );

  std::ostringstream mestimate;   
  mestimate << std::setprecision(1) << std::setw(3) << std::fixed;
  mestimate << opticNerveCalculator.GetMeanEstimate() * mm<< " mm";
  ui->label_estimateMean->setText( mestimate.str().c_str() );
  
  std::ostringstream mdestimate;   
  mdestimate << std::setprecision(1) << std::setw(3) << std::fixed;
  mdestimate << opticNerveCalculator.GetMedianEstimate() * mm << " mm";
  ui->label_estimateMedian->setText( mdestimate.str().c_str() );
  this->timer->start();
}



void OpticNerveUI::SetFrequency(){
  this->intersonDevice.SetFrequency( 
                     this->ui->dropDown_Frequency->currentIndex() + 1 ); 
}

void OpticNerveUI::SetDepth(){
  int depth = this->intersonDevice.SetDepth( 
                                this->ui->spinBox_Depth->value() );
  this->ui->spinBox_Depth->setValue(depth);
}

void OpticNerveUI::ToggleEstimation(){
  if( opticNerveCalculator.isRunning() ){
    opticNerveCalculator.Stop();
    ui->pushButton_Estimation->setText( "Start Estimation" );
  } 
  else{
    opticNerveCalculator.StartProcessing( &intersonDevice );
    ui->pushButton_Estimation->setText( "Stop Estimation" );
  }
}

void OpticNerveUI::SetNerveTop(){
  this->opticNerveCalculator.SetDepth( 
                                this->ui->spinBox_NerveTop->value() );
}

void OpticNerveUI::SetNerveDepth(){
  this->opticNerveCalculator.SetHeight( 
                                this->ui->spinBox_NerveDepth->value() );
}