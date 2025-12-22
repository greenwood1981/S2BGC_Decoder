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
