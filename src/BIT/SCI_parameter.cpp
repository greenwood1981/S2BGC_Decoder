#include "SCI_parameter.h"
#include <iostream>

void SCI_parameter::Parse(std::vector<uint8_t> data) {
  // nSenVar (# sensor variables) nRegVar (# depth regions) nCoefVar (# sensor coefficients)
  unsigned short nSenVar  = data[ 6]*256 + data[ 7]; // Sensor parameters [bytes]; little-endian
  unsigned short nRegVar  = data[ 8]*256 + data[ 9]; // Region parameters [bytes]; little-endian
  unsigned short nCoefVar = data[10]*256 + data[11]; // coefficient params [bytes]; little-endian

  std::cout << "nSenVar: " << nSenVar << ", nRegVar: " << nRegVar << ", nCoefVar: " << nCoefVar << std::endl;

  //vector<string> sensor_name  = { "ctd", "dox", "pH", "eco", "ocr", "NO3" };                  // sensor names parsed in this order
  //vector<string> sensor_param = { "Enabled", "EnDrift", "DecDrift", "dataType", "packType" }; // each sensor has these parameters
  //vector<string> region_name  = { "region1","region2","region3","region4","region5" };        // five regions defined for each sensor
  //vector<string> region_param = { "zMin", "zMax", "dz", "scanT" };                            // region parameters for each sensor, for each region [-1 not sampled]

  received = 1;
  unsigned int n = 12; // parameters begin at byte 12
  signed short val;

  // Sensor parameters
  for( int i = 0; i < 6; i++ ) {
    sensor[i].Enabled = data[n+1]*256 + data[n];
    sensor[i].EnDrift = data[n+3]*256 + data[n+2];
    sensor[i].DecDrift = data[n+5]*256 + data[n+4];
    sensor[i].dataType = data[n+7]*256 + data[n+6];
    sensor[i].packType = data[n+9]*256 + data[n+8];
    n += 10;
  }

  // Region parameters
  for( int i = 0; i < 6; i++ ) {
    for( int j = 0; j < 5; j++ ) {
      sensor[i].region[j].zMin  = data[n+1]*256 + data[n];
      sensor[i].region[j].zMax  = data[n+3]*256 + data[n+2];
      sensor[i].region[j].dz    = data[n+5]*256 + data[n+4];
      sensor[i].region[j].scanT = data[n+7]*256 + data[n+6];
      n += 8;
    }
  }

  // Coefficient parameters
  for( int i = 0; i < 6; i++ ) {
      sensor[i].offset = data[n+1]*256 + data[n];
      sensor[i].gain = data[n+3]*256 + data[n+2];
      n += 4;
  }
}
