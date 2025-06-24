#ifndef _HEXFILE_H
#define _HEXFILE_H

#include "message.h"
#include "../profile/profile.h"
#include "../pump/pump.h"
#include "../pressure_time_series/rise.h"
#include "../pressure_time_series/fall.h"
#include "../gps/gps.h"
#include "../bist/bist.h"
#include "../Engineering/Engineering_Data.h"
#include "../BIT/BIT.h"
#include "../BIT/SCI_parameter.h"
#include "../argo_mission/argo.h"

class hexfile {
  std::map<string,profile> prof;
  Fall_Data fall;
  Rise_Data rise;
  Pump_Data pump;
  ctd_bist bist_ctd;
  do_bist bist_do;
  eco_bist bist_eco;
  ph_bist bist_ph;
  ocr_bist bist_ocr;
  no3_bist bist_no3;
  Engineering_Data eng_data;
  mission miss;
  SCI_parameter sensor;
  Science science;
  BIT bit;
  BIT_Beacon beacon;
  Argo_Mission argo;
  vector<GPS> gps;
  std::string filepath;
  std::string filename;
  std::ifstream fin;
  std::list<packet> packets;                // as messages are parsed, packets will be added here and sorted by priority
  std::map<unsigned int, message> messages; // use map to save messages using PID as key; duplicates ignored
  std::string upload_command;
public:
  hexfile( std::string f );
  void print();
  void Decode();
  void write_List();
  uint16_t sn;
  int16_t cycle;
  std::string jsonpath; // full path of json output file; assigned by write_JSON()
  friend std::ostream & operator << ( std::ostream &os, hexfile &h );
  void write_JSON();
  void archive(); // move processed hex file into float subdirectory, create float directory skeleton if needed
};
#endif
