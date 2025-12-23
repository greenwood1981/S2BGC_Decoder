#include "bist.h"
#include <map>
#include <iostream>
#include <format>
#include "../output/write_log.h"

float ieee754(std::vector<uint8_t> d, int p) {
    // __builtin_bswap32(xx);
    return *reinterpret_cast<float *>(&d[p]);
}

void ctd_bist::parse(std::vector<uint8_t> d) {
  received = 1;
  status = ( d[6] << 8 ) + d[7];

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
  uint16_t flag;
  for (int p = 0; p < 12; p++) {
    flag = std::pow(2,p);
    if (status & flag) {
      log( std::format( " -- {}", CTD_Status_Flag[p]) );
    }
  }

  numErr = ( d[8] << 8 ) + d[9];
  volt_cnt = ( d[10] << 8 ) + d[11];
  p_cnt = ( d[12] << 8 ) + d[13];
  ma0 = ( d[14] << 8 ) + d[15];
  ma1 = ( d[16] << 8 ) + d[17];
  ma2 = ( d[18] << 8 ) + d[19];
  ma3 = ( d[20] << 8 ) + d[21];
};

void do_bist::parse(std::vector<uint8_t> d) {
  received = 1;
  status = ( d[6] << 8 ) + d[7];

  std::map<uint8_t,std::string> DO_Status_Flag = {
    {0,"DO Error parsing the return string"},
    {1,"DO failed all tries to parse"},
    {2,"DO timed out from inactivity"},
    {3,"DO not correct state when asked for sample"},
    {4,"DO bad start of string to parse"},
    {5,"DO emulation mode"}
  };
  uint16_t flag;
  for (int p = 0; p < 6; p++) {
    flag = std::pow(2,p);
    if (status & flag) {
      log( std::format( " -- {}", DO_Status_Flag[p]) );
    }
  }

  numErr = ( d[8] << 8 ) + d[9];
  volt_cnt = ( d[10] << 8 ) + d[11];
  maMax = ( d[12] << 8 ) + d[13];
  maAvg = ( d[14] << 8 ) + d[15];
  doPh = ieee754(d,16);
  thmV = ieee754(d,20);
  DO = ieee754(d,24);
  thmC = ieee754(d,28);
};

void ph_bist::parse(std::vector<uint8_t> d) {
  received = 1;
  status = ( d[6] << 8 ) + d[7];

  std::map<uint8_t,std::string> pH_Status_Flag = {
    {0,"pH Error parsing the return string"},
    {1,"pH failed all tries to parse"},
    {2,"pH timed out from inactivity"},
    {3,"pH not correct state when asked for sample"},
    {4,"pH bad start of string to parse"},
    {5,"pH emulation mode"}
  };
  uint16_t flag;
  for (int p = 0; p < 6; p++) {
    flag = std::pow(2,p);
    if (status & flag) {
      log( std::format( " -- {}", pH_Status_Flag[p]) );
    }
  }

  numErr = ( d[8] << 8 ) + d[9];
  volt_cnt = ( d[10] << 8 ) + d[11];
  maMax = ( d[12] << 8 ) + d[13];
  maAvg = ( d[14] << 8 ) + d[15];
  vRef = ieee754(d,16);
  Vk = ieee754(d,20);
  Ik = ieee754(d,24);
  Ib = ieee754(d,28);
};

void eco_bist::parse(std::vector<uint8_t> d) {
  received = 1;
  status = ( d[6] << 8 ) + d[7];

  std::map<uint8_t,std::string> eco_Status_Flag = {
    {0,"ECO Error parsing the return string"},
    {1,"ECO failed all tries to parse"},
    {2,"ECO timed out from inactivity"},
    {3,"ECO not correct state when asked for sample"},
    {4,"ECO bad start of string to parse"},
    {5,"ECO emulation mode"}
  };
  uint16_t flag;
  for (int p = 0; p < 6; p++) {
    flag = std::pow(2,p);
    if (status & flag) {
      log( std::format( " -- {}", eco_Status_Flag[p]) );
    }
  }

  numErr = ( d[8] << 8 ) + d[9];
  volt_cnt = ( d[10] << 8 ) + d[11];
  maMax = ( d[12] << 8 ) + d[13];
  maAvg = ( d[14] << 8 ) + d[15];
  ch01 = ( d[16] << 8 ) + d[17];
  ch02 = ( d[18] << 8 ) + d[19];
  ch03 = ( d[20] << 8 ) + d[21];
};

void ocr_bist::parse(std::vector<uint8_t> d) {
  received = 1;
  status = ( d[6] << 8 ) + d[7];

  std::map<uint8_t,std::string> ocr_Status_Flag = {
    {0,"OCR Error parsing the return string"},
    {1,"OCR failed all tries to parse"},
    {2,"OCR timed out from inactivity"},
    {3,"OCR not correct state when asked for sample"},
    {4,"OCR bad start of string to parse"},
    {5,"OCR emulation mode"}
  };
  uint16_t flag;
  for (int p = 0; p < 6; p++) {
    flag = std::pow(2,p);
    if (status & flag) {
      log( std::format( " -- {}", ocr_Status_Flag[p]) );
    }
  }

  numErr = ( d[8] << 8 ) + d[9];
  volt_cnt = ( d[10] << 8 ) + d[11];
  maMax = ( d[12] << 8 ) + d[13];
  maAvg = ( d[14] << 8 ) + d[15];
  ch01 = ( d[16] << 8 ) + d[17];
  ch02 = ( d[18] << 8 ) + d[19];
  ch03 = ( d[20] << 8 ) + d[21];
  ch04 = ( d[22] << 8 ) + d[23];
};

void no3_bist::parse_engineer(std::vector<uint8_t> d) {
  eng_received = 1;
  status = ( d[6] << 8 ) + d[7];

  std::map<uint8_t,std::string> NO3_Status_Flag = {
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
  uint16_t flag;
  for (int p = 0; p < 13; p++) {
    flag = std::pow(2,p);
    if (status & flag) {
      log( std::format( " -- {}", NO3_Status_Flag[p]) );
    }
  }

  numErr = ( d[8] << 8 ) + d[9];
  J_err     = ieee754(d,10); // Error counter
  J_rh      = ieee754(d,14); // Relative Humidity
  J_volt    = ieee754(d,18); // Measured voltage
  J_amps    = ieee754(d,22); // Measured amps
  J_darkM   = ieee754(d,26); // Dark spectrum mean
  J_darkS   = ieee754(d,30); // Dark spectrum stdDev
  J_no3     = ieee754(d,34); // Nitrate
  J_res     = ieee754(d,38); // Residual RMS
  J_fit1    = ieee754(d,42); // FIT Pixel Begin
  J_fit2    = ieee754(d,46); // FIT Pixel End
  J_Spectra = ieee754(d,50); // First spectrum channel value
  J_SeaDark = ieee754(d,54); // Seawater Dark, mean of channels 1-5
};

void no3_bist::parse_spectral(std::vector<uint8_t> d) {
	if ( d.size() < 22 ) {
		log(std::format(" * warning, BIST Nitrate spectrum packet is incomplete [{:d} bytes]; skipping",d.size()) );
		spectral_received = 0;
		return;
	}

	spectral_received = 1;

	spectral_count = (d[6]<<8) + d[7];

	// Contains NO3 spectral values S[k]
	for(int i = 8; i < 8 + (spectral_count*2); i++) {
		svals.push_back( (d[i]<<8) + d[i+1] ); // 2-byte integer counts of each spectral bin
	}
};

void no3_bist::parse_ascii(std::vector<uint8_t> d) {
	if ( d.size() < 22 ) {
		log(std::format(" * warning, BIST Nitrate ascii packet is incomplete [{:d} bytes]; skipping",d.size()) );
		ascii_received = 0;
		return;
	}

	ascii_received = 1;
	// ASCII string subset of the engineering values that the SUNA displays before the spectrum.
	// These should match the values in bisIDno3a
	uint16_t nDatSize = ((d[1] & 0x0F) << 8) + d[2];

	// exclude packet header (first 6) and termination char ';' (last)
	for(uint16_t i = 6; i < nDatSize - 1; i++) {
		SUNA_str.push_back((char)d[i]);
	}
	// Split ASCII string using ',' delimiter
	std::vector<float> result;
	std::stringstream ss(SUNA_str);
	std::string item;
	while (getline(ss,item,',')) {
		if (item.size())
			result.push_back(std::stof(item));
		else
			result.push_back(NAN);
	}
    // BG ascii_timestamp epoch appears to be seconds since 2000-01-01
	time_t epoch_1970 = 946684800 + result[1];
	ascii_timestamp = boost::posix_time::from_time_t(epoch_1970);
	ascii_pressure = result[2];
	ascii_temperature = result[3];
	ascii_salinity = result[4];
	ascii_sample_count = result[5];
	ascii_cycle_count = result[6];
	ascii_error_count = result[7];
	ascii_internal_temp = result[8];
	ascii_spectrometer = result[9];
	ascii_RH = result[10];
	ascii_voltage = result[11];
	ascii_current = result[12];
	ascii_ref_detector_mean = result[13];
	ascii_ref_detector_std = result[14];
	ascii_dark_spectrum_mean = result[15];
	ascii_dark_spectrum_std = result[16];
	ascii_sensor_salinity = result[17];
	ascii_sensor_nitrate = result[18];
	ascii_residual_rms = result[19];
	ascii_FIT_pixel_begin = result[20];
	ascii_FIT_pixel_end = result[21];
};

void ctd_bist::print() {
	std::cout << "status   = " << status << std::endl;
	std::cout << "numErr   = " << numErr << std::endl;
	std::cout << "volt_cnt = " << 0.01*volt_cnt << std::endl;
	std::cout << "p_cnt    = " << (0.04*p_cnt)-10 << std::endl;
	std::cout << "ma0      = " << ma0 << std::endl;
	std::cout << "ma1      = " << ma1 << std::endl;
	std::cout << "ma2      = " << ma2 << std::endl;
	std::cout << "ma3      = " << ma3 << std::endl;
};

void do_bist::print() {
	std::cout << "status   = " << status << std::endl;
	std::cout << "numErr   = " << numErr << std::endl;
	std::cout << "volt_cnt = " << 0.01*volt_cnt << std::endl;
	std::cout << "maMax    = " << maMax << std::endl;
	std::cout << "maAvg    = " << maAvg << std::endl;
	std::cout << "doPh     = " << doPh << std::endl;
	std::cout << "thmV     = " << thmV << std::endl;
	std::cout << "DO       = " << DO << std::endl;
	std::cout << "thmC     = " << thmC << std::endl;
};

void ph_bist::print() {
	std::cout << "status   = " << status << std::endl;
	std::cout << "numErr   = " << numErr << std::endl;
	std::cout << "volt_cnt = " << 0.01*volt_cnt << std::endl;
	std::cout << "maMax    = " << maMax << std::endl;
	std::cout << "maAvg    = " << maAvg << std::endl;
	std::cout << "vRef     = " << vRef << std::endl;
	std::cout << "Vk     = " << Vk << std::endl;
	std::cout << "Ik       = " << Ik << std::endl;
	std::cout << "Ib     = " << Ib << std::endl;
};

void eco_bist::print() {
	std::cout << "status   = " << status << std::endl;
	std::cout << "numErr   = " << numErr << std::endl;
	std::cout << "volt_cnt = " << 0.01*volt_cnt << std::endl;
	std::cout << "maMax    = " << maMax << std::endl;
	std::cout << "maAvg    = " << maAvg << std::endl;
	std::cout << "ch01     = " << ch01 << std::endl;
	std::cout << "ch02     = " << ch02 << std::endl;
	std::cout << "ch03     = " << ch03 << std::endl;
};

void ocr_bist::print() {
	std::cout << "status   = " << status << std::endl;
	std::cout << "numErr   = " << numErr << std::endl;
	std::cout << "volt_cnt = " << 0.01*volt_cnt << std::endl;
	std::cout << "maMax    = " << maMax << std::endl;
	std::cout << "maAvg    = " << maAvg << std::endl;
	std::cout << "ch01      = " << ch01 << std::endl;
	std::cout << "ch02       = " << ch02 << std::endl;
	std::cout << "ch03     = " << ch03 << std::endl;
	std::cout << "ch04     = " << ch04 << std::endl;
};

void no3_bist::print() {
	std::cout << "status    = " << status << std::endl;
	std::cout << "numErr    = " << numErr << std::endl;
	std::cout << "J_err     = " << J_err << std::endl;
	std::cout << "J_rh      = " << J_rh << std::endl;
	std::cout << "J_volt    = " << J_volt << std::endl;
	std::cout << "J_amps    = " << J_amps << std::endl;
	std::cout << "J_darkM   = " << J_darkM << std::endl;
	std::cout << "J_darkS   = " << J_darkS << std::endl;
	std::cout << "J_Nitrate = " << J_no3 << std::endl;
	std::cout << "J_res     = " << J_res << std::endl;
	std::cout << "J_fit1    = " << J_fit1 << std::endl;
	std::cout << "J_fit2    = " << J_fit2 << std::endl;
	std::cout << "J_Spectra = " << J_Spectra << std::endl;
	std::cout << "J_SeaDark = " << J_SeaDark << std::endl;
	for(int i = 0; i < 8; i++)
		std::cout << "svals[" << i << "] = " << svals[i] << std::endl;
	std::cout << "SUNA_str = " << SUNA_str << std::endl;
};
