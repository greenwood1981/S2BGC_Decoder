#include <sstream>
#include <algorithm>
#include "../hexfile/hexfile.h"
#include "../json/json.hpp"

using json = nlohmann::ordered_json;
extern json config;

void hexfile::write_List() {
	std::stringstream ss;
	ss << "data/" << sn << "/" << sn << "_" << std::setw(3) << std::setfill('0') << std::internal << cycle << ".lis";

	std::ofstream fout(ss.str());
	if (fout.good())
		std::cout << "Opened: " << ss.str() << std::endl;
	else
		std::cout << "unable to open: " << ss.str() << std::endl;

	fout << "# WHOI BGC-SOLO Decoder for RUDICS messages v20231129" << std::endl;
	fout << *this << std::endl;

	fout << "GPS Position" << std::endl;
	fout << "#    C Phs Date     Time   Latitude  Longitude Tfix  N Signal   Hdop status" << std::endl;
	for( auto g : gps)
		fout << g;
	fout << std::endl;


	// Print profiles
	for (auto &[pname,vdict] : config["prof"].items()) {
		//std::cout << "printing list " << pname << " size " << prof[pname].size() << std::endl;
		// print profilfe header [2 lines]
		fout << "# BGC-SOLO " << pname << std::endl;
		int cpres;    // column precision
		int cwidth;   // column width defined in config.json
		fout << "# ";
		for (auto & [var,vatts] : vdict.items()) {
			cwidth = std::max( var.size(), (size_t)vatts["col_width"] );
			fout << std::setw(cwidth) << var << " ";
		}
		fout << std::endl;

		// print profile [columns]
		fout << std::setfill(' ') << std::fixed;
		for (int i = 0; i < prof[pname].size; i++) {
			fout << "  ";
			for (auto & [var,vatts] : vdict.items()) {
				cwidth = std::max( var.size(), (size_t)vatts["col_width"]);
				cpres  = vatts["col_precision"];
				fout << std::setw(cwidth) << std::setprecision(cpres) << prof[pname][var][i] << " ";
			}
			fout << std::endl;
		}
		fout << std::endl;
		prof[pname].print_stats();
	}

	fout << fall << std::endl;
	fout << rise << std::endl;
	fout << pump << std::endl;
}
