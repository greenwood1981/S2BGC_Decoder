#include "Engineering_Data.h"
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
#include <format>
#include "../output/write_log.h"

#include "../json/json.hpp"
using json = nlohmann::ordered_json;
extern json config;

extern std::string S2BGC_PATH;


#define PARAMETER_VALUE_WIDTH 8
#define PARAMETER_NAME_WIDTH  8
#define PARAMETER_UNIT_WIDTH  6

// Mission Parameters are sent along with human readable variable names
void mission::parse(std::vector<uint8_t> d) {
	int ptype = d[0] & 0xF;
	int pseg  = d[5];
    log( std::format("Packet[{:2X}] Mission ({:d})",ptype,pseg) );
	//std::cout << "Packet[" << std::setw(2) << ptype << "] Mission (" << pseg << ")" << std::endl;

	std::ifstream f(S2BGC_PATH + "/config/mission.json",std::ios::in);
	json params = json::parse(f);

	std::string pname,val,unit="",desc="";
	int n = 6;
	//std::cout << "Begin parameter parse; size=" << ((d[1]&0XFF)<<8)+d[2] << " subid=" << int(d[3]) << std::endl;

	// Read in parameters one at a time
	while ( d[n] != ';' ) {
		pname.clear();
		unit.clear();
		desc.clear();
		// Read in parameter name until '=' sign. Note that parameter name legth varies from packet to packet!
		while (d[n] != '=') {
	       	if (d[n] != 32 && d[n] != 10 && d[n] != 124) // ignore spaces, CR, '|'
				pname.push_back(d[n]);
			n++;
		}
		n++; // skip next character '='
        val = "";
        // Read in parameter value until '|' character. Ignore space characters.
		while (d[n] != '|') {
          if (d[n] != 32)
	        val.push_back(d[n]);
          n++;
		}
        while (d[n] == '|' || d[n] == 0 || d[n] == 10) {
			//std::cout << "ignoring '" << std::hex << std::setw(2) << std::setfill('0') << int(d[n]) << "." << std::endl << std::dec << std::setfill(' ');
	        n++; // skip next character '|'
		}
        //std::cout << std::setw(12) << pname << "," << val << std::endl;

		// Match mission parameter to known parameters as defined in config/mission.json
		if ( params.contains(pname) ) {
			unit = params[pname]["units"];
			desc = params[pname]["description"];
            //std::cout << unit << "," << desc << std::endl;
		}
		else {
			std::cout << "* Unknown mission parameter: " << pname << ", " << val << std::endl;
		}

		list.push_back({pname,unit,desc,val});
    }
    //std::cout << "End parse" << std::endl;
}


// Parses Engineering Diagnostic-Dive [0x0A], Profile-Mode [0x0B] and Engineering Beacon [0x0C]
// Requires a separate pfile.json that describes variable names, order, types, descriptions
// Only parameters sent. Use pfile.json to interpret parameter data
void Engineering_Data::parse_pfile(std::vector<uint8_t> d,std::string pfile) {
	int ptype = d[0] & 0xF;
	int pseg  = d[5];
    log( std::format("Packet[{:2X}] Engineering Data ({:d})",ptype,pseg) );
	//std::cout << "Packet[" << std::setw(2) << ptype << "] Engineering_Data (" << pseg << ")" << std::endl;

    std::ifstream f(pfile);
    json params = json::parse(f);

	int n = 6;
	double val;
	int prec;
	std::stringstream unit,vstr,pnamestr;

	pnamestr << std::setw(PARAMETER_NAME_WIDTH) << "\"Eng_ver\"";
	vstr << std::setw(PARAMETER_VALUE_WIDTH) << (uint16_t)d[4];
	unit << std::setw(PARAMETER_UNIT_WIDTH) << "\"1\"";
	list.push_back({pnamestr.str(),unit.str(),"Engineering Packet software version",vstr.str()});
	pnamestr.str("");
	vstr.str("");
	unit.str("");

	for(auto &[pname,patts] : params.items()) {
		prec = 0;
		unit.str("");
		unit << "\"";
		if (patts["type"] == "U8") {
			val = d[n];
			n++;
		}
		else if (patts["type"] == "U16") {
			val = (d[n]<<8) + d[n+1];
			n+=2;
		}
		else if (patts["type"] == "I16") {
			val = (d[n]<<8) + d[n+1];
			n+=2;
		}
		else if (patts["type"] == "U24") {
			val = (d[n]<<16) + (d[n+1]<<8) + d[n+2];
			n+=3;
		}
		else if (patts["type"] == "I24") {
			val = (d[n]<<16) + (d[n+1]<<8) + d[n+2];
			n+=3;
		}

		else {
			std::cout << "unknown parameter" << std::endl;
			val = (d[n]<<8) + d[n+1];
			n+=2;
		}
		if (patts.contains("scale")) {
			//std::cout << pname << " scale: " << patts["scale"] << std::endl;
			if (patts["scale"] == "pres")
				val /= double(config["prof"]["CTD Discrete"]["PRES"]["gain"]);
			else if (patts["scale"] == "temp")
				val /= double(config["prof"]["CTD Discrete"]["TEMP"]["gain"]);
			else if (patts["scale"] == "psal")
				val /= double(config["prof"]["CTD Discrete"]["PSAL"]["gain"]);
            else
				val *= (float)patts["scale"];
			prec = (int)patts["prec"]; // precision for output (list files and json); every parameter with scale needs "prec" defined
		}
		if (patts.contains("offset")) {
			//std::cout << pname << " offset: " << patts["offset"] << std::endl;
			if (patts["offset"] == "pres")
				val -= double(config["prof"]["CTD Discrete"]["PRES"]["offset"]);
			else if (patts["offset"] == "temp")
				val -= double(config["prof"]["CTD Discrete"]["TEMP"]["offset"]);
			else if (patts["offset"] == "psal")
				val -= double(config["prof"]["CTD Discrete"]["PSAL"]["offset"]);
            else
			val -= (float)patts["offset"];
		}
		if (patts.contains("units"))
			unit << (std::string)patts["units"] << "\"";
		else
			unit << "1\"";
		vstr.str("");
		pnamestr.str("");
		pnamestr << "\"" << pname << "\"";
		vstr << std::setw(PARAMETER_VALUE_WIDTH) << std::fixed << std::setprecision(prec) << val;
		list.push_back({pnamestr.str(),unit.str(),(std::string)patts["description"],vstr.str()});
		//std::cout << std::setw(PARAMETER_NAME_WIDTH) << pname << " " << std::setw(PARAMETER_VALUE_WIDTH) << std::fixed << std::setprecision(prec) << val << " " << std::setw(PARAMETER_UNIT_WIDTH) << unit.str() << " " << (std::string)patts["description"] << std::endl;
	}
}
