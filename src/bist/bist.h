#ifndef BIST_H
#define BIST_H

#include <boost/date_time.hpp>
#include <vector>
#include <string>

class ctd_bist {
public:
	bool received;
	uint16_t status;
	uint16_t numErr;
	uint16_t volt_cnt;
	uint16_t p_cnt;
	uint16_t ma0;
	uint16_t ma1;
	uint16_t ma2;
	uint16_t ma3;
	ctd_bist() : received(0) {}
	void parse(std::vector<uint8_t> d);
	void print();
};

class do_bist {
public:
	bool received;
	uint16_t status;
	uint16_t numErr;
	uint16_t volt_cnt;
	uint16_t maMax;
	uint16_t maAvg;
	float doPh;
	float thmV;
	float DO;
	float thmC;
	do_bist() : received(0) {}
	void parse(std::vector<uint8_t> d);
	void print();
};

class ph_bist {
public:
	bool received;
	uint16_t status;
	uint16_t numErr;
	uint16_t volt_cnt;
	uint16_t maMax;
	uint16_t maAvg;
	float vRef;
	float Vk;
	float Ik;
	float Ib;
	ph_bist() : received(0) {}
	void parse(std::vector<uint8_t> d);
	void print();
};

class eco_bist {
public:
	uint16_t status;
	uint16_t numErr;
	uint16_t volt_cnt;
	uint16_t maMax;
	uint16_t maAvg;
	uint16_t Chl;
	uint16_t bb;
	uint16_t CDOM;
	bool received;
	eco_bist() : received(0) {}
	void parse(std::vector<uint8_t> d);
	void print();
};

class ocr_bist {
public:
	uint16_t status;
	uint16_t numErr;
	uint16_t volt_cnt;
	uint16_t maMax;
	uint16_t maAvg;
	uint16_t ch01;
	uint16_t ch02;
	uint16_t ch03;
	uint16_t ch04;
	bool received;
	ocr_bist() : received(0) {}
	void parse(std::vector<uint8_t> d);
	void print();
};

class no3_bist {
public:
	uint16_t status;
	uint16_t numErr;
	float J_err;
	float J_rh;
	float J_volt;
	float J_amps;
	float J_darkM;
	float J_darkS;
	float J_no3;
	float J_res;
	float J_fit1;
	float J_fit2;
	float J_Spectra;
	float J_SeaDark;
	uint16_t svals[8];
	std::string SUNA_str;
	boost::posix_time::ptime ascii_timestamp;
	float ascii_pressure;
	float ascii_temperature;
	float ascii_salinity;
	uint32_t ascii_sample_count;
	uint32_t ascii_cycle_count;
	uint32_t ascii_error_count;
	float ascii_internal_temp;
	float ascii_spectrometer;
	float ascii_RH;
	float ascii_voltage;
	float ascii_current;
	uint32_t ascii_ref_detector_mean;
	uint32_t ascii_ref_detector_std;
	uint32_t ascii_dark_spectrum_mean;
	uint32_t ascii_dark_spectrum_std;
	float ascii_sensor_salinity;
	float ascii_sensor_nitrate;
	float ascii_residual_rms;
	uint32_t ascii_FIT_pixel_begin;
	uint32_t ascii_FIT_pixel_end;
	bool eng_received;
	bool spectral_received;
	bool ascii_received;
	no3_bist() : eng_received(0), spectral_received(0), ascii_received(0) {}
	void parse_engineer(std::vector<uint8_t> d);
	void parse_spectral(std::vector<uint8_t> d);
	void parse_ascii(std::vector<uint8_t> d);
	void print();
};

#endif
