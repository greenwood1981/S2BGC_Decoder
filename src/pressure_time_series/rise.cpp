#include "rise.h"

#include <iostream>
using std::cout;
using std::endl;

#include <format>
#include "../output/write_log.h"

#include "../json/json.hpp"
using json = nlohmann::ordered_json;
extern json config;

std::ostream & operator << ( std::ostream &os, Rise_Data &r) {
    os << "# Rise Time Series" << endl;
    for( auto s : r.Scan ) {
        os << s.time << " " << std::setw(5) << s.p_cnt << " " << std::fixed << std::setprecision(2) << std::setw(7) << s.pres << endl;
    }
	return os;
}

void Rise_Data::Decode( std::vector<uint8_t> data) {
	received = 1;
	int p_cnt, t_cnt;

	PressureTimeSeries s;
	unsigned int tmp,   // temp variable used to discard unused fields
		p_form,         // Packet format; 0: 4 bytes, 1: 5 bytes
		nbyte,          // Number of bytes for this packet
		nb_per_scan;    // Number of bytes per Fall Scan

	s.packet = data[0] & 0x0F;
	p_form = (data[1] & 0xF0) >> 4;
	nbyte = ((data[1] & 0x0F) << 8) + data[2];
    int subseg = data[5] & 0x0F;
	uint32_t secs = *reinterpret_cast<uint32_t *>(&data[6]);
	secs = __builtin_bswap32(secs);
	start_time = ptime(date(2000,1,1),seconds(secs));

    for( unsigned int i = 10; i < nbyte-1; i+= 5) {
        s.index = i;

        // parse timestamp (seconds since start_time defined above)
        t_cnt = ( data[i] << 8) + data[i+1];
        s.time = start_time + seconds(t_cnt);

        // counts to dbar
        s.phase = (data[i+2] & 0xF0) >> 4;
        s.p_cnt = ((data[i+2] & 0x0F) << 16) + (data[i+3] << 8) + data[i+4];
        s.pres = s.p_cnt * (float)config["PRESSURE_GAIN"] - (float)config["PRESSURE_OFFSET"];
        Scan.push_back(s);
    }
    sort();
	//std::cout << "Packet[" << std::setw(2) << s.packet << "] Rise Data (" << subseg << ") format: " << p_form << std::endl;
    log(std::format("Packet[{:2X}] Rise Data ({:d}) format: {:d}",s.packet,subseg,p_form) );

}

/* 2018/05/18 BG Fall scans are uniquely identified using ptime; PressureTimeSeries operators '<' + '==' overloaded to compare ptime
// Remove duplicates by: 1) sorting 2) removing adjacent unique scans 3) erasing duplicates that are now at end of vector
Fall.Scan[] will now be sorted with duplicates removed */
void Rise_Data::sort() {
    if (Scan.size() > 0) {
        std::sort(Scan.begin(),Scan.end());
        Scan.erase(std::unique(Scan.begin(),Scan.end()),Scan.end());
    }
}
