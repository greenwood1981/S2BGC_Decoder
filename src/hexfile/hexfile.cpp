#include "hexfile.h"
#include <sstream>
#include <algorithm>
#include "../output/write_log.h"
#include <filesystem>
#include <format>

#include "../json/json.hpp"
using json = nlohmann::ordered_json;
extern json config;

extern std::string S2BGC_PATH;

hexfile::hexfile( std::string f ) {
	upload_command = ""; // initialize upload command

	// Open .hex file
	filepath = f;
    filename = f.substr(f.find_last_of("/\\") + 1); // file basename

	fin.open(filepath,std::ifstream::binary);
	if ( !fin.good() ) {
		log(std::string(" -- Error; unable to open: ") + filename);
		return;
	}

	log( std::string("Processing ") + filename );

	int cnt = 0;
	message m;
	while (fin >> m) {
		if (cnt++ > 50) {
			log( std::string(" -- Error reading ") + filename + " [infinite loop]; breaking");
			break;
		}
		sn = m.SN;
		cycle = m.cycle;

		// Ignore duplicate messages (identified using PID)
		if ( messages.count(m.PID) ) {
			log(std::string(" -- warning, ignoring duplicate PID: ") + std::to_string(m.PID) );
			continue;
		}
		for( const auto &p : m.packets ) {
			// log(std::string(" -- inserting: ") + std::to_string((int)p.header.sensorID) + " " + std::to_string( (int)p.header.data_ID ) );
			packets.push_back(p);
		}
		messages[m.PID] = m;
	}
}

std::ostream & operator << ( std::ostream &os, hexfile &h) {
	int packet_count = 0;
	int packet_bytes = 0;
	int total_bytes = 0;
	char profile_key[7];
	string desc;

	std::map<std::string,int> packets;
	std::map<std::string,int> pcnt;


	os << "#        SN Cycle Size PID                Date Sensor_IDs" << std::endl;
	for (auto & [PID,m] : h.messages) {
		os << "RUDICS" << " " << std::setw(4) << h.sn << " " << std::setw(5) << h.cycle << " ";
		os << std::setw(4) << m.size << " " << std::setw(3) << PID << " ";
		os << std::setfill('0') << std::setw(4) << m.yy << "/" << std::setw(2) << m.mm << "/";
		os << std::setw(2) << m.dd << " " << std::setw(2) << m.HH << ":" << std::setw(2) << m.MM << ":" << std::setw(2) << m.SS << " ";
		os << std::setfill(' ');
		total_bytes += m.size;

		for (auto p : m.packets) {
			os << std::hex << std::setw(2) << std::setfill('0') << (unsigned int)p.header.sensorID << ",";
			packet_count++;
			packet_bytes += p.size;

			int sensor_id = p.data[0];
			int data_id = p.data[3] & 0x0F;
			int segment = p.data[5] & 0x0F; // sub-segment index
			int pro = p.data[4];

			sprintf(profile_key,"%02X%02X%02X",sensor_id,data_id,pro);

			// Initialize if missing
			if (packets.count(std::string(profile_key)) == 0)
				packets[std::string(profile_key)] = 0;
			if (pcnt.count(std::string(profile_key)) == 0)
				pcnt[std::string(profile_key)] = 0;

			// Increment
			packets[std::string(profile_key)] += p.size;
			pcnt[std::string(profile_key)]++;
		}

		os << std::dec << std::setfill(' ') << std::endl;
	}
	os << std::endl;
	os << "# Raw packet information" << std::endl;
	os << "# Packets  Packet_bytes Total_bytes" << std::endl;
	os << "P0 " << std::setw(7) << packet_count << " " << std::setw(12) << packet_bytes << " " << std::setw(11) << total_bytes << std::endl;
	os << "# Sensor_ID bytes packets Description" << std::endl;
	for (auto & [key,value] : packets) {
		if (config["packets"].count(key))
			desc = std::string(config["packets"][key]["profile"]) + " " + std::string(config["packets"][key]["name"]);
		else
			desc = "unknown";
		os << std::setw(11) << key << " " << std::setw(5) << value << " " << std::setw(7) << pcnt[key] << " " << desc << std::endl;
	}

	return os;
}


void hexfile::print() {
	for (auto & [PID,m] : messages) {
		//std::cout << "Message PID:" << PID << " size: " << m.size << std::endl;
		for (auto p : m.packets) {
			//std::cout << " - SID:" << std::hex << std::setfill('0') << std::setw(2) << (uint16_t)p.header.sensorID << std::dec << " size: " << std::setw(3) << p.size << " ";
			for( int h = 0; h < p.data.size(); h++) {
				if (h > 50) {
					break;
				}
				std::cout << std::hex << std::setw(2) << std::setfill('0') << (uint16_t)p.data[h] << " ";
			}
			std::cout << std::dec << std::endl;
		}
	}
	std::cout << std::setfill(' ');
}

void hexfile::Decode() {
	char key[7];
    packets.sort(); // sort by priority

	for (auto p : packets) {
			sprintf(key,"%02X%02X%02X",p.data[0],p.data[3] & 0x0F,p.data[4]); // 0,4?
			if (p.header.sensorID == 0) { // GPS
				gps.push_back(GPS(p.data));
			}
            else if (p.header.sensorID == 2) { // Argo Mission
				argo.Decode(p.data);
            }
            else if (p.header.sensorID == 1) { // parameter dump
                if (p.header.data_ID == 2) { // upload command?
					for(int c = 6; c < p.header.nDat2 - 1; c++) {
						if ( 32 <= p.data[c] && p.data[c] <= 126 )
							upload_command += p.data[c];
						else
							upload_command += '*';
					}
					log(std::string("Packet[ 1] Upload command: ") + upload_command);
                }
				else if (p.header.data_ID == 3)
					miss.parse(p.data);
				else if (p.header.data_ID == 4)
					log("Packet[ 1] SCI parameter array; parsing not implemented");
				else {
					log(std::string(" -- Unknown parameter listing [") + key + "]; ignore");
				}
            }
			else if (p.header.sensorID == 4) { // Engineering
				int type = p.header.data_ID;
				int dir = p.header.proDir;
				if (type == 0 && dir == 1) {
					fall.Decode(p.data);
				}
				if (type == 0 && dir == 0) {
					rise.Decode(p.data);
				}
				if (type == 2) { // BGC Science
					science.Parse(p.data);
				}
				if (type == 4) { // pump
					pump.Decode(p.data);
				}
				if (type == 10 ) { // diagnostic engineering data
					eng_data.parse_pfile(p.data,S2BGC_PATH + "/config/diagnostic.json");
					log("Packet[10] Diagnostic");
				}
				if (type == 11 ) { // profile engineering data
					eng_data.parse_pfile(p.data,S2BGC_PATH + "/config/Engineering_Data.json");
				}
                if (type == 12) { // BIT Beacon
                    beacon.parse(p.data);
                }
                if (type == 13 | type == 14 ) { // BIT OK [13] and FAIL [14]
					bit.parse(p.data);
                }
			}
			else if (p.header.sensorID == 5) { // BIST
				std::map<int,std::string> bist_description = {
					{0,"Main CPU"},{1,"Science Board"},{2,"CTD"},{3,"Dissolved Oxygen"},{4,"pH"},{5,"ECO"},
                    {6,"OCR"},{7,"NO3 Engineering"},{8,"NO3 Spectral"},{9,"NO3 ASCII string"}
                };
				log(std::string("Packet[ 5] BIST ") + bist_description[ p.header.data_ID] );
				if (p.header.data_ID == 2) {
					bist_ctd.parse(p.data);
				}
				if (p.header.data_ID == 3) {
					bist_do.parse(p.data);
				}
				if (p.header.data_ID == 4) {
					bist_ph.parse(p.data);
				}
				if (p.header.data_ID == 5) {
					bist_eco.parse(p.data);
				}
				if (p.header.data_ID == 6) {
					bist_ocr.parse(p.data);
				}
				if (p.header.data_ID == 7) {
					bist_no3.parse_engineer(p.data);
				}
				if (p.header.data_ID == 8) {
					bist_no3.parse_spectral(p.data);
				}
				if (p.header.data_ID == 9) {
					bist_no3.parse_ascii(p.data);
				}
			}
			else if ( config["packets"].contains(key) ) {
				string pname = config["packets"][key]["profile"];
				string vname = config["packets"][key]["name"];
				prof[pname].insert(p.data);
			}
	}

	// Convert raw counts to SI units
	for (auto &[pname,vdict] : config["prof"].items()) {
		prof[pname].Conv_Cnts2SI();
		//prof[pname].print_stats();
	}
}

void hexfile::archive() {
	// check if float subdirectories exist. If not, create empty skeleton
	std::string floatdir = std::string(config["directories"]["output"]) + "/" + std::to_string(sn);
	if (!std::filesystem::exists(floatdir)) {
		std::filesystem::create_directory(floatdir);
		log(std::format("+ Creating float {} subdirectory",sn));
	}
	if (!std::filesystem::exists(floatdir+"/hex")) {
		std::filesystem::create_directory(floatdir + "/hex");
		log(std::format("+ Creating float {} hex subdirectory",sn));
	}
	std::filesystem::rename(filepath,floatdir+"/hex/"+filename); // move processed .hex file from incoming to float hex subdirectory
}
