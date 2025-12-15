#ifndef _PUMP_H
#define _PUMP_H

#include <sstream>
#include <tuple>
#include <map>
#include <vector>
#include <cstdint>
#include <boost/date_time.hpp>
using boost::posix_time::ptime;
using boost::gregorian::date;
using boost::posix_time::seconds;

// Scan during pumping
class PumpTimeSeries {
public:
	unsigned int p_cnt;    // raw pressure counts
	unsigned int c_cnt;    // raw current counts
	unsigned int v_cnt;    // raw voltage counts
	unsigned int t_cnt;    // seconds since start of dive
	double pres;           // Pressure [dBar]
	float volt;            // Pump voltage [Volts]
	float curr;            // Pump Current [A]
	int pump_time;         // Time that pump ran for this event [s]
	ptime time;
	unsigned int vac_strt; // Vacuum at start of pumping
	unsigned int vac_end;  // Vacuum at end of pumping
	unsigned int phase;    // phase of the sample
	unsigned int packet;   // Packet number                - used for ordering time series
	unsigned int index;    // index of pump scan in packet - used for ordering time series

	// Sort first by packet number, next by scan index
	bool operator<(const PumpTimeSeries &rhs) const { return std::tie(packet,index) < std::tie(rhs.packet,rhs.index); }
	bool operator==(const PumpTimeSeries &rhs) const { return std::tie(packet,index) == std::tie(rhs.packet,rhs.index); }
};

// Pump time series
class Pump_Data {
public:
	Pump_Data() {}
	void Decode(std::vector<uint8_t> &data);
    friend std::ostream & operator << ( std::ostream &os, Pump_Data &p );

	int received;                        // Flag set if all of the data has been received
	std::vector <PumpTimeSeries> Scan;   // Time series of pump scans
	std::map<int,std::string> dive_phase;
	ptime start_time;                    // Start time of for time series
	unsigned int version;                // packet version [0:original S2BGC, 1: firmware v10.2+]
};

#endif
