#ifndef BIT_H
#define BIT_H

#include <string>
#include <vector>
#include <cstdint>

class BIT {
public:
  bool received;
  unsigned short nQueued;
  int status;
  float pressure;                // units?
  float cpu_voltage;             // raw counts 0.01V
  float pump_voltage_prior;      // no_load (raw counts 0.01V)
  float pump_voltage_after;      // no load (raw counts 0.01V)
  int pump_current;              // (raw counts 1mA)
  unsigned short pump_time;      // seconds
  unsigned short pump_oil_prior; // oil counts before pumping
  unsigned short pump_oil_after; // oil counts after pumping
  float vacuum_prior;            // pcase vacuum before pumping (raw counts 0.01 inHg)
  float vacuum_after;            // pcase vacuum after pumping (raw counts 0.01 inHg)
  unsigned short valve_open;     // # of tries needed to open the valve
  unsigned short valve_close;    // # of tries needed to close the valve
  unsigned short interrupt_id;   // ID of last interrupt
  std::string SBE_response;      // 30 byte string from the SBE reply to "PTS"
  float cpu_temp;                // temperature of CPU: degC (CPU temp/8.0 - 2.0)
  unsigned short RH;             // relative humidity [%]
  BIT() : received(0) {};
  void parse( std::vector<uint8_t> d );
};

class BIT_Beacon {
public:
  bool received;
  unsigned short nQueued;   // # of data blocks queued for this dive
  unsigned short nTries;    // # tries to connect during last surfacing
  unsigned short parXstat;  // parse_X_reply status in the last surf session
  unsigned short SBDIstat;  // ATSBD return status in last surface session
  unsigned short SBDsecs;   // seconds taken in sending last SBD message
  float cpu_voltage;        // cpu battery voltage  (raw counts 0.01V)
  float pump_voltage;       // pump battery voltage (raw counts 0.01V)
  float vacuum_now;         // pcase vacuum at start of abort-cycle transmit ( raw counts 0.01 inHg)
  float vacuum_abort;       // pcase vacuum entering the abort cycle ( raw counts 0.01inHg)
  unsigned short ISRID;     // ID of the last interrupt
  unsigned short abortFlag; // abort flag that caused the abort
  float CPUtemp;            // CPU temperature; degC = counts/8.0 - 2.0
  unsigned short RH;        // relative humidity [%]
  BIT_Beacon() : received(0) {};
  void parse( std::vector<uint8_t> d);
};

class Science {
public:
  Science() : received(0),jumpCtr(0) {};
  void Parse( std::vector<uint8_t> d );
  bool received;
  float version;
  unsigned short files;
  unsigned short nWritten;
  unsigned short mbFree;
  unsigned short ctdParseErr;
  unsigned short jumpCtr;
};

#endif
