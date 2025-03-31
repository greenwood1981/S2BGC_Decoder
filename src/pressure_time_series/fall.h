#ifndef FALL_H
#define FALL_H
#include "pressure_time_series.h"
#include <fstream>

#include <boost/date_time.hpp>
using boost::posix_time::ptime;
using boost::gregorian::date;
using boost::posix_time::seconds;

class Fall_Data {
public:
	Fall_Data() {}
	void Decode( std::vector<uint8_t> d);
	std::vector <PressureTimeSeries> Scan; // Descent pressure time series scans
	void sort();
	int received;            // Flag set if all the data has been received
	ptime start_time;        // Start time for time series
	float pres_start;        // DP0 pressure before start to descend
	float pres_end;          // [DP1] pressure at end of descent
	unsigned int total_time; // from open valve to end of sink [seconds]
	float speed;             // Average descent rate while sinking [cm/s]
	friend std::ostream & operator << ( std::ostream &os, Fall_Data &s );
};
#endif
