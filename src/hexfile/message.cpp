#include "message.h"
#include <boost/date_time/gregorian/gregorian.hpp>
#include <cstring>

std::ostream & operator << ( std::ostream &os, message &m ) {
	std::cout << "SN    : " << m.SN << std::endl;
	std::cout << "cycle : " << m.cycle << std::endl;
	std::cout << "size  : " << m.size << std::endl;
	std::cout << "PID   : " << m.PID << std::endl;
	std::cout << "Date  : " << m.yy << "/" << m.mm << "/" << m.dd << " " << m.HH << ":" << m.MM << ":" << m.SS << std::endl;
	std::cout << "data = [";
	for (unsigned int n = 0; n < m.data.size(); n++)
		std::cout << (unsigned int)m.data[n] << ",";
	std::cout << "]" << std::endl;

	return os;
}

std::istream & operator >> ( std::istream &is, message &m ) {
	m.data.clear();
	m.packets.clear();
	std::string line,tmp;
	char delim;

	while (is.good() && is.peek() != '#') { // ignore white space, erroneous lines before next message (identified with '#')
        is.get();
    }

	if (is.good() && is.peek() == '#') {
		// ignore first line "SN Cycle ..."
		std::getline(is,line);

        if (is.peek() == 'S')
          m.type = "SBD";
        if (is.peek() == 'R')
          m.type = "RUDICS";
		// Parse SN,cycle,size,PID,Date from 2nd line
		is >> tmp >> m.SN >> delim >>  m.cycle >> delim >> m.size >> delim;
		is >> tmp; // IMEI
		is >> m.momsn >> delim; // MOMSN
		//log(std::string(" momsn ") + std::to_string(m.momsn) );
		is >> tmp; // MTMSN
		is >> m.PID >> delim; // Message ID (PID)
		is >> m.yy >> delim >> m.mm >> delim >> m.dd >> delim; // Year,Month,Day
		is >> m.HH >> delim >> m.MM >> delim >> m.SS >> delim;  // Hour,Minute,Second
		m.time = boost::posix_time::ptime(boost::gregorian::date(m.yy,m.mm,m.dd),boost::posix_time::time_duration(m.HH,m.MM,m.SS));
		std::getline(is,line); // ignore rest of line, CRLF

		// Read in hex string
		m.data.resize(m.size);
		unsigned int v;
		for( unsigned int n = 0; n < m.size; n++ ) {
			is >> std::hex >> v;
			m.data[n] = v;
		}
		is >> std::dec;

		std::getline(is,line); // ignore rest of line

		// read packets from hex data
		packet p;
		int n = 8; // ignore first 8 bytes; SN, etc..
		uint8_t *d = m.data.data();
		while (n < m.size - 7) {
			packet *p = new packet;
			memcpy(&(p->header),&d[n],6);
			p->size = p->header.nDat1 * 256 + p->header.nDat2;
			std::copy(m.data.begin()+n,m.data.begin()+n+p->size,std::back_inserter(p->data)); // copy data to packet object
			m.packets.push_back(*p);
			n += p->size;
			if (p->size == 0) {
				log(std::string("-- Message Error [momsn ") + std::to_string(m.momsn));
				break;
			}
		}
	}
	return is;
}
