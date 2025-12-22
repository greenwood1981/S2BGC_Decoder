#include <vector>
#include <string>
#include <cstdint>

struct BGC_SOLO_parameter {
  std::string name;
  std::string unit;
  std::string desc;
  std::string val;
};

// Engineering and Diagnostic parameters
class Engineering_Data {
public:
  std::vector<BGC_SOLO_parameter> list;
  void parse_pfile(std::vector<uint8_t>d, std::string pfile);
};

// Mission parameters
class mission {
public:
  std::vector<BGC_SOLO_parameter> list;
  void parse(std::vector<uint8_t>d);
};
