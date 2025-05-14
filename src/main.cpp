#include "hexfile/hexfile.h"
#include "output/write_log.h"
#include <cstdlib>
#include <string>
#include <format>
#include <boost/filesystem.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

using namespace boost::filesystem;

#include "json/json.hpp"
using json = nlohmann::ordered_json;
json config;

std::string S2BGC_PATH;

int main( int argc, char **argv) {

  // Read decoder config file
  const char *s2bgc_path = getenv("S2BGC_PATH");
  if (s2bgc_path == NULL) {
	// If S2BGC_PATH env variable is not defined, exit. Write to stdout becuase log() requires S2BGC_PATH
    std::cout << "Unable to load S2BGC_PATH environment variable; exiting." << std::endl;
    return 0;
  }
  S2BGC_PATH = s2bgc_path;
  std::ifstream f(S2BGC_PATH + "/config/config.json");
  if (!f) {
	// If config.json is not found, exit. Write to stdout because log() requires S2BGC_PATH
    std::cout << "Unable to open config.json; exiting." << std::endl;
	return 0;
  }
  config = json::parse(f);

  log("Starting BGC-SOLO");

  // Command-line options
  std::string filter;
  if (argc == 3) {
    filter = std::format("{:04d}_{:03d}.hex",std::stoi(argv[1]),std::stoi(argv[2])); // S2BGC_Decoder [sn] [cycle]
    log( std::format("* Using filter: {}",filter));
  }

  // read and process each hex file in hex directory
  std::string hexdir = S2BGC_PATH + "/incoming";
  std::vector<std::filesystem::path> hexfiles;
  std::copy(std::filesystem::directory_iterator(hexdir),std::filesystem::directory_iterator(),std::back_inserter(hexfiles));
  std::sort(hexfiles.begin(),hexfiles.end());
  for (const std::filesystem::path &filepath : hexfiles) {
    //log(std::string(" - processing ") + filepath.string() );
    if ( filter.length() ) {
		if (filter != filepath.filename().string())
			continue;
	}
    hexfile h(filepath.string());
    h.Decode();
	h.archive();
   	h.write_JSON();

  }

  log("Finished.");
  return 0;
}
