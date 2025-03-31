#ifndef GPS_H
#define GPS_H

#include <boost/date_time.hpp>
using boost::posix_time::ptime;

class GPS {
public:
	GPS( std::vector<uint8_t> d );

	ptime gps_time;
    int received; // Flag set if all the data has been received
    int phase; // type of position [0 = position at end of verification dive; 1 = start; 2 = end; 3 = abort ]
    uint16_t week,wDay,hour,mins;
	int valid;    // 0: not valid, -2: west, +2: east
	int time2fix; // Time to obtain GPS fix [sec]
    int num_sat;  // Number of satellites for GPS fix
    int snr_min;  // Signal to Noise minimum for satellite signals
    int snr_mean; // Signal to Noise mean for satellite signals
    int snr_max;  // Signal to Noise maximum for satellite signals
    float hdop;   // HDOP horizontal dilution of precision (fix quality) for GPS positions 0-250
	int32_t lat,lon;
	float flat,flon;
	std::vector<uint8_t> data;

    friend std::ostream & operator << ( std::ostream &os, GPS &s );
};

#endif
