#ifndef PRESSURE_TIME_SERIES
#define PRESSURE_TIME_SERIES

#include <boost/date_time.hpp>
using boost::posix_time::ptime;

class PressureTimeSeries {
public:
    ptime time;
    double pres;
    unsigned int p_cnt;
    unsigned int phase;
    unsigned int index;
    unsigned int packet;
    // overload < and == in order to use std::sort() algorithm
    bool operator<( const PressureTimeSeries &right ) const { return time < right.time; }
    bool operator==( const PressureTimeSeries &right ) const { return time == right.time; }
};

#endif
