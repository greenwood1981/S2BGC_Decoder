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
	unsigned int i;     // temporary indexing variable
	unsigned int nbyte; // Number of bytes for this pump packet

	s.phase = -9;
	s.packet = data[0] & 0x0F;
	version = (data[1] & 0xF0) >> 4;
	nbyte = ((data[1] & 0x0F) << 8) + data[2];

	int subseg = data[5] & 0x0F;

	i = 6;
	if (version == 1) {
		// firmware versions 10.2+ include start_time
		uint32_t secs = *reinterpret_cast<uint32_t *>(&data[i]);
		secs = __builtin_bswap32(secs);
		start_time = ptime(date(2000,1,1),seconds(secs));
		std::cout << "Pump start: " << start_time << std::endl;
		i += 4;
	}

	while (i < nbyte -1) {
		s.index = i;
		s.phase = (data[i] & 0xF0) >> 4;
		s.p_cnt = ((data[i] & 0x0F) << 16) + (data[i+1] << 8) + data[i+2];
		s.pres = s.p_cnt * (float)config["PRESSURE_GAIN"] - (float)config["PRESSURE_OFFSET"];
		s.pump_time = (data[i+3]<<8) + data[i+4];
		if (s.pump_time > 0x8000)
			s.pump_time -= 0x10000;

		s.v_cnt = (data[i+5] << 8) + data[i+6];
		s.volt = s.v_cnt * (float)config["VOLTAGE_GAIN"] - (float)config["VOLTAGE_OFFSET"];
		s.c_cnt = (data[i+7] << 8) + data[i+8];
		s.curr = s.c_cnt * (float)config["CURRENT_GAIN"] - (float)config["CURRENT_OFFSET"];
		s.vac_strt = data[i+9];
		s.vac_end = data[i+10];
		std::cout << s.pres << " " << s.pump_time << " " << s.volt << " " << s.curr << " " << s.vac_strt << " " << s.vac_end << std::endl;

		i += 11;

		switch (version) {
			case 1:
				// firmware versions 10.2+ samples include elapsed time
				s.t_cnt = (data[i]<<16) + (data[i+1]<<8) + data[i+2];
				s.time = start_time + seconds(s.t_cnt); // compute scan time
				std::cout << s.t_cnt << " " << s.time << std::endl;
				i+=3;
				break;
		}
		Scan.push_back(s);
	}

	std::sort(Scan.begin(),Scan.end());
	log(std::format("Packet[{:2X}] Pump Data ({:d}) format: {:d}",s.packet,subseg,version) );
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
