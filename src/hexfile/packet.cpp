#include "packet.h"

std::ostream & operator << ( std::ostream &os, packet &p ) {
    std::cout << "sensorID : " << (int)p.header.sensorID << std::endl;
    std::cout << "ver      : " << (int)p.header.ver << std::endl;
    std::cout << "nDatSize : " << (int)p.header.nDat1 * 256 + (int)p.header.nDat2 << std::endl;
    std::cout << "packType : " << (int)p.header.packType << std::endl;
    std::cout << "data_ID  : " << (int)p.header.data_ID << std::endl;
    std::cout << "proType  : " << (int)p.header.proType << std::endl;
    std::cout << "proDir   : " << (int)p.header.proDir << std::endl;
    std::cout << "pSame    : " << (int)p.header.pSame << std::endl;
    std::cout << "subBlk   : " << (int)p.header.subBlk << std::endl;
    return os;
}

std::istream & operator >> ( std::istream &is, packet &p ) {
    is.read( (char *)&p.header, sizeof(packet_header) );
    std::cout << p;
    return is;
}

// Define packet priorities
int packet::priority() const {
    // Argo Packet includes CTD gain,offsets; process before CTD profiles and engineering (drift averages)
	if (header.sensorID == 2)
		return 1;
    else
		return 2;
}

// Used to sort packets by priority
bool packet::operator<(const packet &rhs) const {
	return priority() < rhs.priority();
}
