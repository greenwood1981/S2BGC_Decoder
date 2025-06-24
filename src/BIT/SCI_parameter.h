#ifndef SCI_PARAMETER_H
#define SCI_PARAMETER_H

#include <cstdint>
#include <vector>
using std::vector;
#include <string>
using std::string;

class region_parameter {
public:
  short zMin;
  short zMax;
  short dz;
  short scanT;
};

class sensor_parameter {
public:
  short Enabled;      // Sensor sampling is enabled
  short EnDrift;      // percentage of drift to sample
  short DecDrift;     // decimation for this sensor during drift
  short dataType;     // sensor specific data type (columns to telemeter?)
  short packType;     // data packing type (first diff, 2nd diff, etc..)
  short gain, offset; // sensor gain and offset definitions
  region_parameter region[5];
};

class SCI_parameter {
public:
  SCI_parameter() : received(0) {} // default constructor set received to zero

  void Parse( std::vector<uint8_t> d );
  sensor_parameter sensor[6];

  bool received; // SCI_parameter packet received (typically during cycle -1)

  short nSenVar;  // Sensor parameters [bytes]; little-endian
  short nRegVar;  // Region parameters [bytes]; little-endian
  short nCoefVar; // coefficient params [bytes]; little-endian
};

#endif

/*

  // nSenVar (# sensor variables) nRegVar (# depth regions) nCoefVar (# sensor coefficients)
  unsigned short nSenVar  = data[ 6]*256 + data[ 7]; // Sensor parameters [bytes]; little-endian
  unsigned short nRegVar  = data[ 8]*256 + data[ 9]; // Region parameters [bytes]; little-endian
  unsigned short nCoefVar = data[10]*256 + data[11]; // coefficient params [bytes]; little-endian
  signed short val;

  std::cout << "nSenVar: " << nSenVar << ", nRegVar: " << nRegVar << ", nCoefVar: " << nCoefVar << std::endl;

  vector<string> sensor_name  = { "ctd", "dox", "pH", "eco", "ocr", "NO3" };                  // sensor names parsed in this order
  vector<string> sensor_param = { "Enabled", "EnDrift", "DecDrift", "dataType", "packType" }; // each sensor has these parameters
  vector<string> region_name  = { "region1","region2","region3","region4","region5" };        // five regions defined for each sensor
  vector<string> region_param = { "zMin", "zMax", "dz", "scanT" };                            // region parameters for each sensor, for each region [-1 not sampled]

  unsigned int n = 12; // parameters begin at byte 12

  // Sensor parameters
  for( int i = 0; i < 6; i++ ) {
    for( int j = 0; j < 5; j++ ) {
      val = data[n+1]*256 + data[n];
      n += 2;
      std::cout << "[" << n << "] " << sensor_name[i] << " " << sensor_param[j] << ": " << val << std::endl;
    }
  }

  // Region parameters
  for( int i = 0; i < 6; i++ ) {
    for( int j = 0; j < 5; j++ ) {
      for( int k = 0; k < 4; k++ ) {
        val = data[n+1]*256 + data[n];
        n += 2;
        std::cout << "[" << n << "] " << sensor_name[i] << " " << region_name[j] << " " << region_param[k] << ": " << val << std::endl;
      }
    }
  }

  // Coefficient parameters
  for( int i = 0; i < 6; i++ ) {
      val = data[n+1]*256 + data[n];
      n += 2;
      std::cout << "[" << n << "] " << sensor_name[i] << " offset: " << val << std::endl;
      val = data[n+1]*256 + data[n];
      n += 2;
      std::cout << "[" << n << "] " << sensor_name[i] << " gain: " << val << std::endl;
  }
}
*/
