#include <string>
#include <vector>
#include <cstdint>

class pass_through {

public:
  std::string ctd_info;
  std::string ctd_coefficients;
  std::string do_info;
  std::string do_coefficients;
  std::string ph_info;
  std::string eco_info;
  std::string ocr_info;
  std::string suna_info;

  void parse( std::vector<uint8_t> d);

};
