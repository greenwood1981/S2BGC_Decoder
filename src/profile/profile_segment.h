#ifndef PROFILE_SEGMENT_H
#define PROFILE_SEGMENT_H

#include <vector>
using std::vector;
#include <string>
using std::string;
#include <cstdint>

class profile_segment {
  void Unpack_1p0(bool autogain);    // Unpack using 1st difference algorithm
  void Unpack_2p0();                 // Unpack using 2nd difference algorithm
  void Unpack_BGC_2d(bool autogain); // New BGC unpack algorithm
  void Unpack_Integer(int s);
public:
	profile_segment(vector<uint8_t> d, string n, double g, double o, int as);

	vector<uint8_t> data;
	vector<long int> raw_counts;
	string profile;
	string name;
	unsigned short index, id;
	unsigned short sensor_id;     // ID to identify type of sensor family
	unsigned short pro;           // profile code (header byte #4) how profile is averaged and acquired
	unsigned short message_index; // profile sub-segment ID (header byte #5 2nd nibble); used to sort segments
	unsigned int total_scans;
	unsigned int unpk,nbyte,npts;
	unsigned int autogain_scale;  // for use with auto-gain profiles; auto-gain transmits a gain/offset for each segment. The gain is multiplied by this
	double gain,offset;           // gain/offset used to convert raw counts to SI units (as defined in config.json)
	bool operator<(const profile_segment &rhs) const; // used to sort segments first by type, then by message_index
	unsigned long length() { return index + raw_counts.size(); }
};
#endif
