#include "pump.h"
#include <algorithm>
#include <iostream>
#include <iomanip>

#include <format>
#include "../output/write_log.h"

#include "../json/json.hpp"
using json = nlohmann::ordered_json;
extern json config;

void Pump_Data::Decode(std::vector<uint8_t> &data) {
	received = 1;
	PumpTimeSeries s;
	unsigned int tmp, // temp variable used to discard unused fields
		nbyte,        // Number of bytes for this pump packet
		nb_per_scan;  // Number of bytes per pump scan
	int p_form;
	s.phase = -9;
	s.packet = data[0] & 0x0F;
	p_form = (data[1] & 0xF0) >> 4;
	int subseg = data[5] & 0x0F;
	nbyte = ((data[1] & 0x0F) << 8) + data[2];

	for( unsigned int i = 6; i < nbyte -1; i+=11) {
		s.index = i;
		s.phase = (data[i] & 0xF0) >> 4;
		s.p_cnt = ((data[i] & 0x0F) << 16) + (data[i+1] << 8) + data[i+2];
		s.pres = s.p_cnt * (float)config["PRESSURE_GAIN"] - (float)config["PRESSURE_OFFSET"];
		s.pump_time = (data[i+3] << 8) + data[i+4];
		if (s.pump_time > 0x8000)
			s.pump_time -= 0x10000;
		s.v_cnt = (data[i+5] << 8) + data[i+6];
		s.volt = s.v_cnt * (float)config["VOLTAGE_GAIN"] - (float)config["VOLTAGE_OFFSET"];
		s.c_cnt = (data[i+7] << 8) + data[i+8];
		s.curr = s.c_cnt * (float)config["CURRENT_GAIN"] - (float)config["CURRENT_OFFSET"];
		s.vac_strt = data[i+9];
		s.vac_end = data[i+10];
		Scan.push_back(s);
	}

	std::cout << "Packet[" << std::setw(2) << s.packet << "] Pump Data (" << subseg << ") format: " << p_form << std::endl;
	log(std::format("Packet[{:2X}] Pump Data ({:d}) format: {:d}",s.packet,subseg,p_form) );

	std::sort(Scan.begin(),Scan.end());
}

std::ostream & operator << ( std::ostream &os, Pump_Data &p ) {
	os << "# Pump Time Series" << std::endl;
	os << std::setfill(' ');
	for (auto s : p.Scan) {
		os << std::setw(4) << s.pump_time << " ";
		os << std::setw(8) << std::setprecision(3) << s.pres << " ";
		os << std::setw(4) << std::setprecision(0) << s.curr << " ";
		os << std::setw(4) << std::setprecision(0) << s.volt << " ";
		os << std::setw(3) << s.vac_strt << " " << std::setw(3) << s.vac_end << " ";
		os << std::setw(2) << s.phase << std::endl;
	}
	return os;
}
