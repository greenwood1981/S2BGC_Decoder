#include "profile.h"
#include <iostream>
using std::cout;
using std::endl;

#include "../output/write_log.h"
#include <format>

#include "../json/json.hpp"
using json = nlohmann::ordered_json;
extern json config;

unsigned long profile::size() {
	unsigned long max = 0;
	for( auto & [ch_name,ch_data] : channel ) {
		if (ch_data.size() > max)
			max = ch_data.size();
	}
	return max;
}

void profile::print_stats() {
	printf("segments:\n");
    for(auto &s : segments)
		std::cout << s.name << "[" << s.message_index << "]" << ": " << s.index << " - " << s.index + s.raw_counts.size() << std::endl;
	//printf("  [%d] [%s] 0x%02X %lu\n",s.message_index,s.type.c_str(),s.id,s.raw_counts.size());
	printf("channels:\n");
	for(auto &c : channel)
		printf("%s [size=%lu]\n",c.first.c_str(),c.second.size());
	printf("\n");
}

// Convert profile segments matching channel_name from raw counts to SI units. Save ordered data to map<string,vector<float> channel
void profile::Conv_Cnts2SI() {
	float val;

	// Convert segment data to SI, perform min/max check, and add to corresponding channel
	// segments are already ordered - see operator< overload
	for( auto &s : segments ) {
        //string segment_name = config["packets"][s.name]["name"];
		string segment_name = s.name;
		//cout << "Conv_Cnts2SI; " << segment_name << " size: " << size() << " pnum:" << s.message_index << " indx: " << s.index << " total scans:" << s.total_scans << " gain:" << s.gain << " offset:" << s.offset << std::endl;

		if ( channel.count(segment_name) ) {
			for( unsigned int i = 0; i < s.raw_counts.size(); i++ ) {
				//if ( s.raw_counts[i] != 0xffff ) // SI unit conversion
				val = (float)s.raw_counts[i] / s.gain - s.offset;// / gain - offset;
				//else
				//	val = fill;
				//if (val <= rmin || val >= rmax) // min+max check
				//	val = fill;
				channel[segment_name].push_back(val);
				//cout << segment_name << ": " << val << std::endl;
			}
		}
		else {
			cout << "conv_cnts2si cant find " << segment_name << std::endl;
		}
	}

	// Ensure all channels have same length
	for( auto & [ch_name,ch_data] : channel ) {
		ch_data.resize( size(), -999 ); // resize each channel to size(); length of the longest channel
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

	channel[ p["name"] ]; // initialize channel if it doesn't already exist

	profile_segment s(data,string(p["name"]),prof["gain"],prof["offset"]);
	segments.insert(s);
	log( std::format("Packet[{:2X}] {:s} {:s} ({:d}) unpk {:d} size {:d}",sensor_id,string(p["profile"]),string(p["name"]),segment,unpk,data.size()) );
    //std::cout << "Packet[" << sensor_id << "] " << string(p["profile"]) << " " << string(p["name"]) << " (" << segment << ") unpk " << unpk << ", size " << data.size() <<  std::endl;
	//std::cout << "+Inserting " << string(p["profile"]) << "-" << string(p["name"]) << "[" << segment << "] unpk = " << unpk << " size = " << data.size() << std::endl;
}
