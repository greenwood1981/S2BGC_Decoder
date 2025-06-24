#include "argo.h"
//#include <iostream>
//#include <iomanip>
#include <format>
#include <list>
#include "../output/write_log.h"

#include "../json/json.hpp"

using json = nlohmann::ordered_json;
extern json config;

Argo_Mission::Argo_Mission() {
  // Argo_Mission Constructor used to set default mission values
  received = 0;
  packet_version = -1;
  firmware_version = -1;
  float_version = 99;
  mission_number = 1;
  prof_depth_target = 150;
  park_depth_target = 150;
  rise_time_max = 180;
  fall2park_time_max = 360;
  park2prof_time_max = 0;
  drift_time_target = 0;
  ascent_speed_target = 14;
  num_seeks = 1;
  surf_time_max = 0;
  seek_time_max = 0;
  // default CTD gain and offsets
  pres_gain = 25;
  pres_offset = 10;
  temp_gain = 1000;
  temp_offset = 5;
  psal_gain = 5000;
  psal_offset = -25;
  cycle_time_max = 0;
}

void Argo_Mission::Decode(std::vector<uint8_t> &data) {
  received = 1;
  packet_version = (data[0] & 0xf0) >> 4;
  int pnum = data[0] & 0x0f;
  log( std::format("Packet[{:2X}] Argo Mission ",pnum) );
  firmware_version = data[5] + ( 0.1 * data[6] );
  prof_depth_target = (data[7]<<8) + data[8];
  park_depth_target = (data[9]<<8) + data[10];
  rise_time_max = (double)((data[11]<<8) + data[12])/60.0;
  fall2park_time_max = (double)((data[13]<<8) + data[14])/60.0;
  park2prof_time_max = (double)((data[15]<<8) + data[16])/60.0;
  drift_time_target = (double)((data[17]<<8) + data[18])*5.0/60.0;
  float_version = data[19];
  ascent_speed_target = data[20]; // cm/s
  num_seeks = (data[21]<<8) + data[22]; // number of seek periods
  surf_time_max = double((data[23]<<8) + data[24]) / 3600.0; // hours
  seek_time_max = double((data[25]<<8) + data[26]) / 60.0;   // hours
  pres_gain = (data[27]<<8) + data[28];
  pres_offset = (data[29]<<8) + data[30];
  temp_gain = (data[31]<<8) + data[32];
  temp_offset = (data[33]<<8) + data[34];
  psal_gain = (data[35]<<8) + data[36];
  psal_offset = (data[37]<<8) + data[38];

  // CTD gains and offsets used for CTD Discrete,CTD Binned, CTD Drift profiles as well as engineering drift averages
  for(const auto &profile : std::list<std::string>({"CTD Discrete","CTD Binned","CTD Drift"})) {
    config["prof"][profile]["PRES"]["gain"] = pres_gain;
    config["prof"][profile]["PRES"]["offset"] = pres_offset;
    config["prof"][profile]["TEMP"]["gain"] = temp_gain;
    config["prof"][profile]["TEMP"]["offset"] = temp_offset;
    config["prof"][profile]["PSAL"]["gain"] = psal_gain;
    config["prof"][profile]["PSAL"]["offset"] = psal_offset;
  }

  compute_cycle_time_max();
  log( std::format("  {:d}m park, {:d}m profile, {:.2f} days",park_depth_target,prof_depth_target,cycle_time_max) );
}

void Argo_Mission::compute_cycle_time_max() {
  // profile depth [m] / ascent speed [cm/s] x 100 [cm/m] / 3600 [s/hr] = rise time [hr]
  //std::cout << "rise time: min(" << rise_time_max << "," << 100./3600.*prof_depth_target/ascent_speed_target << ")" << std::endl;
  //std::cout << "seek: " << seek_time_max << " x " << num_seeks << " = " << seek_time_max*num_seeks << std::endl;
  //std::cout << "hours = " << std::min(rise_time_max,100./3600.*prof_depth_target/ascent_speed_target) + fall2park_time_max
  //  + drift_time_target + park2prof_time_max + (seek_time_max*num_seeks) + surf_time_max << std::endl;

  cycle_time_max = (std::min(rise_time_max,100./3600.*prof_depth_target/ascent_speed_target) + fall2park_time_max
    + drift_time_target + park2prof_time_max + (seek_time_max*num_seeks) + surf_time_max) / 24; // convert hours to days
}

