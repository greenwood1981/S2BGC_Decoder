#include <iostream> // BG DEBUG
#include <fstream>
#include <sstream>
#include <cmath>
#include <filesystem>
#include <boost/date_time.hpp>
using namespace boost::posix_time;
#include "../hexfile/hexfile.h"
#include "../json/json.hpp"
#include "../version.h"

#include "../output/write_log.h"
#include <format>

using json = nlohmann::ordered_json;
extern json config;
extern string S2BGC_PATH;

std::string decimal( float val, int cwidth, int cpres ) {
    std::stringstream ss;
	// Test for NaN; JSON does not support NaN. Send "null" instead
	if (std::isnan(val))
		ss << "null";
	else if (val == -999)
		ss << std::setw(cwidth) << -999; // if -999, no need to pad out extra decimals; -999.0000
	else
    	ss << std::fixed << std::setw(cwidth) << std::setprecision(cpres) << val;
    return ss.str();
}

boost::posix_time::ptime utcnow() {
  return boost::posix_time::second_clock::universal_time();
}

string date_format( const ptime &t, string format ) {
  int yr,mo,dy,hr,mn,sc;
  const string alphabet = "ymdHMS"; // define characters to be considered symbols
  string tmp = ""; // temporary variables used to parse format string

  if ( t.is_not_a_date_time() ) {
    yr=9999; mo=99; dy=99; hr=99; mn=99; sc=99; // set time to fill if not_a_date_time
  }
  else {
    try {
      tm Date = to_tm(t);
      yr = Date.tm_year + 1900;
      mo = Date.tm_mon + 1;
      dy = Date.tm_mday;
      hr = Date.tm_hour;
      mn = Date.tm_min;
      sc = Date.tm_sec;
    }
    catch (const std::out_of_range &e) {
      yr=9999; mo=99; dy=99; hr=99; mn=99; sc=99; // set time to fill if any value is out of range
    }
  }
  // parse format string
  std::stringstream out;
  out << std::fixed << std::setfill('0');
  while ( format.size() ) {
    if (format.substr(0,4) == "yyyy") {
      out << std::setw(4) << yr;
      format.erase(0,4);
    }
    else if (format.substr(0,2) == "yy") {
      out << std::setw(2) << yr % 100;
      format.erase(0,2);
    }
    else if (format.substr(0,2) == "mm") {
      out << std::setw(2) << mo;
      format.erase(0,2);
    }
    else if (format.substr(0,2) == "dd") {
      out << std::setw(2) << dy;
      format.erase(0,2);
    }
    else if (format.substr(0,2) == "HH") {
      out << std::setw(2) << hr;
      format.erase(0,2);
    }
    else if (format.substr(0,2) == "MM") {
      out << std::setw(2) << mn;
      format.erase(0,2);
    }
    else if (format.substr(0,2) == "SS") {
      out << std::setw(2) << sc;
      format.erase(0,2);
    }
    else {
      out << format[0];
      format.erase(0,1);
    }
  }
  return out.str();
}

void hexfile::write_JSON() {
  json Doc;
  bool first1,first2,first3,first4;
  uint16_t flag;

  std::stringstream gpstr;

  // check if float subdirectories exist. If not, create them
  std::string floatdir = std::string(config["directories"]["output"]) + "/" + std::to_string(sn);
  if (!std::filesystem::exists(floatdir)) {
    std::filesystem::create_directory(floatdir);
    log(std::format("+ Creating float {} subdirectory",sn));
  }
  if (!std::filesystem::exists(floatdir+"/json")) {
    std::filesystem::create_directory(floatdir + "/json");
    log(std::format("+ Creating float {} json subdirectory",sn));
  }
  string jsonpath = format("{:s}/{:d}/json/{:d}_{:-03d}.json",std::string(config["directories"]["output"]),sn,sn,cycle);

  std::ofstream fout(jsonpath);
  if (fout.good())
    log( std::format("Writing {:s}",jsonpath) );
  else {
    log( std::format("Unable to open {:s}",jsonpath) );
	return;
  }

  fout << "{" << std::endl;

  // ====== Hexfile summary =====
  int packet_count = 0;
  int packet_bytes = 0;
  int total_bytes = 0;
  char profile_key[7];
  string desc;

  std::map<std::string,int> packets;
  std::map<std::string,int> pcnt;

  fout << "  \"FILE_CREATION_DATE\": \"" << date_format(utcnow(),config["DATE_FORMAT"]) << "\"," << std::endl;
  fout << "  \"DECODER_VERSION\": " << "\"" << MAJOR_VERSION << "." << MINOR_VERSION << "\"," << std::endl;
  fout << "  \"SCHEMA_VERSION\": " << config["SCHEMA_VERSION"] << "," << std::endl;

  // Float specific meta-data.
  string metapath = format("{:s}/{:d}/{:d}_meta.json",std::string(config["directories"]["output"]),sn,sn);
  // Look for SN_meta.json file in float subdirectory, if it doesn't exist, create one using default values [see config/default_meta.json]
  if (!std::filesystem::exists(metapath)) {
    string metadefault = S2BGC_PATH + "/config/default_meta.json";
    std::filesystem::copy(metadefault,metapath);
    log(std::format("+ Create Float {}_meta.json with default values",sn));
  }

  std::ifstream f(metapath);
  auto float_meta = nlohmann::ordered_json::parse(f); // user ordered_json to preserver order
  // iterate over meta attributes defined in floatxxx.json file
  for (auto & [key,val] : float_meta.items())
    fout << "  \"" << key << "\": " << val << "," << std::endl;
  fout << "  \"PROFILE_NUMBER\": " << cycle << "," << std::endl;


  // ====== HEX SUMMARY ==========
  fout << "  \"hex_summary\": [" << std::fixed << std::endl;
  first1 = true;
  for (auto & [PID,m] : messages) {
    if (!first1)
      fout << "," << std::endl;
    first1 = false;
    fout << "    { \"PID\": " << std::setw(2) << PID << ", ";
    fout << "\"source\": \"" << m.type << "\", ";
    fout << "\"TIME\": \"" << date_format(m.time,config["DATE_FORMAT"]) << "\", ";
    fout << "\"momsn\": " << m.momsn << ", ";
	//fout << std::setfill('0');
	//fout << "\"TIME\": \"" << std::setw(4) << m.yy << "/" << std::setw(2) << m.mm << "/" << std::setw(2) << m.dd;
    //fout << "T" << std::setw(2) << m.HH << ":" << std::setw(2) << m.MM << ":" << std::setw(2) << m.SS << "Z\", ";
	fout << std::setfill(' ');
    fout << "\"size\": " << std::setw(4) << m.size << ", ";
    fout << "\"sensor_ids\": [";
    total_bytes += m.size;
    first2 = true;
    // compute packet statistics
    for (auto p : m.packets) {
      if (!first2)
        fout << ",";
      first2 = false;
      fout << (unsigned int)p.header.sensorID;
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
    fout << "] }";
  }
  fout << std::endl;
  fout << "  ]," << std::endl;

  // ====== PACKETS =============
  fout << "  \"packet_info\": {" << std::fixed << std::endl;
  fout << "    \"packet_count\": " << packet_count << "," << std::endl;
  fout << "    \"packet_bytes\": " << packet_bytes << "," << std::endl;
  fout << "    \"packet_type\": [" << std::endl;

  first1 = true;
  for( auto & [key,val] : packets ) {
    if (!first1)
      fout << "," << std::endl;
    first1 = false;
    if (config["packets"].count(key))
      desc = std::string(config["packets"][key]["profile"]) + " " + std::string(config["packets"][key]["name"]);
    else
      desc = "unknown";
    fout << "      { \"id\": \"" << key << "\", \"bytes\": " << std::setw(4) << val << ", \"packets\": " << std::setw(2) << pcnt[key] << ", \"description\": \"" << desc << "\" }";
  }
  fout << std::endl << "    ]" << std::endl;
  fout << "  }";


  // ====== GPS =================
  std::map<int,std::string> gps_phase = {{0,"GPS_BIST"},{1,"GPS_START"},{2,"GPS_END"},{3,"GPS_ABORT"},{5,"GPS_BITPASS"},{6,"GPS_BITFAIL"}};
  if (gps.size()) {
    fout << "," << std::endl;
    fout << "  \"GPS\": [" << std::fixed << std::endl;
    first1 = true;
    for ( auto g : gps ) {
      if (!first1)
        fout << "," << std::endl;
      first1 = false;
      gpstr.str("");
	  gpstr << "\"" << gps_phase[g.phase] << "\"";
      fout << "    { \"description\": " << std::setw(11) << gpstr.str() << ", ";
      fout << "\"TIME\": \"" << date_format(g.gps_time,config["DATE_FORMAT"]) << "\", ";
      fout << "\"LATITUDE\": " << decimal(g.flat,9,5)  << ", ";
      fout << "\"LONGITUDE\": " << decimal(g.flon,10,5) << ", ";
      fout << "\"HDOP\": " << decimal(g.hdop,5,1) << ", ";
      fout << "\"sat_cnt\": " << std::setw(2) << g.num_sat << ", ";
      fout << "\"snr_min\": " << std::setw(2) << g.snr_min << ", ";
      fout << "\"snr_mean\": " << std::setw(2) << g.snr_mean << ", ";
      fout << "\"snr_max\": " << std::setw(2) << g.snr_max << ", ";
      fout << "\"time_to_fix\": " << std::setw(2) << g.time2fix << ", ";
      fout << "\"valid\": " << std::setw(2) << g.valid << " }";
    }
    fout << std::endl;
    fout << "  ]";
  }

  // ==== Upload command ========
  if ( upload_command.size() ) {
    fout << "," << std::endl;
    fout << "  \"Upload_Command\": \"" << upload_command << "\"";
  }

  // ==== ARGO Mission ==========
  if (argo.received) {
    fout << "," << std::endl;
    fout << "  \"ARGO_Mission\": {" << std::fixed << std::endl;
    fout << "    \"Float_Version\": " << argo.float_version << "," << std::endl;
    fout << "    \"firmware_version\": " << decimal(argo.firmware_version,3,1) << "," << std::endl;
    fout << "    \"min_ascent_rate\": " << argo.ascent_speed_target << "," << std::endl;
    fout << "    \"profile_target\": " << argo.prof_depth_target << "," << std::endl;
    fout << "    \"drift_target\": " << argo.park_depth_target << "," << std::endl;
    fout << "    \"max_rise_time\": " << decimal(argo.rise_time_max,5,2) << "," << std::endl;
    fout << "    \"max_fall_to_park\": " << decimal(argo.fall2park_time_max,5,2) << "," << std::endl;
    fout << "    \"max_fall_to_profile\": " << decimal(argo.park2prof_time_max,5,2) << "," << std::endl;
    fout << "    \"target_drift_time\": " << argo.drift_time_target << "," << std::endl;
    fout << "    \"target_surface_time\": " << decimal(argo.surf_time_max,6,4) << "," << std::endl;
    fout << "    \"seek_periods\": " << argo.num_seeks << "," << std::endl;
    fout << "    \"seek_time\": " << decimal(argo.seek_time_max,6,4) << "," << std::endl;
    fout << "    \"ctd_pres\": { \"gain\": " << decimal(argo.pres_gain,4,0) << ", \"offset\": " << decimal(argo.pres_offset,3,0) << "}," << std::endl;
    fout << "    \"ctd_temp\": { \"gain\": " << decimal(argo.temp_gain,4,0) << ", \"offset\": " << decimal(argo.temp_offset,3,0) << "}," << std::endl;
    fout << "    \"ctd_psal\": { \"gain\": " << decimal(argo.psal_gain,4,0) << ", \"offset\": " << decimal(argo.psal_offset,3,0) << "}," << std::endl;
    fout << "    \"cycle_time_max\": " << decimal(argo.cycle_time_max,6,3) << std::endl;
    fout << "  }";
  }

  // ==== BIT ===================
  std::map<int,std::string> BIT_status = {
    {0,"RAM BAD"},{1,"ROM BAD"},{2,"EEPROM test failed"},{3,"Air Vent failed"},{4,"CPU Voltage low"},{5,"Pump Voltage low"},{6,"Low Vacuum"},
    {7,"Slow Oil Pump"},{8,"Pump current too high or low"},{9,"SBE comm fail"},{10,"Vale open or close failure"},{11,"Air vacuum high (>1100)"},
    {12,"Comms failed to iridium modem"},{13,"Comms failed to gps device"},{14,"HP pump didn't run long enough"},{15,"pH Bias Battery too low"} };

  // check BIT.status for errors, and add to BIT_errors map
  std::map<int,std::string> BIT_errors;

  if (bit.received) {

    for (int p = 0; p < 16; p++) {
      flag = std::pow(2,p);
      if (bit.status & flag) {
        BIT_errors[flag] = BIT_status[p];
	  }
    }

    fout << "," << std::endl;
    fout << "  \"BIT\": {" << std::endl;
    if (bit.status == 0)
      fout << "    \"status\": \"OK\"," << std::endl;
    else {
      fout << "    \"status\": \"FAIL\"," << std::endl;
      fout << "    \"errors\": [" << std::hex << std::setfill('0') << std::endl;
      first1 = true;
      for (auto & [key,error] : BIT_errors) {
        if (!first1)
          fout << "," << std::endl;
        first1 = false;
        fout << "      { \"0x" << std::setw(4) << key << "\": \"" << error << "\" }";
      }
      fout << std::endl <<  "    ]," << std::endl;
	  fout << std::dec; // set standard output back to decimal
    }
    fout << "    \"blocks_queued\": " << bit.nQueued << "," << std::endl;
    fout << "    \"pressure\": " << bit.pressure << "," << std::endl;
    fout << "    \"cpu_voltage\": " << decimal(bit.cpu_voltage,5,2) << "," << std::endl;
    fout << "    \"pump_voltage_prior\": " << decimal(bit.pump_voltage_prior,5,2) << "," << std::endl;
    fout << "    \"pump_voltage_after\": " << decimal(bit.pump_voltage_after,5,2) << "," << std::endl;
    fout << "    \"pump_current_mA\": " << bit.pump_current << "," << std::endl;
    fout << "    \"pump_time_s\": " << bit.pump_time << "," << std::endl;
    fout << "    \"pump_oil_prior\": " << bit.pump_oil_prior << "," << std::endl;
    fout << "    \"pump_oil_after\": " << bit.pump_oil_after << "," << std::endl;
    fout << "    \"vacuum_prior_inHg\": " << decimal(bit.vacuum_prior,5,2) << "," << std::endl;
    fout << "    \"vacuum_after_inHg\": " << decimal(bit.vacuum_after,5,2) << "," << std::endl;
    fout << "    \"valve_open\": " << bit.valve_open << "," << std::endl;
    fout << "    \"valve_close\": " << bit.valve_close << "," << std::endl;
    fout << "    \"interrupt_id\": " << bit.interrupt_id << "," << std::endl;
    fout << "    \"SBE_response\": \"" << bit.SBE_response << "\"," << std::endl;
    fout << "    \"cpu_temp_degC\": " << decimal(bit.cpu_temp,5,2) << "," << std::endl;
    fout << "    \"RH\": " << bit.RH << std::endl;
    fout << "  }";
  }

  // ==== BEACON ================
  if (beacon.received) {

    fout << "," << std::endl;
    fout << "  \"BIT\": {" << std::endl;
    fout << "    \"status\": \"Beacon\"," << std::endl;
    fout << "    \"nQueued\": " << beacon.nQueued << "," << std::endl;
    fout << "    \"nTries\": " << beacon.nTries << "," << std::endl;
    fout << "    \"parXstat\": " << beacon.parXstat << "," << std::endl;
    fout << "    \"SBDIstat\": " << beacon.SBDIstat << "," << std::endl;
    fout << "    \"cpu_voltage\": " << decimal(beacon.cpu_voltage,5,2) << "," << std::endl;
    fout << "    \"pump_voltage\": " << decimal(beacon.pump_voltage,5,2) << "," << std::endl;
    fout << "    \"vacuum_transmit_inHg\": " << decimal(beacon.vacuum_now,5,2) << "," << std::endl;
    fout << "    \"vacuum_abort_inHg\": " << decimal(beacon.vacuum_abort,5,2) << "," << std::endl;
    fout << "    \"last_interrupt\": " << beacon.ISRID << "," << std::endl;
    fout << "    \"abortFlag\": " << beacon.abortFlag << "," << std::endl;
    fout << "    \"CPUtemp\": " << decimal(beacon.CPUtemp,5,2) << "," << std::endl;
    fout << "    \"RH\": " << beacon.RH << std::endl;
    fout << "  }";
  }


  // ==== FALL TIME-SERIES ======
  std::map<int,std::string> dive_phase = {
    {0,"Dive start"},{1,"Start of sink"},{2,"Pump 2 target"},{3,"Seek"},{4,"Drift begin"},{5,"Drift seek"},{6,"Fall to profile begin"},
    {7,"pre-ascend"},{8,"Profile start"},{9,"Profile end"},{10,"Ice turnaround"},{11,"Sinking"},{12,"Drifting"},{13,"Descending"},
    {14,"Ascending"},{15,"Reached surface"} };

  if (fall.Scan.size()) {
    fout << "," << std::endl;
    fout << "  \"Fall\": [" << std::endl;
    first1 = true;
    for ( auto s : fall.Scan ) {
      if (!first1)
        fout << "," << std::endl;
      first1 = false;
      fout << "    { ";
      fout << "\"TIME\": \"" << date_format(s.time,config["DATE_FORMAT"]) << "\", ";
      fout << "\"PRES\": " << decimal(s.pres,7,2) << ", ";
      fout << "\"phase\": " << decimal(s.phase,2,0) << ", ";
      fout << "\"description\": \"" << dive_phase[s.phase] << "\" }";
    }
    fout << std::endl;
    fout << "  ]";
  }

  // ==== RISE TIME-SERIES ======
  if (rise.Scan.size()) {
    fout << "," << std::endl;
    fout << "  \"Rise\": [" << std::endl;
    first1 = true;
    for ( auto s : rise.Scan ) {
      if (!first1)
        fout << "," << std::endl;
      first1 = false;
      fout << "    { ";
      fout << "\"TIME\": \"" << date_format(s.time,config["DATE_FORMAT"]) << "\", ";
      fout << "\"PRES\": " << decimal(s.pres,7,2) << ", ";
      fout << "\"phase\": " << decimal(s.phase,2,0) << ", ";
      fout << "\"description\": \"" << dive_phase[s.phase] << "\" }";
    }
    fout << std::endl;
    fout << "  ]";
  }

  // ====== BIST CTD ================
  std::map<uint8_t,std::string> CTD_Status_Flag = {
    {0,"SBE ACK Error"},
    {1,"SBE Error starting profile"},
    {2,"SBE Error stopping profile"},
    {3,"SBE Error with init_params()"},
    {4,"SBE Error getting P,T,S during profile"},
    {5,"SBE Error getting PTS sample"},
    {6,"SBE Error getting an FP sample"},
    {7,"SBE busy; not done with last sample yet"},
    {8,"SBE did not parse the command correctly"},
    {9,"SBE emulation mode"},
    {10,"SBE fatal"},
    {11,"SBE Solo2 has control of the SBE!!"}
  };
  std::map<int,std::string> ctd_errors;
  //bist_ctd.status = 134; // BG TESTING -- CLEAN UP
  for (int p = 0; p < 12; p++) {
    flag = std::pow(2,p);
    if (bist_ctd.status & flag) {
		ctd_errors[flag] = CTD_Status_Flag[p];
	}
  }
  if ( bist_ctd.received) {
    fout << "," << std::endl;
    fout << "  \"bist_ctd\": {" << std::endl;
    fout << "    \"status\": " << bist_ctd.status << "," << std::endl;
	if (ctd_errors.size()) {
      fout << "    \"errors\": [" << std::hex << std::setfill('0') << std::endl;
      first1 = true;
      for (auto & [key,error] : ctd_errors) {
        if (!first1)
		  fout << "," << std::endl;
        first1 = false;
        fout << "      { \"0x" << std::setw(4) << key << "\": \"" << error << "\" }";
      }
	  fout << std::endl;
      fout << "    ]," << std::endl;
      fout << std::dec << std::setfill(' ');
    }
    fout << "    \"numErr\": " << bist_ctd.numErr << "," << std::endl;
    fout << "    \"voltage_V\": " << decimal(0.01 * bist_ctd.volt_cnt,5,2) << "," << std::endl;
    fout << "    \"pressure_dbar\": " << decimal(0.04 * bist_ctd.p_cnt - 10,7,2) << "," << std::endl;
    fout << "    \"current_idle_mA\": " << bist_ctd.ma0 << "," << std::endl;
    fout << "    \"current_ctd_high_mA\": " << bist_ctd.ma1 << "," << std::endl;
    fout << "    \"current_ctd_low_mA\": " << bist_ctd.ma2 << "," << std::endl;
    fout << "    \"current_ctd_sleep_mA\": " << bist_ctd.ma3 << std::endl;
    fout << "  }";
  }

  // ====== BIST DO ================
  std::map<uint8_t,std::string> DO_Status_Flag = {
    {0,"DO Error parsing the return string"},
    {1,"DO failed all tries to parse"},
    {2,"DO timed out from inactivity"},
    {3,"DO not correct state when asked for sample"},
    {4,"DO bad start of string to parse"},
    {5,"DO emulation mode"}
  };
  std::map<int,std::string> do_errors;
  //bist_do.status = 38; // BG TESTING -- CLEAN UP
  for (int p = 0; p < 6; p++) {
    flag = std::pow(2,p);
    if (bist_do.status & flag) {
		do_errors[flag] = DO_Status_Flag[p];
	}
  }
  if ( bist_do.received) {
    fout << "," << std::endl;
    fout << "  \"bist_DO\": {" << std::endl;
    fout << "    \"status\": " << bist_do.status << "," << std::endl;
	if (do_errors.size()) {
      fout << "    \"errors\": [" << std::hex << std::setfill('0') << std::endl;
      first1 = true;
      for (auto & [key,error] : do_errors) {
        if (!first1)
		  fout << "," << std::endl;
        first1 = false;
        fout << "      { \"0x" << std::setw(4) << key << "\": \"" << error << "\" }";
      }
	  fout << std::endl;
      fout << "    ]," << std::endl;
      fout << std::dec << std::setfill(' ');
    }
    fout << "    \"numErr\": " << bist_do.numErr << "," << std::endl;
    fout << "    \"voltage_V\": " << 0.01 * bist_do.volt_cnt << "," << std::endl;
    fout << "    \"max_current_mA\": " << bist_do.maMax << "," << std::endl;
    fout << "    \"avg_current_mA\": " << bist_do.maAvg << "," << std::endl;
	fout << std::fixed << std::setprecision(3);
    fout << "    \"phase_delay_us\": " << decimal(bist_do.doPh,6,3) << "," << std::endl;
    fout << "    \"thermistor_voltage_V\": " << decimal(bist_do.thmV,5,3) << "," << std::endl;
    fout << "    \"DO\": " << decimal(bist_do.DO,5,3) << "," << std::endl;
    fout << "    \"thermistor_degC\": " << decimal(bist_do.thmC,5,3) << std::endl;
    fout << "  }";
  }

  // ====== BIST pH ================
  std::map<uint8_t,std::string> pH_Status_Flag = {
    {0,"pH Error parsing the return string"},
    {1,"pH failed all tries to parse"},
    {2,"pH timed out from inactivity"},
    {3,"pH not correct state when asked for sample"},
    {4,"pH bad start of string to parse"},
    {5,"pH emulation mode"}
  };
  std::map<int,std::string> ph_errors;
  //bist_ph.status = 38; // BG TESTING -- CLEAN UP
  for (int p = 0; p < 6; p++) {
    flag = std::pow(2,p);
    if (bist_ph.status & flag) {
		ph_errors[flag] = pH_Status_Flag[p];
	}
  }
  if ( bist_ph.received) {
    fout << "," << std::endl;
    fout << "  \"bist_pH\": {" << std::endl;
    fout << "    \"status\": " << bist_ph.status << "," << std::endl;
	if (ph_errors.size()) {
      fout << "    \"errors\": [" << std::hex << std::setfill('0') << std::endl;
      first1 = true;
      for (auto & [key,error] : ph_errors) {
        if (!first1)
		  fout << "," << std::endl;
        first1 = false;
        fout << "      { \"0x" << std::setw(4) << key << "\": \"" << error << "\" }";
      }
	  fout << std::endl;
      fout << "    ]," << std::endl;
      fout << std::dec << std::setfill(' ');
    }
    fout << "    \"numErr\": " << bist_ph.numErr << "," << std::endl;
	fout << std::fixed << std::setprecision(3);
    fout << "    \"voltage_V\": " << 0.01 * bist_ph.volt_cnt << "," << std::endl;
    fout << "    \"max_current_mA\": " << bist_ph.maMax << "," << std::endl;
    fout << "    \"avg_current_mA\": " << bist_ph.maAvg << "," << std::endl;
    fout << "    \"vRef\": " << decimal(bist_ph.vRef,6,3) << "," << std::endl;
    fout << "    \"Vk\": " << decimal(bist_ph.Vk,6,3) << "," << std::endl;
    fout << "    \"Ik\": " << decimal(bist_ph.Ik,7,3) << "," << std::endl;
    fout << "    \"Ib\": " << decimal(bist_ph.Ib,7,3) << std::endl;
    fout << "  }";
  }

  // ====== BIST ECO ================
  std::map<uint8_t,std::string> eco_Status_Flag = {
    {0,"ECO Error parsing the return string"},
    {1,"ECO failed all tries to parse"},
    {2,"ECO timed out from inactivity"},
    {3,"ECO not correct state when asked for sample"},
    {4,"ECO bad start of string to parse"},
    {5,"ECO emulation mode"}
  };
  std::map<int,std::string> eco_errors;
  //bist_eco.status = 38; // BG TESTING -- CLEAN UP
  for (int p = 0; p < 6; p++) {
    flag = std::pow(2,p);
    if (bist_eco.status & flag) {
		eco_errors[flag] = eco_Status_Flag[p];
	}
  }
  if ( bist_eco.received) {
    fout << "," << std::endl;
    fout << "  \"bist_ECO\": {" << std::endl;
    fout << "    \"status\": " << bist_eco.status << "," << std::endl;
	if (eco_errors.size()) {
      fout << "    \"errors\": [" << std::hex << std::setfill('0') << std::endl;
      first1 = true;
      for (auto & [key,error] : eco_errors) {
        if (!first1)
		  fout << "," << std::endl;
        first1 = false;
        fout << "      { \"0x" << std::setw(4) << key << "\": \"" << error << "\" }";
      }
	  fout << std::endl;
      fout << "    ]," << std::endl;
      fout << std::dec << std::setfill(' ');
    }
    fout << "    \"numErr\": " << bist_eco.numErr << "," << std::endl;
	fout << std::fixed << std::setprecision(3);
    fout << "    \"voltage_V\": " << 0.01 * bist_eco.volt_cnt << "," << std::endl;
    fout << "    \"max_current_mA\": " << bist_eco.maMax << "," << std::endl;
    fout << "    \"avg_current_mA\": " << bist_eco.maAvg << "," << std::endl;
    fout << "    \"Chl\": " << bist_eco.Chl << "," << std::endl;
    fout << "    \"bb\": " << bist_eco.bb << "," << std::endl;
    fout << "    \"CDOM\": " << bist_eco.CDOM << std::endl;
    fout << "  }";
  }

  // ====== BIST OCR ================
  std::map<uint8_t,std::string> ocr_Status_Flag = {
    {0,"OCR Error parsing the return string"},
    {1,"OCR failed all tries to parse"},
    {2,"OCR timed out from inactivity"},
    {3,"OCR not correct state when asked for sample"},
    {4,"OCR bad start of string to parse"},
    {5,"OCR emulation mode"}
  };
  std::map<int,std::string> ocr_errors;
  //bist_ocr.status = 38; // BG TESTING -- CLEAN UP
  for (int p = 0; p < 6; p++) {
    flag = std::pow(2,p);
    if (bist_ocr.status & flag) {
		ocr_errors[flag] = ocr_Status_Flag[p];
	}
  }
  if ( bist_ocr.received) {
    fout << "," << std::endl;
    fout << "  \"bist_OCR\": {" << std::endl;
    fout << "    \"status\": " << bist_ocr.status << "," << std::endl;
	if (ocr_errors.size()) {
      fout << "    \"errors\": [" << std::hex << std::setfill('0') << std::endl;
      first1 = true;
      for (auto & [key,error] : ocr_errors) {
        if (!first1)
		  fout << "," << std::endl;
        first1 = false;
        fout << "      { \"0x" << std::setw(4) << key << "\": \"" << error << "\" }";
      }
	  fout << std::endl;
      fout << "    ]," << std::endl;
      fout << std::dec << std::setfill(' ');
    }
    fout << "    \"numErr\": " << bist_ocr.numErr << "," << std::endl;
	fout << std::fixed << std::setprecision(3);
    fout << "    \"voltage_V\": " << 0.01 * bist_ocr.volt_cnt << "," << std::endl;
    fout << "    \"max_current_mA\": " << bist_ocr.maMax << "," << std::endl;
    fout << "    \"avg_current_mA\": " << bist_ocr.maAvg << "," << std::endl;
    fout << "    \"ch01\": " << bist_ocr.ch01 << "," << std::endl;
    fout << "    \"ch02\": " << bist_ocr.ch02 << "," << std::endl;
    fout << "    \"ch03\": " << bist_ocr.ch03 << "," << std::endl;
    fout << "    \"ch04\": " << bist_ocr.ch04 << std::endl;
    fout << "  }";
  }

  // ====== BIST NO3 ================
  std::map<uint8_t,std::string> no3_Status_Flag = {
    {0,"Nitrate Error parsing the return string"},
    {1,"Nitrate failed all tries to parse"},
    {2,"Nitrate timed out from inactivity"},
    {3,"Nitrate not correct state when asked for sample"},
    {4,"Nitrate bad start of string to parse"},
    {5,"Nitrate no ACK from 'W'"},
    {6,"Nitrate bad CRC"},
    {7,"Nitrate emulation mode"},
    {8,"Nitrate error on 'ACK,TS,DAT'"},
    {9,"Nitrate error on getting SL response"},
    {10,"Nitrate error on getting the ctd"},
    {11,"Nitrate error on getting 'ACK,TS,DAT'"},
    {12,"Nitrate error on setting the RTC"}
  };
  std::map<int,std::string> no3_errors;
  //bist_no3.status = 38; // BG TESTING -- CLEAN UP
  for (int p = 0; p < 13; p++) {
    flag = std::pow(2,p);
    if (bist_no3.status & flag) {
		no3_errors[flag] = no3_Status_Flag[p];
	}
  }
  if ( bist_no3.eng_received) {
    fout << "," << std::endl;
    fout << "  \"bist_Nitrate\": {" << std::endl;
    fout << "    \"status\": " << bist_no3.status << "," << std::endl;
	if (no3_errors.size()) {
      fout << "    \"errors\": [" << std::hex << std::setfill('0') << std::endl;
      first1 = true;
      for (auto & [key,error] : no3_errors) {
        if (!first1)
		  fout << "," << std::endl;
        first1 = false;
        fout << "      { \"0x" << std::setw(4) << key << "\": \"" << error << "\" }";
      }
	  fout << std::endl;
      fout << "    ]," << std::endl;
      fout << std::dec << std::setfill(' ');
    }
    fout << "    \"numErr\": " << bist_no3.numErr << "," << std::endl;
	fout << std::fixed << std::setprecision(3);
    fout << "    \"J_err\": " << decimal(bist_no3.J_err,3,0) << "," << std::endl;
    fout << "    \"J_rh\": " << decimal(bist_no3.J_rh,6,2) << "," << std::endl;
    fout << "    \"J_volt\": " << decimal(bist_no3.J_volt,5,2) << "," << std::endl;
    fout << "    \"J_amps\": " << decimal(bist_no3.J_amps,5,3) << "," << std::endl;
    fout << "    \"J_darkM\": " << decimal(bist_no3.J_darkM,6,1) << "," << std::endl;
    fout << "    \"J_darkS\": " << decimal(bist_no3.J_darkS,6,1) << "," << std::endl;
    fout << "    \"J_Nitrate\": " << decimal(bist_no3.J_no3,6,3) << "," << std::endl;
    fout << "    \"J_res\": " << decimal(bist_no3.J_res,5,3) << "," << std::endl;
    fout << "    \"J_fit1\": " << decimal(bist_no3.J_fit1,5,1) << "," << std::endl;
    fout << "    \"J_fit2\": " << decimal(bist_no3.J_fit2,5,1) << "," << std::endl;
    fout << "    \"J_Spectra\": " << decimal(bist_no3.J_Spectra,7,2) << "," << std::endl;
    fout << "    \"J_SeaDark\": " << decimal(bist_no3.J_SeaDark,7,2) << "," << std::endl;
	if ( bist_no3.spectral_received ) {
		for( int x = 0; x <8; x++ )
			fout << "    \"sval[" << x << "]\": " << bist_no3.svals[x] << "," << std::endl;
	}
	if ( bist_no3.ascii_received )
		fout << "    \"SUNA_str\": \"" << bist_no3.SUNA_str << "\"," << std::endl;
	fout << "    \"ascii_timestamp\": \"" << date_format(bist_no3.ascii_timestamp,config["DATE_FORMAT"]) << "\"," << std::endl;
	fout << "    \"ascii_pressure\": " << decimal(bist_no3.ascii_pressure,7,2) << "," << std::endl;
	fout << "    \"ascii_temperature\": " << decimal(bist_no3.ascii_temperature,7,4) << "," << std::endl;
	fout << "    \"ascii_salinity\": " << decimal(bist_no3.ascii_salinity,7,4) << "," << std::endl;
	fout << "    \"ascii_sample_count\": " << bist_no3.ascii_sample_count << "," << std::endl;
	fout << "    \"ascii_cycle_count\": " << bist_no3.ascii_cycle_count << "," << std::endl;
	fout << "    \"ascii_error_count\": " << bist_no3.ascii_error_count << "," << std::endl;
	fout << "    \"ascii_internal_temp\": " << decimal(bist_no3.ascii_internal_temp,5,2) << "," << std::endl;
	fout << "    \"ascii_spectrometer\": " << decimal(bist_no3.ascii_spectrometer,6,2) << "," << std::endl;
	fout << "    \"ascii_RH\": " << decimal(bist_no3.ascii_RH,5,2) << "," << std::endl;
	fout << "    \"ascii_voltage\": " << decimal(bist_no3.ascii_voltage,5,2) << "," << std::endl;
	fout << "    \"ascii_current\": " << decimal(bist_no3.ascii_current,6,3) << "," << std::endl;
	fout << "    \"ascii_ref_detector_mean\": " << bist_no3.ascii_ref_detector_mean << "," << std::endl;
	fout << "    \"ascii_ref_detector_std\": " << bist_no3.ascii_ref_detector_std << "," << std::endl;
	fout << "    \"ascii_dark_spectrum_mean\": " << bist_no3.ascii_dark_spectrum_mean << "," << std::endl;
	fout << "    \"ascii_dark_spectrum_std\": " << bist_no3.ascii_dark_spectrum_std << "," << std::endl;
	fout << "    \"ascii_sensor_salinity\": " << decimal(bist_no3.ascii_sensor_salinity,5,2) << "," << std::endl;
	fout << "    \"ascii_sensor_nitrate\": " << decimal(bist_no3.ascii_sensor_nitrate,5,2) << "," << std::endl;
	fout << "    \"ascii_residual_rms\": " << decimal(bist_no3.ascii_residual_rms,8,7) << "," << std::endl;
	fout << "    \"ascii_FIT_pixel_begin\": " << bist_no3.ascii_FIT_pixel_begin << "," << std::endl;
	fout << "    \"ascii_FIT_pixel_end\": " << bist_no3.ascii_FIT_pixel_end << std::endl;
    fout << "  }";
  }


  // ====== ENGINEERING PARAMETERS ==========
  // 2025/02/04 BG implement Engineering BGC Science 0x02
  if (science.received) {
    fout << "," << std::endl;
    fout << "  \"Science\": {" << std::endl;
    fout << "    \"Files\": " << science.nWritten << "," << std::endl;
    fout << "    \"Space_available_MB\": " << science.mbFree << "," << std::endl;
    fout << "    \"CTD_Errors\": " << science.ctdParseErr << "," << std::endl;
    fout << "    \"Pressure_Jumps\": " << science.jumpCtr << std::endl;
    fout << "  }";
  }

  // ====== ENGINEERING PARAMETERS ==========
  if (eng_data.list.size()) {
    fout << "," << std::endl;
    fout << "  \"Engineering_Data\": {" << std::endl;
    first1 = true;
    for( auto p : eng_data.list ) {
      if (!first1)
        fout << "," << std::endl;
      first1 = false;
      fout << "    " << std::setw(10) << p.name << ": { ";
      //fout << "\"name\": " << std::setw(10) << p.name << ", ";
      fout << "\"value\": " << p.val << ", ";
  	  fout << "\"unit\": " << std::setw(6) << p.unit << ", ";
      fout << "\"description\": \"" << p.desc << "\" }";
    }
    fout << std::endl;
    fout << "  }";
  }

  // ====== MISSION PARAMETERS ==========
  string pname;
  if (miss.list.size()) {
    fout << "," << std::endl;
    fout << "  \"Mission\": {" << std::endl;
    first1 = true;
    for( auto p : miss.list ) {
      if (!first1)
        fout << "," << std::endl;
      first1 = false;
      pname = "\"" + p.name + "\"";
      fout << "    " << std::setw(12) << pname << ": { ";
      fout << "\"value\": " << std::setw(5) << p.val << ", ";
  	  fout << "\"unit\": \"" << std::setw(4) << p.unit << "\", ";
      fout << "\"description\": \"" << p.desc << "\" }";
    }
    fout << std::endl;
    fout << "  }";
  }

  // ======== WRITE PUMP ========
  if (pump.Scan.size()) {
    fout << "," << std::endl;
    fout << "  \"Pump\": [" << std::endl;
    first1 = true;
    for( auto s : pump.Scan ) {
      if (!first1)
        fout << "," << std::endl;
      first1 = false;
      fout << "    { ";
      fout << "\"PRES\": " << decimal(s.pres,7,2) << ", ";
      fout << "\"current\": " << decimal(s.curr,5,0) << ", ";
      fout << "\"voltage\": " << decimal(s.volt,5,2) << ", ";
      fout << "\"pump_time\": " << decimal(s.pump_time,3,0) << ", ";
      fout << "\"vac_start\": " << decimal(s.vac_strt,2,0) << ", ";
      fout << "\"vac_end\": " << decimal(s.vac_end,3,0) << ", ";
      fout << "\"phase\": " << decimal(s.phase,2,0) << ", ";
      fout << "\"description\": \"" << dive_phase[s.phase] << "\" }";
    }
    fout << std::endl;
    fout << "  ]";
  }

  // ======== WRITE PROFILES ========
  first3 = true;
  for (auto &[pname,vdict] : config["prof"].items()) {
    if (!prof[pname].size())
      continue; // skip json output for empty profiles
    fout << "," << std::endl;
    //if (!first3)
    //  fout << "," << std::endl;
    first3 = false;
    fout << "  \"" << pname << "\": [";
	int cpres;    // column precision
	int cwidth;   // column width defined in config.json
    // columns
    first1 = true;
	for (int i = 0; i < prof[pname].size(); i++) {
      if (!first1)
        fout << ",";
      fout << std::endl;
      first1 = false;
      fout << "    { ";
      first2 = true;
      for (auto & [var,vatts] : vdict.items()) {
        cwidth = vatts["col_width"];
        cpres  = vatts["col_precision"];
		if ( pname == "NITRATE Binned" && var[0] == 'S' && var != "S01")
		  continue; // Spectrum columns handled uniquely below
        if (!first2)
          fout << ", ";
        first2 = false;
        //rec[var] = decimal(prof[pname][var][i],cpres);

		// Handle NO3 Spectrum columns uniquely
		int spectra_count = 41; // default NO3 spectra count = 41
		if (bist_no3.eng_received) {
			spectra_count = bist_no3.J_fit2 - bist_no3.J_fit1 + 1;
		}

		if (var == "S01") {
			std::stringstream sname;
			fout << "\"Spectrum\": [" << std::setfill(' ');
			first4 = true;
			for ( int s = 1; s <= spectra_count; s++ ) {
				if (!first4)
					fout << ",";
				first4 = false;
				sname.str("");
				sname << "S" << std::setw(2) << std::setfill('0') << s;
				fout << std::fixed << std::setprecision(0) << prof[pname][sname.str()][i];
			}
			fout << "]";
		}
		else
	        fout << "\"" << var << "\": " << decimal(prof[pname][var][i],cwidth,cpres);
      }
      fout << " }";
    }
    fout << std::endl;
    fout << "  ]";
  }

  fout << std::endl << "}" << std::endl;
  fout.close();
}
