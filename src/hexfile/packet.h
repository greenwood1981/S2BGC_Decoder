#ifndef PACKET_H
#define PACKET_H

#include <iostream>
#include <vector>
#include <fstream>
#include <cstdint>

struct __attribute__((__packed__)) packet_header {
public:
  uint8_t sensorID;
  uint8_t nDat1 : 4;
  uint8_t ver : 4;
  uint8_t nDat2 : 8;
  uint8_t data_ID : 4;
  uint8_t packType : 4;
  uint8_t proDir : 4;
  uint8_t proType : 4;
  uint8_t subBlk : 4;
  uint8_t pSame : 4;
};

class packet {
public:
  packet_header header;
  uint16_t size;
  std::vector<uint8_t> data;
  packet(){}
  friend std::istream & operator >> ( std::istream &is, packet &p );
  friend std::ostream & operator << ( std::ostream &os, packet &p );
  void decode() {}                       // purely virtual; various classes derive from packet including GPS.. Each one has a decode() function
  bool operator<(const packet &p) const; // overloaded operator<() to be able to sort packets by priority
  int priority() const;                  // define packet priorities; Argo_Mission packet must be processed first
};

#endif
