#ifndef PROFILE_H
#define PROFILE_H
#include "profile_segment.h"
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <vector>
using std::vector;
#include <map>
using std::map;
#include <set>
using std::set;
#include <string>
using std::string;
//#include <cmath>

// Profile class stores profile data for one or more profiles. Each profile is subdivided into one or more profile_segments.
// Profile segments are added to class using insert() member function. Profile class handles ordering of profile segments,
// conversion to SI units, min/max check, and padding of each profile to ensure all channels are the same length.
// Profile data is returned using overloaded [] brackets, and is returned as padded, min/max checked SI unit vector<float>.
class profile {
public:
	profile();
	profile(std::vector<string> channels) { for (auto ch : channels) channel[ch]; }
	map<string,vector<float> > channel;
	set<profile_segment> segments; // segments sorted by S_id, then by message_index. set data structure will not allow duplicates
	void print_stats();

	// add profile segment; sorting and conversion handled separately
	void insert(std::vector<uint8_t> data);
	void clear() { segments.clear(); for (auto &c : channel) c.second.clear(); }

	// overload [] to return vector<float> channel data. Note that Conv_Cnts2SI must be called prior to using this.
	vector<float> & operator[]( string type ) { return channel[type]; }
	//class CTD operator()(unsigned int index); // return indexed PTS data as CTD object

	// Convert profile segments matching channel_name from raw counts to SI units.
	// Save ordered data to map<string,vector<float> channel
	void Conv_Cnts2SI();
	unsigned int compression() { return segments.begin()->unpk; } // returns sensor compression. Assumes each channel uses same compression
	unsigned int vars() { return channel.size(); } // return number of sensor channels
	unsigned int segment_size(); // returns size of longest profile segment; this is used to ensure that all columns ultimately have the same length
	unsigned int size; // size of the profile (all channels should have identical length
};

#endif
