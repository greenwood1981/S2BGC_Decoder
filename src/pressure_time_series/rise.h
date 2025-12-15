#ifndef RISE_H
#define RISE_H
#include "pressure_time_series.h"
#include <boost/date_time.hpp>
using boost::posix_time::ptime;
using boost::gregorian::date;
using boost::posix_time::seconds;

class Rise_Data {
public:
	Rise_Data() {}
	void Decode( std::vector<uint8_t> d);
	std::vector <PressureTimeSeries> Scan; // Descent pressure time series scans
	void sort();
	int received;            // Flag set if all the data has been received
	ptime start_time;        // Start time for time series
	friend std::ostream & operator << ( std::ostream &os, Rise_Data &r );

};
#endif
