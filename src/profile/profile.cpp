#include "profile.h"
#include <iostream>
using std::cout;
using std::endl;

#include "../output/write_log.h"
#include <format>

#include "../json/json.hpp"
using json = nlohmann::ordered_json;
extern json config;

unsigned int profile::segment_size() {
	unsigned int max = 0;
	for( auto &segment : segments ) {
		if (segment.length() > max)
			max = segment.length();
	}
	return max;
}

void profile::print_stats() {
	if (segment_size()) {
		printf("segments:\n");
	    for(auto &s : segments)
			std::cout << s.name << "[" << s.message_index << "]" << ": " << s.index << " - " << s.index + s.raw_counts.size() << std::endl;
		//printf("  [%d] [%s] 0x%02X %lu\n",s.message_index,s.type.c_str(),s.id,s.raw_counts.size());
		printf("channels:\n");
		for(auto &c : channel)
			printf("%s [size=%lu]\n",c.first.c_str(),c.second.size());
		printf("\n");
	}
}

// Convert profile segments matching channel_name from raw counts to SI units. Save ordered data to map<string,vector<float> channel
void profile::Conv_Cnts2SI() {
	float val;

	// Initialize channels with correct size
	vector<float> blank(segment_size(),-999);
	for (auto &s : segments ) {
		channel[s.name] = blank; // Initialize channel with proper size filled with blank values '-999'
	}

	// Convert segment data to SI and add to corresponding channel.
	// Also determine the first index (min_index) that has been received.
	int min_index = 9999;
	for( auto &s : segments ) {
		for( unsigned int i = 0; i < s.raw_counts.size(); i++ ) {
			if (s.raw_counts[i] == -999)
				val = -999;
			else
				val = (double)s.raw_counts[i] / s.gain - s.offset;

			channel[s.name][i+s.index] = val; // assign SI value to channel at the appropriate position

			if (i+s.index< min_index)
				min_index = i+s.index;
		}
	}

	// Use min_index to remove any empty scans at beginning.
	size = segment_size(); // profile size, size = segment_size if first index = 0

	if (min_index > 0 && min_index != 9999) {
		for( auto &[key,c] : channel) {
			c.erase(c.begin(),c.begin()+min_index);
		}
		size = size - min_index; // if first index is greater than zero, update profile size (size)
	}
}

profile::profile() {
	//cout << "Create profile object" << endl;
}

// add profile segment; sorting and conversion handled separately
void profile::insert(std::vector<uint8_t> data) {
	int sensor_id = data[0];
	int data_id = data[3] & 0x0F;
	int segment = data[5]; // sub-segment index
	int pro = data[4];
	char profile_key[7];
	int unpk = (data[3] & 0xF0) >> 4;

	sprintf(profile_key,"%02X%02X%02X",sensor_id,data_id,pro);

	auto p = config["packets"][profile_key];                 // packet meta
	auto prof = config["prof"][ p["profile"].get<std::string>() ][ p["name"].get<std::string>() ]; // profile meta

	//channel[ p["name"] ]; // initialize channel if it doesn't already exist

	int auto_offset = 0;
	if (p.count("auto_offset") ) {
		auto_offset = p["auto_offset"];
	}
	profile_segment s(data,string(p["name"]),float(prof["gain"]),float(prof["offset"]),auto_offset);
	segments.insert(s);

	log( std::format("Packet[{:2X}] {:s} {:s} ({:d}) unpk {:d} size {:d}",sensor_id,string(p["profile"]),string(p["name"]),segment,unpk,data.size()) );
}
