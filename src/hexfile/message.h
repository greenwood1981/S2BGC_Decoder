#include <iostream>
#include <vector>
#include <boost/date_time.hpp>
#include "packet.h"
#include "../output/write_log.h"

class message {
public:
  uint8_t Err,Stat;
  std::string type;
  uint16_t PID;
  boost::posix_time::ptime time;
  uint16_t momsn;
  uint16_t yy,mm,dd,HH,MM,SS,SN,size;
  int16_t cycle;
  double lat,lon;
  int bytes;
  std::vector<packet> packets;
  std::vector<uint8_t> data;

  message(){}
  friend std::istream & operator >> ( std::istream &is, message &m );
  friend std::ostream & operator << ( std::ostream &os, message &m );
};
