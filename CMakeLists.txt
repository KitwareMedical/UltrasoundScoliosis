############################################################################
#
# Library:   UltrasoundOpticNerveUI
#
# Copyright 2010 Kitware Inc. 28 Corporate Drive,
# Clifton Park, NY, 12065, USA.
#
# All rights reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
############################################################################
cmake_minimum_required( VERSION 3.5 )

project( UltrasoundIntersonApps CXX )

option( Build_Spectroscopy ON )
option( Build_PTX ON )
option( BUILD_MEASUREMENTEXPLORER ON )

#Required packages and libraries
find_package( IntersonArraySDKCxx REQUIRED )

#find_package( PlusLib REQUIRED )
#include( ${PlusLib_USE_FILE} )

find_package( ITK REQUIRED )

if( ${Build_Spectroscopy} )
find_package( ITK COMPONENTS
  Ultrasound
  ITKBinaryMathematicalMorphology
  ITKCommon
  ITKDistanceMap
  ITKImageFilterBase
  ITKImageFunction
  ITKImageFusion
  ITKImageGrid
  ITKImageIntensity
  ITKLabelMap
  ITKMathematicalMorphology
  ITKMetricsv4
  ITKOptimizersv4
  ITKRegistrationMethodsv4
  ITKSmoothing
  ITKSpatialObjects
  ITKThresholding
  ITKTransform
  ITKIOImageBase
  ITKIONRRD
)
else()
find_package( ITK COMPONENTS
  ITKBinaryMathematicalMorphology
  ITKCommon
  ITKDistanceMap
  ITKImageFilterBase
  ITKImageFunction
  ITKImageFusion
  ITKImageGrid
  ITKImageIntensity
  ITKLabelMap
  ITKMathematicalMorphology
  ITKMetricsv4
  ITKOptimizersv4
  ITKRegistrationMethodsv4
  ITKSmoothing
  ITKSpatialObjects
  ITKThresholding
  ITKTransform
  ITKIOImageBase
  ITKIONRRD
)
endif()

include( ${ITK_USE_FILE} )

find_package( Qt5 COMPONENTS Core Widgets REQUIRED )

#For Scoliosis App
option( BUILD_SCOLIOSIS ON)
if( BUILD_SCOLIOSIS)
  include_directories( ${SimpleWebServer_Source_DIR})


  if(NOT MSVC)
      add_compile_options(-std=c++11 -Wall -Wextra -Wsign-conversion)
  else()
      add_compile_options(/W1)
  endif()

  add_library(simple-web-server INTERFACE)

  target_include_directories(simple-web-server INTERFACE ${SimpleWebServer_Source_DIR})

  find_package(Threads REQUIRED)
  target_link_libraries(simple-web-server INTERFACE ${CMAKE_THREAD_LIBS_INIT})

  # TODO 2020 when Debian Jessie LTS ends:
  # Remove Boost system, thread, regex components; use Boost::<component> aliases; remove Boost target_include_directories
  if(USE_STANDALONE_ASIO)
      target_compile_definitions(simple-web-server INTERFACE USE_STANDALONE_ASIO)
      find_file(HAVE_ASIO asio.hpp)
      if(NOT HAVE_ASIO)
          message(FATAL_ERROR "Standalone Asio not found")
      endif()
  else()
      find_package(Boost 1.69.0 COMPONENTS system date_time thread REQUIRED)
      target_link_libraries(simple-web-server INTERFACE ${Boost_LIBRARIES})
      target_include_directories(simple-web-server INTERFACE ${Boost_INCLUDE_DIR})
      if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.9)
          target_compile_definitions(simple-web-server INTERFACE USE_BOOST_REGEX)
          find_package(Boost 1.69.0 COMPONENTS regex REQUIRED)
          target_link_libraries(simple-web-server INTERFACE ${Boost_LIBRARIES})
          target_include_directories(simple-web-server INTERFACE ${Boost_INCLUDE_DIR})
      endif()
  endif()
  if(WIN32)
      target_link_libraries(simple-web-server INTERFACE ws2_32 wsock32)
  endif()

  if(APPLE)
      set(OPENSSL_ROOT_DIR "/usr/local/opt/openssl")
  endif()
  find_package(OpenSSL)
  if(OPENSSL_FOUND)
      target_compile_definitions(simple-web-server INTERFACE HAVE_OPENSSL)
      target_link_libraries(simple-web-server INTERFACE ${OPENSSL_LIBRARIES})
      target_include_directories(simple-web-server INTERFACE ${OPENSSL_INCLUDE_DIR})
  endif()

  
  
endif()

#Common setup
include_directories( ${CMAKE_SOURCE_DIR} )
include_directories( ${UltrasoundOpticNerveUI_SOURCE_DIR} )
include_directories( ${CMAKE_CURRENT_BINARY_DIR} )

add_definitions( -DNOMINMAX )


#Optic nerve ui executable

qt5_wrap_ui( OpticNerveUI_H OpticNerve.ui )
qt5_wrap_cpp( optic_source_files_moc OpticNerveUI.h )

set( optic_source_files
  OpticNerveUIDriver.cxx
  OpticNerveUI.cxx
  OpticNerveEstimator.cxx
)

add_executable( OpticNerveUI
  ${optic_source_files}
  ${OpticNerveUI_H}
  ${optic_source_files_moc}
)

target_link_libraries( OpticNerveUI PUBLIC
  ${ITK_LIBRARIES}
  ${IntersonArraySDKCxx_LIBRARIES}
  Qt5::Widgets
)

install( TARGETS OpticNerveUI
  RUNTIME DESTINATION bin COMPONENT Runtime
  LIBRARY DESTINATION bin COMPONENT Runtime
  ARCHIVE DESTINATION lib COMPONENT Development
)

#Scoliosis ui executable
if( BUILD_SCOLIOSIS )
  qt5_wrap_ui( ScoliosisUI_H Scoliosis.ui )
  qt5_wrap_cpp( scoliosis_source_files_moc ScoliosisUI.h )

  set( scoliosis_source_files
    ScoliosisDriver.cxx
    ScoliosisServer.cxx
    ScoliosisUI.cxx
    ScoliosisQueryNN.cxx
  )

  add_executable( ScoliosisUI
    ${scoliosis_source_files}
    ${ScoliosisUI_H}
    ${scoliosis_source_files_moc}
  )

  target_link_libraries( ScoliosisUI PUBLIC
    ${ITK_LIBRARIES}
    ${IntersonArraySDKCxx_LIBRARIES}
    Qt5::Widgets
    simple-web-server
    ${Boost_LIBRARIES}
  )

  target_include_directories(ScoliosisUI PRIVATE ${Boost_INCLUDE_DIR})

  install( TARGETS ScoliosisUI
    RUNTIME DESTINATION bin COMPONENT Runtime
    LIBRARY DESTINATION bin COMPONENT Runtime
    ARCHIVE DESTINATION lib COMPONENT Development
  )

  install( FILES vertebra-lumbalis-posterior-view.png DESTINATION bin)
endif()

#MeasurementExplorer ui executable
if( BUILD_MEASUREMENTEXPLORER )
  qt5_wrap_ui( MeasurementExplorerUI_H MeasurementExplorer.ui )
  qt5_wrap_cpp( MeasurementExplorer_source_files_moc MeasurementExplorerUI.h ClickableUltrasound.h)

  set( MeasurementExplorer_source_files
    MeasurementExplorerDriver.cxx
    MeasurementExplorerUI.cxx
	ClickableUltrasound.cxx
  )

  add_executable( MeasurementExplorerUI
    ${MeasurementExplorer_source_files}
    ${MeasurementExplorerUI_H}
    ${MeasurementExplorer_source_files_moc}
  )

  target_link_libraries( MeasurementExplorerUI PUBLIC
    ${ITK_LIBRARIES}
    ${IntersonArraySDKCxx_LIBRARIES}
    Qt5::Widgets
  )

  install( TARGETS MeasurementExplorerUI
    RUNTIME DESTINATION bin COMPONENT Runtime
    LIBRARY DESTINATION bin COMPONENT Runtime
    ARCHIVE DESTINATION lib COMPONENT Development
  )
endif()

#Spectroscopy ui executable
if( ${Build_Spectroscopy} )

  qt5_wrap_ui( SpectroscopyUI_H Spectroscopy.ui )
  qt5_wrap_cpp( spectroscopy_source_files_moc SpectroscopyUI.h )

  set( spectroscopy_source_files
    SpectroscopyUIDriver.cxx
    SpectroscopyUI.cxx
  )

  add_executable( SpectroscopyUI
    ${spectroscopy_source_files}
    ${SpectroscopyUI_H}
    ${spectroscopy_source_files_moc}
  )

  target_link_libraries( SpectroscopyUI PUBLIC
    ${ITK_LIBRARIES}
    ${IntersonArraySDKCxx_LIBRARIES}
    Qt5::Widgets
  )

  install( TARGETS SpectroscopyUI
    RUNTIME DESTINATION bin COMPONENT Runtime
    LIBRARY DESTINATION bin COMPONENT Runtime
    ARCHIVE DESTINATION lib COMPONENT Development
  )


  qt5_wrap_ui( SpectroscopyBModeUI_H SpectroscopyBMode.ui )
  qt5_wrap_cpp( spectroscopyBMode_source_files_moc SpectroscopyBModeUI.h )

  set( spectroscopyBMode_source_files
    SpectroscopyBModeUIDriver.cxx
    SpectroscopyBModeUI.cxx
  )

  add_executable( SpectroscopyBModeUI
    ${spectroscopyBMode_source_files}
    ${SpectroscopyBModeUI_H}
    ${spectroscopyBMode_source_files_moc}
  )

  target_link_libraries( SpectroscopyBModeUI PUBLIC
    ${ITK_LIBRARIES}
    ${IntersonArraySDKCxx_LIBRARIES}
    Qt5::Widgets
  )

  install( TARGETS SpectroscopyBModeUI
    RUNTIME DESTINATION bin COMPONENT Runtime
    LIBRARY DESTINATION bin COMPONENT Runtime
    ARCHIVE DESTINATION lib COMPONENT Development
  )

endif()


#PTX ui executable
if( ${Build_PTX} )

  #qt5_wrap_ui( PTXUI_H PTX.ui )
  qt5_wrap_cpp( ptx_source_files_moc1 PTXUISaveDialog.h )
  qt5_wrap_cpp( ptx_source_files_moc2 PTXUI.h )

  set( ptx_source_files
    PTXPatientData.h
    PTXUISaveDialog.cxx
    PTXDriver.cxx
    PTXUI.cxx
  )

  add_executable( PTXUI 
    ${ptx_source_files}
   # ${PTXUI_H}
    ${ptx_source_files_moc1}
    ${ptx_source_files_moc2}
  )

  target_link_libraries( PTXUI PUBLIC
    ${ITK_LIBRARIES}
    ${IntersonArraySDKCxx_LIBRARIES}
    Qt5::Widgets
  )

  install( TARGETS PTXUI 
    RUNTIME DESTINATION bin COMPONENT Runtime
    LIBRARY DESTINATION bin COMPONENT Runtime
    ARCHIVE DESTINATION lib COMPONENT Development
  )

endif()






#CPack setup

include( InstallRequiredSystemLibraries )

set( CPACK_MONOLITHIC_INSTALL ON )
set( CPACK_PACKAGE_VENDOR "Kitware, Inc." )
set( CPACK_GENERATOR "ZIP" )
set( CPACK_PACKAGE_VERSION "0.1.4" )
set( CPACK_PACKAGE_VERSION_MAJOR "0" )
set( CPACK_PACKAGE_VERSION_MINOR "1" )
set( CPACK_PACKAGE_VERSION_PATCH "4" )

set( CPACK_INSTALL_CMAKE_PROJECTS "${IntersonArraySDKCxx_DIR};IntersonArraySDKCxx;Runtime;/" )
set( CPACK_INSTALL_CMAKE_PROJECTS "${CPACK_INSTALL_CMAKE_PROJECTS};${CMAKE_BINARY_DIR};${PROJECT_NAME};ALL;/" )

# Get location of windeployqt.exe based on uic.exe location
get_target_property( uic_location Qt5::uic IMPORTED_LOCATION )
get_filename_component( _dir ${uic_location} DIRECTORY )
set( windeployqt "${_dir}/windeployqt.exe" )
if( NOT EXISTS ${windeployqt} )
  message( FATAL_ERROR "Failed to locate windeployqt executable: [${windeployqt}]" )
endif()
install( CODE "execute_process(COMMAND \"${windeployqt}\" \"\${CMAKE_INSTALL_PREFIX}/bin/OpticNerveUI.exe\")" )
#install( CODE "execute_process(COMMAND \"${windeployqt}\" \"\${CMAKE_INSTALL_PREFIX}/bin/PTXUI.exe\")" )

include( CPack )
