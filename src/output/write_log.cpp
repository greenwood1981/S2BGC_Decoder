#include "write_log.h"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <chrono>  // chrono::system_clock
#include <ctime>
#include <sstream> // stringstream
#include <string>
#include <algorithm>
#include <format>

//#include "../json/json.hpp"

//using json = nlohmann::ordered_json;
//extern json config;

extern std::string S2BGC_PATH;

std::string logfile() {
	auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	std::tm tm = *std::gmtime(&now);
    std::string logfile = std::format("{:s}/log/{:04d}{:02d}{:02d}.log",S2BGC_PATH,tm.tm_year+1900,tm.tm_mon+1,tm.tm_mday);
    logfile.erase(std::remove(logfile.begin(),logfile.end(),'"'),logfile.end());
	return logfile;
}

std::string timestamp() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);

    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %H:%M:%S ");
    return ss.str();
}

void log(std::string s) {
  std::ofstream log(logfile(),std::ios::out | std::ios::app);
  if (log.fail()) {
    std::cout << "Error, unable to open logfile " << logfile() << std::endl;
    return;
  }
  log << timestamp() << s << std::endl; // write to log, include timestamp
  std::cout << s << std::endl;          // also write to stdout (without timestamp)
  log.close();
}

