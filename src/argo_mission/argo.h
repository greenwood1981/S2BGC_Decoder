#ifndef _MISSION_H
#define _MISSION_H

#include <vector>
#include <cstdint>

class Argo_Mission {
public:
  Argo_Mission();
  void Decode(std::vector<uint8_t> &data);
  void compute_cycle_time_max(); // estimate cycle time given mission parameters

  int16_t received;
  uint16_t packet_version;
  uint16_t float_version;
  uint16_t mission_number;
  uint16_t ascent_speed_target;
  uint16_t prof_depth_target;
  uint16_t park_depth_target;
  double rise_time_max;
  double fall2park_time_max;
  double park2prof_time_max;
  double drift_time_target;
  uint16_t num_seeks;
  int16_t pres_gain, pres_offset;
  int16_t temp_gain, temp_offset;
  int16_t psal_gain, psal_offset;
  double surf_time_max;
  double cycle_time_max;
  double seek_time_max;
  double firmware_version;
};

#endif
