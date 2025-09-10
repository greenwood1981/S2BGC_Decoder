#include <iostream>
#include <fstream>
#include <regex>
#include <sstream>
#include <cmath>
#include <vector>
#include <string>
#include <filesystem>
#include <boost/date_time.hpp>
using namespace boost::posix_time;
#include "../hexfile/hexfile.h"
#include "../json/json.hpp"  // Include the Nlohmann JSON library
#include <iomanip>

//global variables
using json = nlohmann::ordered_json;
json config_template;
json mission_template;
json RunMission;
json RunSCI;
json GPS;
std::vector<nlohmann::json> GPSobj;
int float_type = 2; //default to BGC for starters (cycle -1,0 do NOT have ARGO_Mission)
float float_version = 10.1; //default to original BGC for starters (cycle -1,0 do NOT have ARGO_Mission)
std::string L1_path;
std::string L0_path;
int rest=3;  //default temperature resolution
int resp=2;  //default pressure resolution
int ress=3;  //default salinity resolution

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

/////////////////////////////////////////////////////////////////////////////////////////////
// Function Declaration 
void write_CTD (std::ofstream& fout, std::string varname, json Doc_L0c, int chan);

// Function Declaration 
void write_BGC (std::ofstream& fout, std::vector<int> Snd, std::string varname, json Doc_L0c);

// Function Declaration 
void write_BGC_Drift (std::ofstream& fout, std::vector<int> Snd, std::string varname, int dd, int sd, json Doc_L0c);

// Function Declaration 
void copyLine ( std::ofstream& fout, std::string item, std::string endc, json Doc_L0c);

// Function Declaration 
int convert_to_hours (float value);

// Function Declaration 
int determine_resolution (int digits);

// Function Declaration 
std::vector<int> ApplyDataT (std::string sensor, int dataT);

// Function Declaration 
void rewrite_json (string filename);

// Function Declaration
void IdentEchoConfig(int SoloType, string command);

// Function Declaration
void ModOneConfig(string ConfigName, int ConfigValue, string unit, string desc);

// Function Declaration
int ModSCI(string command);
/////////////////////////////////////////////////////////////////////////////////////////////


int main( int argc, char* argv[] ) {

  const char *s2BGC_PATH = getenv("S2BGC_PATH");
  if (s2BGC_PATH == NULL) {
        // If S2BGC_PATH env variable is not defined, exit. Write to stdout becuase log() requires S2BGC_PATH
    std::cout << "Unable to load S2BGC_PATH environment variable; exiting." << std::endl;
    return 0;
  }

  std::cout << "Program Name: " << argv[0] << std::endl;

  int tarcynum;
  if (argc > 1) {
    std::cout << "Number of arguments: " << argc - 1 << std::endl;
    std::cout << "Arguments:" << std::endl;
    for (int i = 1; i < argc; ++i) {
      std::cout << "  Argument " << i << ": " << argv[i] << std::endl;
    }
    if ( argc - 1 == 1 ) {
      std::cout << "  No target cycle number specified -> setting to -1  " << std::endl;
      tarcynum = -1;
    } else {
      tarcynum = std::stoi(argv[2]); 
    }
  } else {
    std::cout << "argv[1] (mandatory): float SN; argv[2] (optional): CONFIG/SCI target cycle number to process" << std::endl;
    return 0;
  }

  std::string s2bgc_path = s2BGC_PATH;

  // LOAD Bens mission.json template for units/description
  string missname = s2bgc_path + "/config/mission.json"; //path of original L0 json
  std::ifstream tempmiss(missname, std::ifstream::binary);
  tempmiss >> mission_template; //json of mission template (global)
  string configname = s2bgc_path + "/config/config.json"; //path of original L0 json
  std::ifstream tempconfig(configname, std::ifstream::binary);
  tempconfig >> config_template; //json of config template (global)
  

  L0_path = s2bgc_path + "/data/" + argv[1] + "/json"; //path of original L0 json
  L1_path = s2bgc_path + "/data/" + argv[1] + "/L1"; //path of new L1 json
  // std::cout << L0_path << std::endl;
  // std::cout << L1_path << std::endl;

  // Sort L0 JSON files: Done early to allow L0 file to be Mission Target file
  std::vector<std::filesystem::path> files_in_directory;
  try {
         for (const auto& entry : std::filesystem::directory_iterator(L0_path)) {
            if (entry.is_regular_file()) {
                if (entry.path().extension() == ".json") {
                        files_in_directory.push_back(entry.path());
                }
            }
         }
  } catch (const std::filesystem::filesystem_error& ex) {
          std::cerr << "Filesystem error: " << ex.what() << std::endl;
  }
 
 //exit if there are no files in L0 directory (Can this happen?)
  if ( files_in_directory.size() == 0 ) {
	  std::cout << "No json files in L0 directory " << std::endl;
	  return 0;
  }

  std::sort(files_in_directory.begin(), files_in_directory.end()); //sort the L0 file names

  std::string first_L0_file;  // Identify the first file in L0 directory for targetfile if no L1 exist
  for (const std::string& filename : files_in_directory) {  
	if (first_L0_file.empty()) {
  	  first_L0_file = filename;
	  break;
	}
  }

  std::string tarfilename; //Sort L1 JSON files, choose Target...file to start with
  int start_iter=9999;
  try {  //create directory for new json if necessary + read in L1 directory for reference
    if (std::filesystem::create_directory(L1_path)) {
      std::cout << "  Directory '" << L1_path << "' created successfully." << std::endl;
      tarcynum = -1; //there was no directory so target defaults to -1 from L0
      tarfilename = first_L0_file;
      start_iter=-1;
    } else { 
      std::cout << "  Directory '" << L1_path << "' already exists." << std::endl;
  
      // Sort L1 JSON files within directory: Pull out starting "Mission" and "SCI"
      std::vector<std::filesystem::path> files_in_L1_directory;
      try {
    	  for (const auto& entry : std::filesystem::directory_iterator(L1_path)) {
      	      if (entry.is_regular_file()) {
       		   if (entry.path().extension() == ".json") {
               		 files_in_L1_directory.push_back(entry.path());
               	   }
       	      }
       	  }
      } catch (const std::filesystem::filesystem_error& ex) {
       	  std::cerr << "Filesystem error: " << ex.what() << std::endl;
      }

      if ( files_in_L1_directory.size() == 0 ) {
        tarcynum = -1; //there were no files in the L1 directory: target defaults to -1 from L0
        tarfilename = first_L0_file;
        start_iter=-1;

      } else { // L1 files already exists
        std::sort(files_in_L1_directory.begin(), files_in_L1_directory.end());
// find the latest L1 cycle to load prior or equal to tarcynum
        std::string delimiters("_."); //two delimiters in typical json filename
        std::vector<std::string> token;
        auto it = files_in_L1_directory.end()-1; //search in reverse order 
        while (it >= files_in_L1_directory.begin() & start_iter > tarcynum) {
  	  boost::split(token, it->filename().string(), boost::is_any_of(delimiters));
//	  std::cout << token[token.size()-2] <, std::endl;  
	  start_iter = stoi(token[token.size()-2]);  //assume that cycle number is 2nd to last token
	  --it;
        }
        it++; //increase iterator by 1
        tarfilename = L1_path + "/" + it->filename().string();
      }
    }
  

    std::cout << "  Reference Mission/SCI is from  " << tarfilename << std::endl;
    std::ifstream json_file(tarfilename, std::ifstream::binary);
    json Doc_L1;
    json_file >> Doc_L1;
    if ( Doc_L1.contains(std::string{ "Mission" }) ) {
      RunMission = Doc_L1.at("Mission"); //this is the starting point for Mission
    } else {
      std::cout << "There is no MISSION in target file: exit " << tarfilename << std::endl;
      //  return 0;
    }
    if ( Doc_L1.contains(std::string{ "SCI_Parameters" }) ) {
      RunSCI = Doc_L1.at("SCI_Parameters"); //this is the starting point for SCI
    } else {
      std::cout << "There is no SCI_Parameters in target file: exit " << tarfilename << std::endl;
      // return 0;
    } 
  } catch (const std::filesystem::filesystem_error& ex) {
      std::cerr << "Filesystem error: " << ex.what() << std::endl;
  }

  std::cout << "  Writing of L1 will start from cycle " << start_iter << std::endl;


//Do first loop to pull in all GPS records for rearranging
// Store in vector array GPS, of json GPSobj with cycle addon
  for (const std::string& filename : files_in_directory) {

//    std::cout << filename << std::endl;
    std::string delimiters("_."); //two delimiters in typical json filename
    std::vector<std::string> token;
    boost::split(token, filename, boost::is_any_of(delimiters));
    int check_iter = stoi(token[token.size()-2]);  //assume that cycle number is 2nd to last token
   
    json Doc_L0; //create uninitialized json object
    std::ifstream json_file(filename, std::ifstream::binary);
    json_file >> Doc_L0; // initialize json object with input from oringal file

    if ( Doc_L0.contains(std::string{ "GPS" }) ) {
      GPS["cycle"]=check_iter;
      GPS["data"]=Doc_L0.at("GPS");
      GPSobj.push_back(GPS);
    }
//    for (const auto& obj : GPSobj) {
//        std::cout << obj.dump(4) << std::endl; // Pretty print the JSON object
//    }

  }

  for (const std::string& filename : files_in_directory) {
//   	std::cout << "\n" << std::endl; 
//    	std::cout << filename << std::endl; // printed in alphabetical order
 

        std::string delimiters("_."); //two delimiters in typical json filename
        std::vector<std::string> token;
        boost::split(token, filename, boost::is_any_of(delimiters));
	int check_iter = stoi(token[token.size()-2]);  //assume that cycle number is 2nd to last token

// process if cycnum of filename is > tarcynum
	if ( check_iter >= start_iter ) {
    	  std::cout << std::endl; 
    	  std::cout << "Reading " << filename << std::endl; // printed in alphabetical order

  	  json Doc_L0; //create uninitialized json object

	  std::ifstream json_file(filename, std::ifstream::binary);
	  json_file >> Doc_L0; // initialize json object with input from oringal file

  	  if ( Doc_L0.contains(std::string{ "ARGO_Mission" }) ) {
	    std::string unit="";
	    std::string desc="";
            float_type = Doc_L0.at("ARGO_Mission")["Float_Version"]; // save the float type for use of this code with non-BGC
            float_version = Doc_L0.at("ARGO_Mission")["firmware_version"]; // save the float version for use of this code with non-BGC
            resp=determine_resolution(Doc_L0.at("ARGO_Mission")["ctd_pres"]["gain"]);
            rest=determine_resolution(Doc_L0.at("ARGO_Mission")["ctd_temp"]["gain"]);
            ress=determine_resolution(Doc_L0.at("ARGO_Mission")["ctd_psal"]["gain"]);
            ModOneConfig("MinRis",Doc_L0.at("ARGO_Mission")["min_ascent_rate"],unit,desc);
            int float_check; //as long as ARGO_Mission values has been modified from minutes to hours, must keep convert_to_hours here
	    if ( Doc_L0.contains(std::string{ "Engineering_Data" }) ) {  //in order to apply must get subcycle from engineering
              int binMod = Doc_L0.at("Engineering_Data")["binMod"]["value"];
       	      int subcyc = ( binMod % 8 );  //"binMod" contains subcy and CTD binning (modulus 8)
      	      ModOneConfig("Zpro" + std::to_string(subcyc),Doc_L0.at("ARGO_Mission")["profile_target"],unit,desc);
              ModOneConfig("Ztar" + std::to_string(subcyc),Doc_L0.at("ARGO_Mission")["drift_target"],unit,desc);
              float_check = convert_to_hours(Doc_L0.at("ARGO_Mission")["max_rise_time"]);
	      ModOneConfig("Rise" + std::to_string(subcyc),float_check,unit,desc);
              float_check = convert_to_hours(Doc_L0.at("ARGO_Mission")["max_fall_to_park"]);
	      ModOneConfig("Fall" + std::to_string(subcyc),float_check,unit,desc);
              float_check = convert_to_hours(Doc_L0.at("ARGO_Mission")["max_fall_to_profile"]);
              ModOneConfig("Pwait" + std::to_string(subcyc),float_check,unit,desc);
	    }
            ModOneConfig("Nseek",Doc_L0.at("ARGO_Mission")["seek_periods"],unit,desc);
            float_check = convert_to_hours(Doc_L0.at("ARGO_Mission")["seek_time"]);
            ModOneConfig("STLmin",float_check,unit,desc);
            ModOneConfig("Sgain",Doc_L0.at("ARGO_Mission")["ctd_psal"]["gain"],unit,desc);
            ModOneConfig("Soff",Doc_L0.at("ARGO_Mission")["ctd_psal"]["offset"],unit,desc);
            ModOneConfig("Tgain",Doc_L0.at("ARGO_Mission")["ctd_temp"]["gain"],unit,desc);
            ModOneConfig("Toff",Doc_L0.at("ARGO_Mission")["ctd_temp"]["offset"],unit,desc);
            ModOneConfig("Pgain",Doc_L0.at("ARGO_Mission")["ctd_pres"]["gain"],unit,desc);
            ModOneConfig("Poff",Doc_L0.at("ARGO_Mission")["ctd_pres"]["offset"],unit,desc);
	  }

	  // write file after ARGO_Mission but before others
          rewrite_json(filename);   //write json file here prior to looking at CONFIG dumps or 2-way

	  if ( Doc_L0.contains(std::string{ "SCI_Parameters" }) ) { 
//SCI_Parameters is sent in a single Iridium message, thus I do not believe there is any case where it is partially transmitted.  Because of this go with full overwrite (this is in contrast to CONFIG)
                RunSCI = Doc_L0.at("SCI_Parameters"); //over write running SCI with the file
	  }

	  if ( Doc_L0.contains(std::string{ "Upload_Command" }) ) {
	        std::string UpC = Doc_L0.at( "Upload_Command" );
		std::cout << "Upload_Command = " << UpC  << std::endl;

		std::stringstream ss(UpC);
                std::string segment;	
	        std::vector<std::string> commandVector;  // Declare a vector to hold substrings (commands)
                while (std::getline(ss,segment)) {
                  std::size_t prev=0, pos;
		  while ((pos = segment.find_first_of(";:",prev)) != std::string::npos) { //identify segments separated by ";" AND ":"
	  	    if (pos > prev && segment.substr(pos,1) != ":")  //Only save commands that end in ";"
			  commandVector.push_back(segment.substr(prev,pos-prev));
		    prev = pos+1;
		  }
		}
                for (const std::string& command : commandVector) {
			IdentEchoConfig(std::min(float_type,2),command); // Call routine to identify CONFIG and values in command
		}

	  }

	  if ( Doc_L0.contains(std::string{ "Mission" }) ) {
                json NewMission = Doc_L0.at("Mission"); //this is a new Mission to compare against
// loop through Mission Configs individually as Iridium transmission might be partial;
		for (const auto& loop : NewMission.items())  {
// 	           std::cout << loop.key() << " = " << loop.value()["value"] << "\n";
                   ModOneConfig(loop.key(),loop.value()["value"],loop.value()["unit"],loop.value()["description"]);
		}
	  }


 	} // end of loop over files > tarcynum
  }
  return 0;
}  // end of Main
	
/////////////////////////////////////////////////////////////////////////////////////////////
// Function definition
void IdentEchoConfig(int SoloType, string command) {


// return if command is length 1 : No CONFIG mods
	if (command.length() < 2) return;

// outputs the command sent to this subroutine
//	std::cout << "In IdentEchoConfig \"" << command << "\"" << std::endl;

        char Cp = command.at(0);  // The first character is the primary command
	std::string Cs = std::string(1, command.at(1)); // Secondary command (save as string to be able to 'add' to Config vectors

	std::vector<std::string> config_strings{};
	std::vector<std::string> sci_strings{};
        std::string Cb = "  ";
	switch (Cp) { 
          case '4': {
		if ( Cs == "S" ) {    // BGC Science Board CONFIGS
		    Cb = command.substr(3,2); // BGC Board command
		    if ( Cb == "2E" ) {
	              std::stringstream ss(command.substr(5,command.length()));
		      std::string token;
		      ss >> token;
		      config_strings.push_back(token);
                      command.erase(1,token.length()+5);  // remove CONFIG name & "S 2E" from command, parse values below as a standard SOLOII CONFIG eg "4 Zpro2 XX"
		    } else if ( Cb == "S1" ) {
		      std::cout << "Science Board change single sensor parameter for 1 sensor =" << Cb << std::endl;
                      command.erase(0,4);  // remove CONFIG name & "4S S" from command, parse values below
		    } else if ( Cb == "S2" ) {
		      std::cout << "Science Board change regional parameter for 5 regions for 1 sensor =" << Cb << std::endl;
                      command.erase(0,4);  // remove CONFIG name & "4S S" from command, parse values below
		    } else if ( Cb == "S3" ) {
		      std::cout << "Science Board change regional parameter for 1 region for 1 sensor =" << Cb << std::endl;
                      command.erase(0,4);  // remove CONFIG name & "4S S" from command, parse values below
		    } else if ( Cb == "S4" ) {
		      std::cout << "Science Board change sensor variable parameter for 1 sensor =" << Cb << std::endl;
                      command.erase(0,4);  // remove CONFIG name & "4S S" from command, parse values below
		    } else {
		      std::cout << "Science Board PSM Odd: Ignore Command? = " << Cb << std::endl;
		      return;
		    }
		} else {   // Float CONFIGS
	            std::stringstream ss(command.substr(2,command.length()));
		    std::string token;
		    ss >> token;
		    config_strings.push_back(token);
                    command.erase(2,token.length());  // remove CONFIG name from command, parse values below
		}
		break;
		    }
          case 'B': {
                config_strings.push_back("BinMod"); 
		break;
		    }
          case 'C': {
                config_strings.push_back("PROup"); 
                config_strings.push_back("BLOK"); 
                config_strings.push_back("PB1"); 
                config_strings.push_back("PB2"); 
                config_strings.push_back("AV1"); 
                config_strings.push_back("AV2"); 
                config_strings.push_back("CTDofZ"); 
		break;
		    }
          case 'D': {
                config_strings.push_back("SAMmn"+Cs); //must append sub-cycle
                config_strings.push_back("Nsam"+Cs); 
		break;
		    }
          case 'F': {
                config_strings.push_back("SkSLsc"); 
                config_strings.push_back("AsSLsc"); 
		break;
		    }
          case 'G': {
                config_strings.push_back("Pass"); 
                config_strings.push_back(Cs+"Gain"); //must prepend P/S/T
                config_strings.push_back(Cs+"Off"); 
		break;
		    }
          case 'H': {
                config_strings.push_back("MxHiP");
                config_strings.push_back("PmpBtm");
                config_strings.push_back("Pmpslo");
                config_strings.push_back("OILvac");
                config_strings.push_back("MnSfP");
                config_strings.push_back("VlvDly");
		break;
		    }
          case 'I': {
                config_strings.push_back("ABcymn"); 
		break;
		    }
          case 'J': {
                config_strings.push_back("DrfDat"); 
		break;
		    }
          case 'N': {
                config_strings.push_back("Ndives");
		break;
		    }
          case 'S': {
                config_strings.push_back("GPSsec"); 
                config_strings.push_back("IRIsec"); 
                config_strings.push_back("MxSfP"); 
		break;
		    }
          case 'T': {
                config_strings.push_back("Fall"+Cs); //must append sub-cycle
                config_strings.push_back("Rise"+Cs);
                config_strings.push_back("Pwait"+Cs);
		break;
		    }
          case 'U': {
                config_strings.push_back("UNBINd"); 
                config_strings.push_back("UBZmax"); 
                config_strings.push_back("UBn"); 
		break;
		    }
          case 'V': {
                config_strings.push_back("Ventop"); 
                config_strings.push_back("Ventsc"); 
		break;
		    }
          case 'W': {
                config_strings.push_back("WD_EN");
                config_strings.push_back("WDhour");
		break;
		    }
          case 'X': {
                config_strings.push_back("XP0"); 
                config_strings.push_back("XP1"); 
                config_strings.push_back("XP2"); 
                config_strings.push_back("XPdly"); 
                config_strings.push_back("XP0dZ"); 
		break;
		    }
          default: 

// for commands that differ by float type 
//            std::cout << SoloType << " SoloType " << std::endl;
	    switch (SoloType) { // Pass the float type code (SOLO=0, Deep=1, BGC=2)
	      case 0: {  //SOLOII
//    	        std::cout << "examining SOLO2 specific commands " << std::endl;
	        switch (Cp) { // The first character is the primary command
                  case 'E': {
                    config_strings.push_back("Ice_Tc"); 
                    config_strings.push_back("Ice_Ps"); 
                    config_strings.push_back("Ice_Pd"); 
                    config_strings.push_back("Ice_Mn"); 
                    config_strings.push_back("Ice_Sc"); 
		    break;
			    }
                  case 'L': {
                    config_strings.push_back("SrfDft"); 
                    config_strings.push_back("SrfLon"); 
                    config_strings.push_back("SrfLat"); 
                    config_strings.push_back("SrfMxN"); 
                    config_strings.push_back("SrfInt"); 
	            break;
			    }
                  case 'Z': {
                    config_strings.push_back("Cyc"+Cs); //must append sub-cycle
                    config_strings.push_back("Ztar"+Cs); 
                    config_strings.push_back("Zpro"+Cs); 
                    config_strings.push_back("Tlast"+Cs); 
		    break;
		            }
                  case '#': {
                    config_strings.push_back("Nseek");
                    config_strings.push_back("STLmin");
                    config_strings.push_back("dTadZ");
                    config_strings.push_back("dTsdZ");
                    config_strings.push_back("TlastCF");
	            break;
		            }
                  default: 
	    	    break;
                            } //end of Cp switch
              }  // end of case 0 (SOLO2)
	      case 1: { //Deep SOLO 
//    	        std::cout << "examining DEEP SOLO specfic commands " << std::endl;
	        switch (Cp) { // The first character is the primary command
//                case 'Z': { // firware V 0.5 and prior
//                  config_strings.push_back("Cyc"+Cs); //must append sub-cycle
//                  config_strings.push_back("Zpark"+Cs); 
//                  config_strings.push_back("Zpro"+Cs); 
//                  config_strings.push_back("Clast"+Cs); 
//		    break;
//		            }
                  case 'A': { 
                    config_strings.push_back("Z1_RISE");
                    config_strings.push_back("Z2_RISE"); 
                    config_strings.push_back("W0_RISE"); 
                    config_strings.push_back("W1_RISE"); 
                    config_strings.push_back("W2_RISE"); 
	      	    break;
		            }
                  case 'E': { 
                    config_strings.push_back("dT_CP");
                    config_strings.push_back("dT_DP"); 
                    config_strings.push_back("T_WAIT"); 
                    config_strings.push_back("FP_CHK"); 
                    config_strings.push_back("DT_PMP"); 
                    config_strings.push_back("Z_dPMP"); 
	    	    break;
		            }
                  case 'K': { 
                    config_strings.push_back("PTS_T0");
                    config_strings.push_back("PTS_T1");
                    config_strings.push_back("PTS_T2");
                    config_strings.push_back("PTS_T3");
                    config_strings.push_back("PTS_T4");
		    break;
		        }
//                case 'L': { // V0.5 and before
//                  config_strings.push_back("CCzero");
//                  config_strings.push_back("CC_km");
//                  config_strings.push_back("ZN_CF");
//                  config_strings.push_back("Z_Neu");
//	            break;
//		            }
                  case 'L': { // V0.6 and after
                    config_strings.push_back("CCzero");
                    config_strings.push_back("CC_km");
                    config_strings.push_back("ZN_CF");
                    config_strings.push_back("ccCor");
	    	    break;
		            }
                  case 'M': { 
                    config_strings.push_back("dZ_0");
                    config_strings.push_back("dZ_1");
                    config_strings.push_back("dZ_2");
                    config_strings.push_back("dZ_3");
                    config_strings.push_back("dZ_4");
		    break;
		            }
                  case 'Y': { //for V1.0 and later, use "4" for V0.8
                    config_strings.push_back("Ice_Tc"); 
                    config_strings.push_back("Ice_Ps"); 
                    config_strings.push_back("Ice_Pd"); 
                    config_strings.push_back("Ice_Mn"); 
                    config_strings.push_back("Ice_Sc"); 
                    config_strings.push_back("IceIn"); 
                    config_strings.push_back("IceOut"); 
	    	    break;
			    }
                  case 'Z': { // firware V 0.6 and later
                    config_strings.push_back("Cyc"+Cs); //must append sub-cycle
                    config_strings.push_back("Zpark"+Cs); 
                    config_strings.push_back("Zpro"+Cs); 
                    config_strings.push_back("ccPro"+Cs); 
                    config_strings.push_back("ccPrk"+Cs); 
	        	break;
		            }
                  case '7': {
                    config_strings.push_back("Z_C2D");
                    config_strings.push_back("Z_DP_1");
                    config_strings.push_back("Z_DP_2");
                    config_strings.push_back("Z_DP_3");
                    config_strings.push_back("Z_DP_4");
		    break;
		            }
                  case '#': {
                    config_strings.push_back("Nseek");
                    config_strings.push_back("STLmin");
                    config_strings.push_back("dDdZ");
                    config_strings.push_back("dDdV2");
		    break;
		            }
                  case '@': {
                    config_strings.push_back("MAX_XL");
		    break;
		            }
                  default: 
		    break;
                            } //end of Cp switch
	      } // end of case 1 (DEEP)
	      case 2: { //BGC SOLO
//	        std::cout << "examining BGC SOLO specific commands " << std::endl;
	        switch (Cp) { // The first character is the primary command
                  case 'E': {
                    config_strings.push_back("Ice_Tc"); 
                    config_strings.push_back("Ice_Ps"); 
                    config_strings.push_back("Ice_Pd"); 
                    config_strings.push_back("Ice_Mn"); 
                    config_strings.push_back("Ice_Sc"); 
                    config_strings.push_back("IceIn"); 
                    config_strings.push_back("IceOut"); 
	    	    break;
			    }
                  case 'L': {
                    config_strings.push_back("SrfDft"); 
                    config_strings.push_back("SrfLon"); 
                    config_strings.push_back("SrfLat"); 
                    config_strings.push_back("SrfMxN"); 
                    config_strings.push_back("SrfInt"); 
		    break;
			    }
                  case 'Z': {
                    config_strings.push_back("Cyc"+Cs); //must append sub-cycle
                    config_strings.push_back("Ztar"+Cs); 
                    config_strings.push_back("Zpro"+Cs); 
                    config_strings.push_back("Tlast"+Cs); 
		    break;
		           }
                  case '#': {
                    config_strings.push_back("Nseek");
                    config_strings.push_back("STLmin");
                    config_strings.push_back("dTadZ");
                    config_strings.push_back("dTsdZ");
                    config_strings.push_back("TlastCF");
		    break;
		           }
                  default: 
                    break;
                           } //end of Cp switch
	      default:
                break;
            } //end SOLOtype switch
   	  } //end of dafault from inital casecase 
  	} // end of Solotype block

	std::string good_chars = "1234";
	if ( Cs == "S" && good_chars.find(Cb[1]) != std::string::npos) {

	  int ans = ModSCI(command.substr(0,command.length()));
 	  return;

	} else {

// PROCESS CONFIGS

// Outputs the CONFIG names that are in this command
//       for (size_t i = 0; i < config_strings.size(); i++) {
//         std::cout << config_strings[i] << std::endl;
//       }

// Outputs the command
//	 std::cout << command.substr(2,command.length()) << std::endl;

	
         std::vector<int> config_values;
	 std::stringstream ss(command.substr(2,command.length()));
	 int num;
         while ( ss >> num ) {  //Extracts integers from command separated by whitespace
		config_values.push_back(num);
 	 }

//Calls routine to search for Config String/ Modify to Config Value/Outputs the changed parameters
         for (size_t i= 0; i < std::min(config_values.size(),config_strings.size()); i++) {
	   std::string unit="";
	   std::string desc="";
//	   std::cout << "Identified Upload_Command CONFIG " << config_strings[i] << "=" << config_values[i] << std::endl;
           ModOneConfig(config_strings[i],config_values[i],unit,desc);
         }
 	return;

        }
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Function Declaration
int ModSCI(string command) {

	vector<string> sensor_name  = { "ctd","DO","pH","ECO","OCR","Nitrate" };        // sensor names parsed in this order
        vector<string> region_name  = { "region1","region2","region3","region4","region5" }; // five regions defined for each sensor
	vector<string> S1_name  = { "Enabled","EnDrift","DecDrift","PackType" };        // 
	vector<string> S4_name  = { "gain","offset" };        // 
        vector<string> S3_name  = { "zMin","zMax","dz","scanT" }; // four parameters defined for each sensor

// Outputs the command
//	std::cout << "In ModSCI \"" << command << "\"" << std::endl;

	
        std::vector<int> SCI_values;
	std::stringstream ss(command);
	int num;
        while ( ss >> num ) {  //Extracts integers from command separated by whitespace
		SCI_values.push_back(num);
 	}
 
	if  (RunSCI.size() > 0 ) { // if SCI isn't fully implemented in json then nothing to write to
          int sval = SCI_values[0];
	  switch (sval) { // Pass the command value
	    case 1: { // "4S S1"
		RunSCI[sensor_name[SCI_values[1]]][S1_name[SCI_values[2]]] = SCI_values[3];
//		std::cout << "4S S1 " << sensor_name[SCI_values[1]] << S1_name[SCI_values[2]] << " now = " << RunSCI[sensor_name[SCI_values[1]]][S1_name[SCI_values[2]]] << std::endl;
		break;
		    }
	    case 2: { // "4S S2"
		RunSCI[sensor_name[SCI_values[1]]][region_name[0]][S3_name[SCI_values[2]]] = SCI_values[3];
		RunSCI[sensor_name[SCI_values[1]]][region_name[1]][S3_name[SCI_values[2]]] = SCI_values[4];
		RunSCI[sensor_name[SCI_values[1]]][region_name[2]][S3_name[SCI_values[2]]] = SCI_values[5];
		RunSCI[sensor_name[SCI_values[1]]][region_name[3]][S3_name[SCI_values[2]]] = SCI_values[6];
		RunSCI[sensor_name[SCI_values[1]]][region_name[4]][S3_name[SCI_values[2]]] = SCI_values[7];
//		std::cout << "4S S2 " << sensor_name[SCI_values[1]] << " All region " << S3_name[SCI_values[2]] << " now = " << RunSCI[sensor_name[SCI_values[1]]][region_name[0]][S3_name[SCI_values[2]]] << std::endl;
		break;
		    }
	    case 3: { // "4S S3"
		RunSCI[sensor_name[SCI_values[1]]][region_name[SCI_values[2]]][S3_name[SCI_values[3]]] = SCI_values[4];
//		std::cout << "4S S3 " << sensor_name[SCI_values[1]] << region_name[SCI_values[2]] << S3_name[SCI_values[3]] << " now = " << RunSCI[sensor_name[SCI_values[1]]][region_name[SCI_values[2]]][S3_name[SCI_values[3]]] << std::endl;
		break;
		    }
	    case 4: { // "4S S4"
		RunSCI[sensor_name[SCI_values[1]]][S4_name[SCI_values[2]]] = SCI_values[3];
//		std::cout << "4S S4 " << sensor_name[SCI_values[1]] << S4_name[SCI_values[2]] << " now = " << RunSCI[sensor_name[SCI_values[1]]][S4_name[SCI_values[2]]]<< std::endl;
		break;
		    }
	  }
 	  return 1;  // SCI_Paramter exists and was modified

	} else {
 	  return 0;  // SCI_Paramter does not exists and an Upload Command was not processed
	}
}
/////////////////////////////////////////////////////////////////////////////////////////////
// Function definition
void ModOneConfig(string ConfigName, int ConfigValue, string unit, string desc) {

//	std::cout << "In ModOneConfig " << ConfigName << "  " << ConfigValue << std::endl;	
        int indx = 0; //value passed back by function to communicate index of CONFIG within RunMission
	if ( RunMission.contains(std::string{ConfigName}) ) {
          if ( RunMission[ConfigName]["value"] != ConfigValue ) {
             std::cout << "\"" << ConfigName << "\" MODIFIED from " << RunMission[ConfigName]["value"] << " to NEW VALUE =>  " << ConfigValue <<std::endl; 
	     RunMission[ConfigName]["value"] = ConfigValue;
          } 
	// Always see if need to update "unit" and "description"
          if ( !unit.empty() ) {
  		RunMission[ConfigName]["unit"] = unit;
//                std::cout << "Unit Mission  = " << RunMission[ConfigName]["unit"] <<std::endl; 
	  }
          if ( !desc.empty() ) {
		RunMission[ConfigName]["description"] = desc;
 //               std::cout << "Desc Mission  = " << RunMission[ConfigName]["description"] <<std::endl; 
	  }
	} else {
	  //If made it here then there needs to be a CONFIG added to RunMission
          RunMission[ConfigName]["value"] = ConfigValue;	
	  if ( mission_template.contains(std::string{ConfigName}) ) {
            RunMission[ConfigName]["unit"] = mission_template[ConfigName]["units"];  //Oddly declared differently (+s) 
            RunMission[ConfigName]["description"] = mission_template[ConfigName]["description"];	
	  } else {
            RunMission[ConfigName]["unit"] = unit;
            RunMission[ConfigName]["description"] = desc;	
	  }
	}
        return;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
void rewrite_json(string filename) { //sends in L0 json filename

	json Doc_L0; //create uninitialized json object as local variable

	std::ifstream json_L0(filename, std::ifstream::binary);
        json_L0 >> Doc_L0; // initialize json object with input from file

        //read in some info from L0 json
        std::ostringstream ss;
	if ( Doc_L0.contains("DAC_ID_NUMBER") ) {
  	  int iid=Doc_L0.at("DAC_ID_NUMBER");
          ss <<  L1_path << "/" << std::setw(4) << std::setfill('0') << iid << "_";
//        ss <<  L1_path << "/" << std::setw(5) << std::setfill('0') << iid << "_";
	} else {
          ss <<  L1_path << "/" << std::setw(4) << std::setfill('0') << 9999 << "_";
	}
	if ( Doc_L0.contains("TRANSMISSION_ID_NUMBER") ) {
	  int tid=Doc_L0.at("TRANSMISSION_ID_NUMBER");
          ss << std::setw(6) << std::setfill('0') << tid << "_L1_"; 
	} else {
          ss << std::setw(6) << std::setfill('0') << 999999 << "_L1_"; 
	}
        int cyn = Doc_L0.at("CYCLE_NUMBER");
	if ( cyn > 99 ) {
          ss << cyn << ".json";
	} else if ( cyn > -1 ) {
          ss << std::setw(3) << std::setfill('0') << cyn << ".json";
	} else {
          ss << "-01" << ".json";
	}
	std::string outFile = ss.str();

        std::vector<nlohmann::json> GPSnew;
// choose GPS positions that were from THIS cycles surface interval...save to a new json GPSnew
        for (const auto& obj : GPSobj) {
          int gpscy = obj["cycle"].get<int>();
  	  const auto& matrix = obj.at("data"); {
	    for (const auto& row : matrix) {
	      const auto& gpstype = row.at("description"); {
      	        if ( ( gpscy == ( cyn + 1 ) & gpstype == "GPS_START" ) | ( gpscy == cyn & gpstype != "GPS_START" ) ) {
//     	          std::cout << gpscy << " " << gpstype << std::endl; 
//     	          std::cout << row << std::endl; 
      	          GPSnew.push_back(row); 
                }
              }
            }
          }
        }
//     	std::cout << GPSnew.size() << std::endl; 
//     	std::cout << GPSnew << std::endl; 



// determine if there are any CONFIG that relate to whether data is transmitted or not thus "Missing"
	std::vector<int> SBck(9,1); //presume that all CTD data is transmitted (SndBak=511)
        vector<string> sensor_name  = { "ctd", "DO", "pH", "ECO", "OCR", "Nitrate" };        // sensor names parsed in this order
	std::vector<int> BGCen(6,1); //presume that all BGC data is Enabled
	std::vector<int> BGCed(6,1); //presume that all BGC drift data is Enabled
	std::vector<int> BGCdd(6,1); //presume that all BGC drift data has no decimation
	std::vector<int> BGCsd(6,0); //presume that all BGC drift data starts at beginning of drift (value of 0)
	std::vector<int> SndPr(6,1); //BGC Sensors that send pressure; presume all do by default

	std::vector<int> SndDO(7,0); //Variables sent by DO sensor; presume default
	SndDO = ApplyDataT(sensor_name[1],0); 
	std::vector<int> SndpH(7,0); //Variables sent by pH sensor; presume default (never changes)
	SndpH = ApplyDataT(sensor_name[2],0); 
	std::vector<int> SndECO(7,0); //Variables sent by ECO sensor; presume default (never changes)
	SndECO = ApplyDataT(sensor_name[3],0); 
	std::vector<int> SndOCR(7,0); //Variables sent by OCR sensor; presume default (never changes)
	SndOCR = ApplyDataT(sensor_name[4],0); 
	std::vector<int> SndNitrate(7,0); //Variables sent by Nitrate sensor; presume default
	SndNitrate = ApplyDataT(sensor_name[5],0);

	BGCed[5]=0; //N03 drift is disabled
	bool writeBinCTD = true; //presume that all BinnedCTD data is transmitted
        bool writeRawCTD = true; //presume that all Raw CTD is collected 
        bool writeDriftCTD = true; //presume that all drift CTD is collected 
        bool writeDOair = true; //presume that all in Air collected
        bool isBEACON = false; //presume that not in beacon
        bool isDPLY = false; //presume that not in cycle 0 BIST
        bool isBIST = false; //presume that not in cycle -1 BIST
	int phDec = 1; //presume no decimation of pH
	if ( RunMission.contains("SndBak") ) { // parses the CTD SndBak CONFIG
	  int val = RunMission["SndBak"]["value"].get<int>();
          int i=SBck.size()-1;
	  for ( int& num : SBck ) {
	    int p2 = pow(2,i);
	    if ( val < p2 ) {
	      SBck[i]=0;
	    } else {
	      val=val-p2;
	    }
 	    i--;
	  }
//	  std::cout << SBck[8] << " Send Binned PSAL" << std::endl;
//	  std::cout << SBck[7] << " Send Binned TEMP" << std::endl;
//	  std::cout << SBck[6] << " Send Binned PRES" << std::endl;
//	  std::cout << SBck[5] << " Send CTD Drift" << std::endl;
//	  std::cout << SBck[4] << " Send Pump" << std::endl;
//	  std::cout << SBck[3] << " Send Fall" << std::endl;
//	  std::cout << SBck[2] << " Send Rise" << std::endl;
//	  std::cout << SBck[1] << " Send Raw" << std::endl;
//	  std::cout << SBck[0] << " Send GPS/Eng/Argo" << std::endl;
	  auto it = std::find(SBck.end()-3, SBck.end(), 1);  //check last 3 "Binned" flags to see if any are set to return data
	  if ( it != SBck.end() ) {
	    writeBinCTD = true;
	  } else {
	    writeBinCTD = false; 
	  }
	}
	if ( RunMission.contains("UNBINd") ) {
	  int val = RunMission["UNBINd"]["value"].get<int>();
	  if ( val == 0 ) {
	    writeRawCTD = false; //there is no raw CTD collected
	  }
	}
  	if ( RunMission.contains("DrfDat") ) {
	  int val = RunMission["DrfDat"]["value"].get<int>();
	  if ( val == 0 ) {
	    writeDriftCTD = false; //there is no Drift CTD collected (and no BGC Drift)
	  }
	}
  	if ( RunMission.contains("phDec") ) {
	  int phDec = RunMission["phDec"]["value"].get<int>(); //store the decimation of pH Ik and Ib
	}
// Determine the number of drift measurements (CTD) that are expected in cycle
	int Nsam = -9;
	if ( Doc_L0.contains(std::string{ "Engineering_Data" }) ) {  //in order to apply must get subcycle from engineering
	  if ( Doc_L0[ "Engineering_Data" ].contains("binMod") ) {  
            int binMod = Doc_L0.at("Engineering_Data")["binMod"]["value"];
       	    int subcyc = ( binMod % 8 );  //"binMod" contains subcy and CTD binning (modulus 8)
	    if ( RunMission.contains("Nsam" + std::to_string(subcyc)) ) {
	      Nsam = RunMission["Nsam" + std::to_string(subcyc)]["value"].get<int>();
//	      std::cout << "Nsam " << Nsam << std::endl;
	    }
	  }
	}
  	if ( RunMission.contains("sDOairN") ) {
	  int val = RunMission["sDOairN"]["value"].get<int>();
	  if ( val == 0 ) {
	    writeDOair = false; //there is no in air collected
	  }
	}
  	if ( RunMission.contains("sSendPr") ) { // determine which BGC did not send PRES
	  int val = RunMission["sSendPr"]["value"].get<int>();
	  val = val - 192; // sSendPr is a factor of 2 ^ 7...but only use 5 (with 6 sensors)
  	  int i =SndPr.size()-1;
	  for ( int& num : SndPr ) {
	    int p2 = pow(2,i);
	    if ( val < p2 ) {
	      SndPr[i]=0;
	    } else {
	      val=val-p2;
	    }
//	    std::cout << i << " " <<  SndPr[i] << " " << val << std::endl;
 	    i--;
	  }
	}
	if ( RunSCI.contains("DO") ) { // Now add the dataT for the DO BGC paramters
	  int dataT = RunSCI["DO"]["DataType"].get<int>();
	  SndDO = ApplyDataT("DO",dataT);
	  SndDO[0] = SndPr[1]; //transfer pressure to SndDO
	}
	if ( RunSCI.contains("pH") ) { // Now add the dataT for the DO BGC paramters
	  int dataT = RunSCI["pH"]["DataType"].get<int>();
	  SndpH = ApplyDataT("pH",dataT);
	  SndpH[0] = SndPr[2]; //transfer pressure to SndpH
	}
	if ( RunSCI.contains("ECO") ) { // Now add the dataT for the DO BGC paramters
	  int dataT = RunSCI["ECO"]["DataType"].get<int>();
	  SndECO = ApplyDataT("ECO",dataT);
	  SndECO[0] = SndPr[3]; //transfer pressure to SndECO
	}
	if ( RunSCI.contains("OCR") ) { // Now add the dataT for the DO BGC paramters
	  int dataT = RunSCI["OCR"]["DataType"].get<int>();
	  SndOCR = ApplyDataT("OCR",dataT);
	  SndOCR[0] = SndPr[4]; //transfer pressure to SndOCR
	}
	if ( RunSCI.contains("Nitrate") ) { // Now add the dataT for the Nitrate BGC paramters
	  int dataT = RunSCI["Nitrate"]["DataType"].get<int>();
	  SndNitrate = ApplyDataT("Nitrate", dataT );
	  SndNitrate[0] = SndPr[5]; //transfer pressure to SndNitrate
	}
// RunSCI always present...but need size loop // Load in 
	int SCIsize = RunSCI.size();
	BGCen[0]=0; //dummy fill the CTD part of the variable (not used)
	BGCed[0]=0; //dummy fill the CTD part of the variable (not used)
	BGCdd[0]=1; //dummy fill the CTD part of the variable (not used)
	BGCsd[0]=100; //dummy fill the CTD part of the variable (not used)
        for (size_t i = 1; i < SCIsize; ++i) {
	  BGCen[i] = RunSCI[sensor_name[i]]["Enabled"].get<int>();
	  BGCsd[i] = 100 - RunSCI[sensor_name[i]]["EnDrift"].get<int>();
	  BGCdd[i] = RunSCI[sensor_name[i]]["DecDrift"].get<int>();
          if ( BGCsd[i] < 100 ) { // modified EnDrift to declare BGCsd. Now BGCsd is 100 if drift is off
	    BGCed[i] = 1; 
	  } else {
	    BGCed[i] = 0; 
	  }
//	  std::cout << sensor_name[i] << BGCen[i] << BGCed[i] << std::endl;
        }
	if ( BGCen[1] == 0 ) {
	  writeDOair = false; //there is no in air collected since entire sensor off
	}

// Check for special cycles through "packet_info" that return non-CONFIG data
	if ( Doc_L0.contains(std::string{ "packet_info" }) ) {
	  const auto& matrix = Doc_L0.at("packet_info")["packet_type"];
	  for (const auto& row : matrix) {
	     string id_string = row["id"].get<std::string>();
	     if ( id_string == "000000" | id_string == "040A08" ) {
	       isDPLY = true;
	       std::cout << "Identified Deployment cycle " << cyn << " packet_info id_string => " << id_string << std::endl;
	     }
	     if ( id_string == "000500" | id_string == "000600" | id_string == "040D08" ) {
	       isBIST = true;
	       std::cout << "Identified BIST cycle " << cyn << " packet_info id_string => " << id_string << std::endl;
	     }
	     if ( id_string == "000300" | id_string == "040C08" ) {
	       isBEACON = true;
	       std::cout << "Identified BEACON cycle " << cyn << " packet_info id_string => " << id_string << std::endl;
	     }
	  }
	}
// FINISHED gathering of CONFIG
//
// write the updated json file
    	std::cout << "Writing " << outFile << std::endl; 
        std::ofstream fout(outFile);

        fout << "{" << std::endl;
	copyLine ( fout, "FILE_CREATION_DATE", "", Doc_L0 );  // as last field dont send in ","
        fout << "," << std::endl << "  \"FILE_UPDATE_DATE\": \"" << date_format(utcnow(),"yyyy-mm-ddTHH:MM:SSZ") << "\"";
	copyLine ( fout, "DECODER_VERSION", ",", Doc_L0 );
	copyLine ( fout, "SCHEMA_VERSION", ",", Doc_L0 );
	copyLine ( fout, "INTERNAL_ID_NUMBER", ",", Doc_L0 );
	copyLine ( fout, "DAC_ID_NUMBER", ",", Doc_L0 );
	copyLine ( fout, "WMO_ID_NUMBER", ",", Doc_L0 );
	copyLine ( fout, "TRANSMISSION_ID_NUMBER", ",", Doc_L0 );
	copyLine ( fout, "INSTRUMENT_TYPE", ",", Doc_L0 );
	//copyLine ( fout, "WMO_INSTRUMENT_TYPE (TABLE 1770)", ",", Doc_L0 );
	//copyLine ( fout, "WMO_RECORDER_TYPE (TABLE 4770)", ",", Doc_L0 );
	//copyLine ( fout, "POSITIONING_SYSTEM", ",", Doc_L0 );
	copyLine ( fout, "PI", ",", Doc_L0 );
	copyLine ( fout, "OPERATING_INSTITUTION", ",", Doc_L0 );
	copyLine ( fout, "PROJECT_NAME", ",", Doc_L0 );
	copyLine ( fout, "CYCLE_NUMBER", ",", Doc_L0 ); 

	if ( Doc_L0.contains(std::string{ "telemetry_summary" }) ) {
  	  fout << "," << std::endl; //finish the previous block
  	  fout << "  \"telemetry_summary\": [" << std::endl;
	  const auto& matrix = Doc_L0.at("telemetry_summary");
          size_t j = 0;
	  for (const auto& row : matrix) {
            j++; 
            fout << "    { \"PID\": " << std::setw(2) << row["PID"].get<int>() << ", ";
            fout << "\"source\": " << row["source"] << ", ";
            fout << "\"TIME\": " << row["TIME"] << ", ";
            fout << "\"momsn\": " << row["momsn"].get<int>() << ", ";
            fout << "\"size\": " << std::setw(4) << row["size"].get<int>() << ", ";
            fout << "\"sensor_ids\": "; 
            size_t i = 0;
      	    for (const auto& element : row) {
              i++; 
     	      if (i == row.size() & j == matrix.size() ) {
	  	fout << element << " }" << std::endl;
  	      } else if ( i == row.size() ) {
		fout << element << " }," << std::endl;
	      }
  	    }
	  }
	  fout << "  ]";
	}

	if ( Doc_L0.contains(std::string{ "packet_info" }) ) {
  	  fout << "," << std::endl; //finish the previous block
  	  fout << "  \"packet_info\": {" << std::endl;
          fout << "    \"packet_count\": " << Doc_L0.at("packet_info")["packet_count"] << "," << std::endl;
          fout << "    \"packet_bytes\": " << Doc_L0.at("packet_info")["packet_bytes"] << "," << std::endl;
          fout << "    \"packet_type\": [" << std::endl;
	  const auto& matrix = Doc_L0.at("packet_info")["packet_type"];
          size_t i = 0;
	  for (const auto& row : matrix) {
             i++; 
             fout << "      { \"id\": " << row["id"] << ", ";
             fout << "\"bytes\": " << std::setw(4) << row["bytes"].get<int>() << ", ";
             fout << "\"packets\": " << std::setw(2) << row["packets"].get<int>() << ", ";
             fout << "\"description\": " << row["description"];
     	     if ( i == matrix.size() ) {
               fout <<  " }" << std::endl;
	     } else {
               fout <<  " }," << std::endl;
	     }
	  } 
	  fout << "    ]" << std::endl;
	  fout << "  }";
	} 

// Modified to shift GPS from what is in Doc_L0
//	if ( Doc_L0.contains(std::string{ "GPS" }) ) {
	if ( GPSnew.size() > 0 ) {
  	  fout << "," << std::endl; //finish the previous block
  	  fout << "  \"GPS\": [" << std::endl;
//	  const auto& matrix = Doc_L0.at("GPS");
	  const auto& matrix = GPSnew;
          size_t i = 0;
	  for (const auto& row : matrix) {
            i++; 
	    std::string s=row["description"].get<std::string>();
            fout << "    { \"description\": " << std::setw(9-s.length()) << "" << row["description"] << ",";
            fout << " \"TIME\": " << row["TIME"] << ", ";
            fout << "\"LATITUDE\": " << decimal(row["LATITUDE"],9,5) << ",";
            fout << " \"LONGITUDE\": " << decimal(row["LONGITUDE"],10,5) << ",";
            fout << " \"HDOP\": " << decimal(row["HDOP"],5,1) << ", ";
            fout << "\"sat_cnt\": " << std::setw(2) << row["sat_cnt"].get<int>() << ",";
            fout << " \"snr_min\": " << std::setw(2) << row["snr_min"].get<int>() << ",";
            fout << " \"snr_mean\": " << std::setw(2) << row["snr_mean"].get<int>() << ",";
            fout << " \"snr_max\": " << std::setw(2) <<  row["snr_max"].get<int>() << ",";
            fout << " \"time_to_fix\": " << std::setw(3) << row["time_to_fix"].get<int>() << ",";
            fout << " \"valid\": " << std::setw(2) << row["valid"].get<int>();
     	    if ( i == matrix.size() ) {
              fout <<  " }" << std::endl;
	    } else {
              fout <<  " }," << std::endl;
	    }
	  } 
	  fout << "  ]";
	} 

	if ( Doc_L0.contains(std::string{ "Upload_Command" }) ) {
          fout << "," << std::endl; //finish the previous block
          fout << "  \"Upload_Command\": " << Doc_L0.at("Upload_Command");
        }

	if ( Doc_L0.contains(std::string{ "ARGO_Mission" }) ) {
	  int float_check;
  	  fout << "," << std::endl; //finish the previous block
          resp=determine_resolution(Doc_L0.at("ARGO_Mission")["ctd_pres"]["gain"]);
          rest=determine_resolution(Doc_L0.at("ARGO_Mission")["ctd_temp"]["gain"]);
          ress=determine_resolution(Doc_L0.at("ARGO_Mission")["ctd_psal"]["gain"]);
  	  fout << "  \"ARGO_Mission\": {" << std::endl;
          fout << "    \"Float_Version\": " << Doc_L0.at("ARGO_Mission")["Float_Version"] << "," << std::endl;
          fout << "    \"firmware_version\": " << decimal(Doc_L0.at("ARGO_Mission")["firmware_version"],3,1) << "," << std::endl;
          fout << "    \"min_ascent_rate\": " << Doc_L0.at("ARGO_Mission")["min_ascent_rate"] << "," << std::endl;
          fout << "    \"profile_target\": " << Doc_L0.at("ARGO_Mission")["profile_target"] << "," << std::endl;
          fout << "    \"drift_target\": " << Doc_L0.at("ARGO_Mission")["drift_target"] << "," << std::endl;
              float_check = convert_to_hours(Doc_L0.at("ARGO_Mission")["max_rise_time"]); //convert modified ARGO_Mission back to minutes
          fout << "    \"max_rise_time\": " << decimal(float_check,3,0) << "," << std::endl;
              float_check = convert_to_hours(Doc_L0.at("ARGO_Mission")["max_fall_to_park"]); //convert modified ARGO_Mission back to minutes
          fout << "    \"max_fall_to_park\": " << decimal(float_check,3,0) << "," << std::endl;
              float_check = convert_to_hours(Doc_L0.at("ARGO_Mission")["max_fall_to_profile"]); //convert modified ARGO_Mission back to minutes
          fout << "    \"max_fall_to_profile\": " << decimal(float_check,3,0) << "," << std::endl;
              float_check = convert_to_hours(Doc_L0.at("ARGO_Mission")["target_drift_time"]); //convert modified ARGO_Mission back to minutes
          fout << "    \"target_drift_time\": " << decimal(float_check,4,0) << "," << std::endl;
          fout << "    \"target_surface_time\": " << decimal(Doc_L0.at("ARGO_Mission")["target_surface_time"],6,4) << "," << std::endl;
          fout << "    \"seek_periods\": " << decimal(Doc_L0.at("ARGO_Mission")["seek_periods"],2,0) << "," << std::endl;
              float_check = convert_to_hours(Doc_L0.at("ARGO_Mission")["seek_time"]); //convert modified ARGO_Mission back to minutes
          fout << "    \"seek_time\": " << decimal(float_check,3,0) << "," << std::endl;
          fout << "    \"ctd_pres\": { \"gain\":" << std::setw(5) << Doc_L0.at("ARGO_Mission")["ctd_pres"]["gain"].get<int>() << ", \"offset\":" << std::setw(4) << Doc_L0.at("ARGO_Mission")["ctd_pres"]["offset"].get<int>() << "}," << std::endl;
          fout << "    \"ctd_temp\": { \"gain\":" << std::setw(5) << Doc_L0.at("ARGO_Mission")["ctd_temp"]["gain"].get<int>() << ", \"offset\":" << std::setw(4) << Doc_L0.at("ARGO_Mission")["ctd_temp"]["offset"].get<int>() << "}," << std::endl;
          fout << "    \"ctd_psal\": { \"gain\":" << std::setw(5) << Doc_L0.at("ARGO_Mission")["ctd_psal"]["gain"].get<int>() << ", \"offset\":" << std::setw(4) << Doc_L0.at("ARGO_Mission")["ctd_psal"]["offset"].get<int>() << "}" << std::endl;
	  fout << "  }";
	}

        if ( Doc_L0.contains(std::string{ "BIT" }) ) {
          fout << "," << std::endl; //finish the previous block
          const auto& matrix = Doc_L0.at("BIT");
          fout << "  \"BIT\": {" << std::endl;
//          std::string status = Doc_L0["BIT"]["status"];
          float pump_voltage;
	  if (Doc_L0["BIT"]["status"] == "Beacon") {
             fout << "    \"status\": " << Doc_L0["BIT"]["status"] << "," << std::endl;
             fout << "    \"nQueued\": " << Doc_L0["BIT"]["nQueued"] << "," << std::endl;
             fout << "    \"nTries\": " << Doc_L0["BIT"]["nTries"] << "," << std::endl;
             fout << "    \"parXstat\": " << Doc_L0["BIT"]["parXstat"] << "," << std::endl;
             fout << "    \"SBDIstat\": " << Doc_L0["BIT"]["SBDIstat"] << "," << std::endl;
             fout << "    \"cpu_voltage\": " << decimal(Doc_L0["BIT"]["cpu_voltage"],5,2) << "," << std::endl;
             fout << "    \"pump_voltage\": " << decimal(Doc_L0["BIT"]["pump_voltage"],5,2) << "," << std::endl;
             fout << "    \"vacuum_transmit_inHg\": " << decimal(Doc_L0["BIT"]["vacuum_transmit_inHg"],5,2) << "," << std::endl;
             fout << "    \"vacuum_abort_inHg\": " << decimal(Doc_L0["BIT"]["vacuum_abort_inHg"],5,2) << "," << std::endl;
             fout << "    \"last_interrupt\": " << Doc_L0["BIT"]["last_interrupt"] << "," << std::endl;
             fout << "    \"abortFlag\": " << Doc_L0["BIT"]["abortFlag"] << "," << std::endl;
             fout << "    \"CPUtemp\": " << Doc_L0["BIT"]["CPUtemp"] << "," << std::endl;
             fout << "    \"RH\": " << Doc_L0["BIT"]["RH"] << std::endl;
          } else {
             fout << "    \"status\": " << Doc_L0["BIT"]["status"] << "," << std::endl;
             fout << "    \"blocks_queued\": " << Doc_L0["BIT"]["blocks_queued"] << "," << std::endl;
             fout << "    \"pressure\": " << decimal(Doc_L0["BIT"]["pressure"],5,resp) << "," << std::endl;
             fout << "    \"cpu_voltage\": " << decimal(Doc_L0["BIT"]["cpu_voltage"],5,2) << "," << std::endl;
             fout << "    \"pump_voltage_prior\": " << decimal(Doc_L0["BIT"]["pump_voltage_prior"],5,2) << "," << std::endl;
             fout << "    \"pump_voltage_after\": " << decimal(Doc_L0["BIT"]["pump_voltage_after"],5,2) << "," << std::endl;
             fout << "    \"pump_voltage_mA\": " << Doc_L0["BIT"]["pump_voltage_mA"] << "," << std::endl;
             fout << "    \"pump_time_s\": " << Doc_L0["BIT"]["pump_time_s"] << "," << std::endl;
             fout << "    \"pump_oil_prior\": " << Doc_L0["BIT"]["pump_oil_prior"] << "," << std::endl;
             fout << "    \"pump_oil_after\": " << Doc_L0["BIT"]["pump_oil_after"] << "," << std::endl;
             fout << "    \"vacuum_prior_inHg\": " << decimal(Doc_L0["BIT"]["vacuum_prior_inHg"],5,2) << "," << std::endl;
             fout << "    \"vacuum_after_inHg\": " << decimal(Doc_L0["BIT"]["vacuum_after_inHg"],5,2) << "," << std::endl;
             fout << "    \"valve_open\": " << Doc_L0["BIT"]["valve_open"] << "," << std::endl;
             fout << "    \"valve_close\": " << Doc_L0["BIT"]["valve_close"] << "," << std::endl;
             fout << "    \"interrupt_id\": " << Doc_L0["BIT"]["interrupt_id"] << "," << std::endl;
             fout << "    \"SBE_response\": " << Doc_L0["BIT"]["SBE_response"] << "," << std::endl;
             fout << "    \"cpu_temp_degC\": " << Doc_L0["BIT"]["cpu_temp_degC"] << "," << std::endl;
             fout << "    \"RH\": " << Doc_L0["BIT"]["RH"] << std::endl;
          }
          fout << "  }";
        }

	if ( Doc_L0.contains(std::string{ "Fall" }) || ( SBck[3] == 1 && !isBEACON && !isBIST && !isDPLY ) ) {
  	  fout << "," << std::endl; //finish the previous block
  	  fout << "  \"Fall\": [" << std::endl;
	  if ( Doc_L0.contains(std::string{ "Fall" }) ) {
	  const auto& matrix = Doc_L0.at("Fall");
          size_t i = 0;
	  for (const auto& row : matrix) {
             i++; 
             fout << "    { \"TIME\": " << row["TIME"] << ", ";
             fout << "\"PRES\": " << decimal(row["PRES"],7,resp) << ", ";
             fout << "\"phase\": " << decimal(row["phase"],2,0) << ", ";
             fout << "\"description\": " << row["description"];
     	     if ( i == matrix.size() ) {
               fout <<  " }" << std::endl;
	     } else {
               fout <<  " }," << std::endl;
	     }
          } 
          } 
	  fout << "  ]";
        }

	if ( Doc_L0.contains(std::string{ "Rise" }) || ( SBck[2] == 1 && !isBEACON && !isBIST ) ) {
 	  fout << "," << std::endl; //finish the previous block
  	  fout << "  \"Rise\": [" << std::endl;
	  if ( Doc_L0.contains(std::string{ "Rise" }) ) {
	  const auto& matrix = Doc_L0.at("Rise");
          size_t i = 0;
	  for (const auto& row : matrix) {
             i++; 
             fout << "    { \"TIME\": " << row["TIME"] << ", ";
             fout << "\"PRES\": " << decimal(row["PRES"],7,resp) << ", ";
             fout << "\"phase\": " << decimal(row["phase"],2,0) << ", ";
             fout << "\"description\": " << row["description"];
     	     if ( i == matrix.size() ) {
               fout <<  " }" << std::endl;
	     } else {
               fout <<  " }," << std::endl;
	     }
	  } 
	  } 
	  fout << "  ]";
        } 

	if ( Doc_L0.contains(std::string{ "bist_ctd" }) ) {
  	  fout << "," << std::endl; //finish the previous block
	  const auto& matrix = Doc_L0.at("bist_ctd");
  	  fout << "  \"bist_ctd\": {" << std::endl;
          fout << "    \"status\": " << Doc_L0.at("bist_ctd")["status"] << "," << std::endl;
          if ( Doc_L0["bist_ctd"].contains("errors") ) {
	    fout << "    \"errors\": [" << std::endl;
	    const auto& matrix = Doc_L0["bist_ctd"].at("errors");
            size_t i = 0;
	    for (const auto& row : matrix) {
              i++; 
	      fout << "      { \"";
              for (auto const& [keyr,valr] : row.items()) {
		   fout  << keyr << "\": "; 
		   fout << valr;
              }
     	      if ( i == matrix.size() ) {
                fout <<  " }" << std::endl;
	      } else {
                fout <<  " }," << std::endl;
	      }
	    }
            fout <<  "    ]," << std::endl;
	  }
          fout << "    \"numErr\": " << Doc_L0.at("bist_ctd")["numErr"] << "," << std::endl;
          fout << "    \"voltage_V\": " << decimal(Doc_L0.at("bist_ctd")["voltage_V"],5,2) << "," << std::endl;
          fout << "    \"pressure_dbar\": " << decimal(Doc_L0.at("bist_ctd")["pressure_dbar"],7,resp) << "," << std::endl;
          fout << "    \"current_idle_mA\": " << Doc_L0.at("bist_ctd")["current_idle_mA"] << "," << std::endl;
          fout << "    \"current_ctd_high_mA\": " << Doc_L0.at("bist_ctd")["current_ctd_high_mA"] << "," << std::endl;
          fout << "    \"current_ctd_low_mA\": " << Doc_L0.at("bist_ctd")["current_ctd_low_mA"] << "," << std::endl;
          fout << "    \"current_ctd_sleep_mA\": " << Doc_L0.at("bist_ctd")["current_ctd_sleep_mA"] << std::endl;
  	  fout << "  }";
	}

	if ( Doc_L0.contains(std::string{ "bist_DO" }) ) {
  	  fout << "," << std::endl; //finish the previous block
	  const auto& matrix = Doc_L0.at("bist_DO");
  	  fout << "  \"bist_DO\": {" << std::endl;
          fout << "    \"status\": " << Doc_L0.at("bist_DO")["status"] << "," << std::endl;
          if ( Doc_L0["bist_DO"].contains("errors") ) {
	    fout << "    \"errors\": [" << std::endl;
	    const auto& matrix = Doc_L0["bist_DO"].at("errors");
            size_t i = 0;
	    for (const auto& row : matrix) {
              i++; 
	      fout << "      { \"";
              for (auto const& [keyr,valr] : row.items()) {
		   fout  << keyr << "\": "; 
		   fout << valr;
              }
     	      if ( i == matrix.size() ) {
                fout <<  " }" << std::endl;
	      } else {
                fout <<  " }," << std::endl;
	      }
	    }
            fout <<  "    ]," << std::endl;
	  }
          fout << "    \"numErr\": " << Doc_L0.at("bist_DO")["numErr"] << "," << std::endl;
          fout << "    \"voltage_V\": " << decimal(Doc_L0.at("bist_DO")["voltage_V"],5,2) << "," << std::endl;
          fout << "    \"max_current_mA\": " << Doc_L0.at("bist_DO")["max_current_mA"] << "," << std::endl;
          fout << "    \"avg_current_mA\": " << Doc_L0.at("bist_DO")["avg_current_mA"] << "," << std::endl;
          fout << "    \"phase_delay_us\": " << decimal(Doc_L0.at("bist_DO")["phase_delay_us"],6,3) << "," << std::endl;
          fout << "    \"thermistor_voltage_V\": " << decimal(Doc_L0.at("bist_DO")["thermistor_voltage_V"],5,3) << "," << std::endl;
          fout << "    \"DO\": " << decimal(Doc_L0.at("bist_DO")["DO"],5,3) << "," << std::endl;
          fout << "    \"thermistor_degC\": " << decimal(Doc_L0.at("bist_DO")["thermistor_degC"],5,3) << std::endl;
          fout << "  }";
        }

	if ( Doc_L0.contains(std::string{ "bist_pH" }) ) {
  	  fout << "," << std::endl; //finish the previous block
	  const auto& matrix = Doc_L0.at("bist_pH");
  	  fout << "  \"bist_pH\": {" << std::endl;
          fout << "    \"status\": " << Doc_L0.at("bist_pH")["status"] << "," << std::endl;
          if ( Doc_L0["bist_pH"].contains("errors") ) {
	    fout << "    \"errors\": [" << std::endl;
	    const auto& matrix = Doc_L0["bist_pH"].at("errors");
            size_t i = 0;
	    for (const auto& row : matrix) {
              i++; 
	      fout << "      { \"";
              for (auto const& [keyr,valr] : row.items()) {
		   fout  << keyr << "\": "; 
		   fout << valr;
              }
     	      if ( i == matrix.size() ) {
                fout <<  " }" << std::endl;
	      } else {
                fout <<  " }," << std::endl;
	      }
	    }
            fout <<  "    ]," << std::endl;
	  }
          fout << "    \"numErr\": " << Doc_L0.at("bist_pH")["numErr"] << "," << std::endl;
          fout << "    \"voltage_V\": " << decimal(Doc_L0.at("bist_pH")["voltage_V"],5,2) << "," << std::endl;
          fout << "    \"max_current_mA\": " << Doc_L0.at("bist_pH")["max_current_mA"] << "," << std::endl;
          fout << "    \"avg_current_mA\": " << Doc_L0.at("bist_pH")["avg_current_mA"] << "," << std::endl;
          fout << "    \"vRef\": " << decimal(Doc_L0.at("bist_pH")["vRef"],6,3) << "," << std::endl;
          fout << "    \"Vk\": " << decimal(Doc_L0.at("bist_pH")["Vk"],6,3) << "," << std::endl;
          fout << "    \"Ik\": " << decimal(Doc_L0.at("bist_pH")["Ik"],7,3) << "," << std::endl;
          fout << "    \"Ib\": " << decimal(Doc_L0.at("bist_pH")["Ib"],7,3) << std::endl;
          fout << "  }";
        }

	if ( Doc_L0.contains(std::string{ "bist_ECO" }) ) {
  	  fout << "," << std::endl; //finish the previous block
	  const auto& matrix = Doc_L0.at("bist_ECO");
  	  fout << "  \"bist_ECO\": {" << std::endl;
          fout << "    \"status\": " << Doc_L0.at("bist_ECO")["status"] << "," << std::endl;
          if ( Doc_L0["bist_ECO"].contains("errors") ) {
	    fout << "    \"errors\": [" << std::endl;
	    const auto& matrix = Doc_L0["bist_ECO"].at("errors");
            size_t i = 0;
	    for (const auto& row : matrix) {
              i++; 
	      fout << "      { \"";
              for (auto const& [keyr,valr] : row.items()) {
		   fout  << keyr << "\": "; 
		   fout << valr;
              }
     	      if ( i == matrix.size() ) {
                fout <<  " }" << std::endl;
	      } else {
                fout <<  " }," << std::endl;
	      }
	    }
            fout <<  "    ]," << std::endl;
	  }
          fout << "    \"numErr\": " << Doc_L0.at("bist_ECO")["numErr"] << "," << std::endl;
          fout << "    \"voltage_V\": " << decimal(Doc_L0.at("bist_ECO")["voltage_V"],5,2) << "," << std::endl;
          fout << "    \"max_current_mA\": " << Doc_L0.at("bist_ECO")["max_current_mA"] << "," << std::endl;
          fout << "    \"avg_current_mA\": " << Doc_L0.at("bist_ECO")["avg_current_mA"] << "," << std::endl;
          fout << "    \"Chl\": " << decimal(Doc_L0.at("bist_ECO")["Chl"],2,0) << "," << std::endl;
          fout << "    \"bb\": " << decimal(Doc_L0.at("bist_ECO")["bb"],2,0) << "," << std::endl;
          fout << "    \"CDOM\": " << decimal(Doc_L0.at("bist_ECO")["CDOM"],2,0) << std::endl;
  	  fout << "  }";
        }

	if ( Doc_L0.contains(std::string{ "bist_OCR" }) ) {
 	  fout << "," << std::endl; //finish the previous block
	  const auto& matrix = Doc_L0.at("bist_OCR");
  	  fout << "  \"bist_OCR\": {" << std::endl;
          fout << "    \"status\": " << Doc_L0.at("bist_OCR")["status"] << "," << std::endl;
          if ( Doc_L0["bist_OCR"].contains("errors") ) {
	    fout << "    \"errors\": [" << std::endl;
	    const auto& matrix = Doc_L0["bist_OCR"].at("errors");
            size_t i = 0;
	    for (const auto& row : matrix) {
              i++; 
	      fout << "      { \"";
              for (auto const& [keyr,valr] : row.items()) {
		   fout  << keyr << "\": "; 
		   fout << valr;
              }
     	      if ( i == matrix.size() ) {
                fout <<  " }" << std::endl;
	      } else {
                fout <<  " }," << std::endl;
	      }
	    }
            fout <<  "    ]," << std::endl;
	  }
          fout << "    \"numErr\": " << Doc_L0.at("bist_OCR")["numErr"] << "," << std::endl;
          fout << "    \"voltage_V\": " << decimal(Doc_L0.at("bist_OCR")["voltage_V"],5,2) << "," << std::endl;
          fout << "    \"max_current_mA\": " << Doc_L0.at("bist_OCR")["max_current_mA"] << "," << std::endl;
          fout << "    \"avg_current_mA\": " << Doc_L0.at("bist_OCR")["avg_current_mA"] << "," << std::endl;
          fout << "    \"ch01\": " << decimal(Doc_L0.at("bist_OCR")["ch01"],1,0) << "," << std::endl;
          fout << "    \"ch02\": " << decimal(Doc_L0.at("bist_OCR")["ch02"],1,0) << "," << std::endl;
          fout << "    \"ch03\": " << decimal(Doc_L0.at("bist_OCR")["ch03"],1,0) << "," << std::endl;
          fout << "    \"ch04\": " << decimal(Doc_L0.at("bist_OCR")["ch04"],1,0) << std::endl;
          fout << "  }";
        }

	if ( Doc_L0.contains(std::string{ "bist_Nitrate" }) ) {
  	  fout << "," << std::endl; //finish the previous block
	  const auto& matrix = Doc_L0.at("bist_Nitrate");
  	  fout << "  \"bist_Nitrate\": {" << std::endl;
          fout << "    \"status\": " << Doc_L0.at("bist_Nitrate")["status"] << "," << std::endl;
          if ( Doc_L0["bist_Nitrate"].contains("errors") ) {
	    fout << "    \"errors\": [" << std::endl;
	    const auto& matrix = Doc_L0["bist_Nitrate"].at("errors");
            size_t i = 0;
	    for (const auto& row : matrix) {
              i++; 
	      fout << "      { \"";
              for (auto const& [keyr,valr] : row.items()) {
		   fout  << keyr << "\": "; 
		   fout << valr;
              }
     	      if ( i == matrix.size() ) {
                fout <<  " }" << std::endl;
	      } else {
                fout <<  " }," << std::endl;
	      }
	    }
            fout <<  "    ]," << std::endl;
	  }
          fout << "    \"numErr\": " << Doc_L0.at("bist_Nitrate")["numErr"] << "," << std::endl;
          fout << "    \"J_err\": " << decimal(Doc_L0.at("bist_Nitrate")["J_err"],3,0) << "," << std::endl;
          fout << "    \"J_rh\": " << decimal(Doc_L0.at("bist_Nitrate")["J_rh"],6,2) << "," << std::endl;
          fout << "    \"J_volt\": " << decimal(Doc_L0.at("bist_Nitrate")["J_volt"],5,2) << "," << std::endl;
          fout << "    \"J_amps\": " << decimal(Doc_L0.at("bist_Nitrate")["J_amps"],5,3) << "," << std::endl;
          fout << "    \"J_darkM\": " << decimal(Doc_L0.at("bist_Nitrate")["J_darkM"],6,1) << "," << std::endl;
          fout << "    \"J_darkS\": " << decimal(Doc_L0.at("bist_Nitrate")["J_darkS"],6,1) << "," << std::endl;
          fout << "    \"J_Nitrate\": " << decimal(Doc_L0.at("bist_Nitrate")["J_Nitrate"],6,3) << "," << std::endl;
          fout << "    \"J_res\": " << decimal(Doc_L0.at("bist_Nitrate")["J_res"],5,3) << "," << std::endl;
          fout << "    \"J_fit1\": " << decimal(Doc_L0.at("bist_Nitrate")["J_fit1"],5,1) << "," << std::endl;
          fout << "    \"J_fit2\": " << decimal(Doc_L0.at("bist_Nitrate")["J_fit2"],5,1) << "," << std::endl;
          fout << "    \"J_Spectra\": " << "null" << "," << std::endl;
          fout << "    \"J_SeaDark\": " << decimal(Doc_L0.at("bist_Nitrate")["J_SeaDark"],7,2) << "," << std::endl;
 	  fout << "    \"sval[0]\": " << Doc_L0.at("bist_Nitrate")["sval[0]"] << "," << std::endl;
 	  fout << "    \"sval[1]\": " << Doc_L0.at("bist_Nitrate")["sval[1]"] << "," << std::endl;
 	  fout << "    \"sval[2]\": " << Doc_L0.at("bist_Nitrate")["sval[2]"] << "," << std::endl;
 	  fout << "    \"sval[3]\": " << Doc_L0.at("bist_Nitrate")["sval[3]"] << "," << std::endl;
 	  fout << "    \"sval[4]\": " << Doc_L0.at("bist_Nitrate")["sval[4]"] << "," << std::endl;
 	  fout << "    \"sval[5]\": " << Doc_L0.at("bist_Nitrate")["sval[5]"] << "," << std::endl;
 	  fout << "    \"sval[6]\": " << Doc_L0.at("bist_Nitrate")["sval[6]"] << "," << std::endl;
 	  fout << "    \"sval[7]\": " << Doc_L0.at("bist_Nitrate")["sval[7]"] << "," << std::endl;
          fout << "    \"SUNA_str\": " << Doc_L0.at("bist_Nitrate")["SUNA_str"] << "," << std::endl;
          fout << "    \"ascii_timestamp\": " << Doc_L0.at("bist_Nitrate")["ascii_timestamp"] << "," << std::endl;
          fout << "    \"ascii_pressure\": " << decimal(Doc_L0.at("bist_Nitrate")["ascii_pressure"],7,resp) << "," << std::endl;
          fout << "    \"ascii_temperature\": " << decimal(Doc_L0.at("bist_Nitrate")["ascii_temperature"],6,rest) << "," << std::endl;
          fout << "    \"ascii_salinity\": " << decimal(Doc_L0.at("bist_Nitrate")["ascii_salinity"],6,ress) << "," << std::endl;
          fout << "    \"ascii_sample_count\": " << Doc_L0.at("bist_Nitrate")["ascii_sample_count"] << "," << std::endl;
          fout << "    \"ascii_cycle_count\": " << Doc_L0.at("bist_Nitrate")["ascii_cycle_count"] << "," << std::endl;
          fout << "    \"ascii_error_count\": " << Doc_L0.at("bist_Nitrate")["ascii_error_count"] << "," << std::endl;
          fout << "    \"ascii_internal_temp\": " << decimal(Doc_L0.at("bist_Nitrate")["ascii_internal_temp"],5,2) << "," << std::endl;
          fout << "    \"ascii_spectrometer\": " << decimal(Doc_L0.at("bist_Nitrate")["ascii_spectrometer"],6,2) << "," << std::endl;
          fout << "    \"ascii_RH\": " << decimal(Doc_L0.at("bist_Nitrate")["ascii_RH"],5,2) << "," << std::endl;
          fout << "    \"ascii_voltage\": " << decimal(Doc_L0.at("bist_Nitrate")["ascii_voltage"],5,2) << "," << std::endl;
          fout << "    \"ascii_current\": " << decimal(Doc_L0.at("bist_Nitrate")["ascii_current"],6,3) << "," << std::endl;
          fout << "    \"ascii_ref_detector_mean\": " << Doc_L0.at("bist_Nitrate")["ascii_ref_detector_mean"] << "," << std::endl;
          fout << "    \"ascii_ref_detector_std\": " << Doc_L0.at("bist_Nitrate")["ascii_ref_detector_std"] << "," << std::endl;
          fout << "    \"ascii_dark_spectrum_mean\": " << Doc_L0.at("bist_Nitrate")["ascii_dark_spectrum_mean"] << "," << std::endl;
          fout << "    \"ascii_dark_spectrum_std\": " << Doc_L0.at("bist_Nitrate")["ascii_dark_spectrum_std"] << "," << std::endl;
          fout << "    \"ascii_sensor_salinity\": " << decimal(Doc_L0.at("bist_Nitrate")["ascii_sensor_salinity"],5,2) << "," << std::endl;
          fout << "    \"ascii_sensor_nitrate\": " << decimal(Doc_L0.at("bist_Nitrate")["ascii_sensor_nitrate"],5,2) << "," << std::endl;
          fout << "    \"ascii_residual_rms\": " << decimal(Doc_L0.at("bist_Nitrate")["ascii_residual_rms"],8,7) << "," << std::endl;
          fout << "    \"ascii_FIT_pixel_begin\": " << Doc_L0.at("bist_Nitrate")["ascii_FIT_pixel_begin"] << "," << std::endl;
          fout << "    \"ascii_FIT_pixel_end\": " << Doc_L0.at("bist_Nitrate")["ascii_FIT_pixel_end"] << std::endl;
  	  fout << "  }";
	}

	if ( Doc_L0.contains(std::string{ "Science" }) ) {
 	  fout << "," << std::endl; //finish the previous block
	  const auto& matrix = Doc_L0.at("Science");
  	  fout << "  \"Science\": {" << std::endl;
          fout << "    \"Files\": " << Doc_L0.at("Science")["Files"] << "," << std::endl;
          fout << "    \"Space_available_MB\": " << Doc_L0.at("Science")["Space_available_MB"] << "," << std::endl;
          fout << "    \"CTD_Errors\": " << Doc_L0.at("Science")["CTD_Errors"] << "," << std::endl;
          fout << "    \"Pressure_Jumps\": " << Doc_L0.at("Science")["Pressure_Jumps"] << std::endl;
  	  fout << "  }";
	}
	
// Note: in BEACON MODE, Engineering data is saved to "BIT" within the json
	if ( Doc_L0.contains(std::string{ "Engineering_Data" }) || ( SBck[0] == 1 && !isBEACON && !isBIST) ) {
  	  fout << "," << std::endl; //finish the previous block
	  if ( Doc_L0.contains(std::string{ "Engineering_Data" }) ) {
	  const auto& matrix = Doc_L0.at("Engineering_Data");
  	  fout << "  \"Engineering_Data\": {" << std::endl;
          int i = 0;
	  for (const auto& loop : matrix.items()) {
  	    i++;
	    std::string name = "\"" + loop.key();
 	    fout << "    " << std::setw(9) << name << "\": {";
	    std::stringstream ss;
	    ss << loop.value()["value"];
 	    fout << " \"value\": " << std::setw(8) << ss.str() << ",";
	    ss.str("");
	    ss << loop.value()["unit"];
  	    fout << " \"unit\": " << std::setw(6) << ss.str() << ","; 
  	    fout << " \"description\": " << matrix[loop.key()]["description"];
     	    if ( i == matrix.size() ) {
              fout <<  " }" << std::endl;
	    } else {
              fout <<  " },"<< std::endl;
	    }
	  }
	  }
          fout << "  }";
        }
      
// output "Mission" no matter what
 	fout << "," << std::endl; //finish the previous block
  	fout << "  \"Mission\": {" << std::endl;
        int i = 0;
	for (const auto& loop : RunMission.items()) {
	  i++;
	  std::string name = "\"" + loop.key();
 	  fout << "    " << std::setw(11) << name << "\": {";
	  std::stringstream ss;
	  // for some CONFIG that are disabled within firmware (e.g. Ice with version 10.1) mark -1
	  if ( name == "\"Ice_Mn" & std::round(10*float_version) <= 101 ) {
	    ss << -1;
          } else {
	    ss << loop.value()["value"];
	  }
       	  fout << " \"value\": " << std::setw(5) << ss.str() << ",";
	  ss.str("");
	  ss << loop.value()["unit"];
  	  fout << " \"unit\": " << std::setw(6) << ss.str() << ","; 
  	  fout << " \"description\": " << RunMission[loop.key()]["description"];
     	  if ( i == RunMission.size() ) {
            fout <<  " }" << std::endl;
	  } else {
            fout <<  " },"<< std::endl;
	  }
	}
        fout << "  }";

// output SCI no matter what
        fout << "," << std::endl; //finish the previous block
        vector<string> region_name  = { "region1","region2","region3","region4","region5" }; // five regions defined for each sensor
        fout << "  \"SCI_Parameters\": {" << std::endl;
        for( int i = 0; i < SCIsize ; i++) { 
	  if ( i == 0 ) { //zero "ctd"
            fout << "    \"" << sensor_name[i] << "\": {" << std::endl;
            fout << "      \"Enabled\": " << "-1" << "," << std::endl;
            fout << "      \"EnDrift\": " << "-1" << "," << std::endl;
            fout << "      \"DecDrift\": " << "-1" << "," << std::endl;
            fout << "      \"DataType\": " << "-1" << "," << std::endl;
            fout << "      \"PackType\": " << "-1" << "," << std::endl;
            fout << "      \"gain\": " << "-1" << "," << std::endl;
            fout << "      \"offset\": " << "-1" << "," << std::endl;
            for( int j = 0; j < 5; j++) {
              fout << "      \"" << region_name[j] << "\":";
              fout << " {\"zMin\": " << "-1";
              fout << ", \"zMax\": " << "-1";
              fout << ", \"dz\": " << "-1";
              fout << ", \"scanT\": " << "-1" << "}";
              if (j < 4)
                fout << ",";
              fout << std::endl;
            } 
            fout << "    }";
            if (i < 5)
              fout << ",";
            fout << std::endl;
          } else {
            fout << "    \"" << sensor_name[i] << "\": {" << std::endl;
            fout << "      \"Enabled\": " << RunSCI[sensor_name[i]]["Enabled"] << "," << std::endl;
            fout << "      \"EnDrift\": " << RunSCI[sensor_name[i]]["EnDrift"] << "," << std::endl;
            fout << "      \"DecDrift\": " << RunSCI[sensor_name[i]]["DecDrift"] << "," << std::endl;
            fout << "      \"DataType\": " << RunSCI[sensor_name[i]]["DataType"] << "," << std::endl;
            fout << "      \"PackType\": " << RunSCI[sensor_name[i]]["PackType"] << "," << std::endl;
            fout << "      \"gain\": " << RunSCI[sensor_name[i]]["gain"] << "," << std::endl;
            fout << "      \"offset\": " << RunSCI[sensor_name[i]]["offset"] << "," << std::endl;
            for( int j = 0; j < 5; j++) {
              fout << "      \"" << region_name[j] << "\":";
              fout << " {\"zMin\": " << decimal(RunSCI[sensor_name[i]][region_name[j]]["zMin"],4,0);
              fout << ", \"zMax\": " << decimal(RunSCI[sensor_name[i]][region_name[j]]["zMax"],4,0);
              fout << ", \"dz\": " << decimal(RunSCI[sensor_name[i]][region_name[j]]["dz"],4,0);
              fout << ", \"scanT\": " << decimal(RunSCI[sensor_name[i]][region_name[j]]["scanT"],4,0) << "}";
              if (j < 4)
                fout << ",";
              fout << std::endl;
            } 
            fout << "    }";
            if (i < 5)
              fout << ",";
            fout << std::endl;
          }
        }
        fout << "  }";
 
	if ( Doc_L0.contains(std::string{ "Pump" }) || ( SBck[4] == 1 && !isBEACON && !isBIST) ) {
  	  fout << "," << std::endl; //finish the previous block
  	  fout << "  \"Pump\": [" << std::endl;
	  if ( Doc_L0.contains(std::string{ "Pump" }) ) {
	  const auto& matrix = Doc_L0.at("Pump");
          size_t i = 0;
	  for (const auto& row : matrix) {
             i++; 
             fout << "    { \"PRES\": " << decimal(row["PRES"],7,resp) << ", ";
	     int iTime = matrix.contains(std::string{"TIME"});
             if ( iTime ) {
               fout << "    { \"TIME\": " << row["TIME"] << ", ";
	     }
             fout << "\"current\": " <<  decimal(row["current"],5,0) << ", ";
             fout << "\"voltage\": " << decimal(row["voltage"],5,2) << ", ";
             fout << "\"pump_time\": " << decimal(row["pump_time"],3,0) << ", ";
             fout << "\"vac_start\": " << decimal(row["vac_start"],3,0) << ", ";
             fout << "\"vac_end\": " << decimal(row["vac_end"],3,0) << ", ";
             fout << "\"phase\": " << decimal(row["phase"],2,0) << ", ";
             fout << "\"description\": " << row["description"];
     	     if ( i == matrix.size() ) {
               fout <<  " }" << std::endl;
	     } else {
               fout <<  " }," << std::endl;
	     }
	  } 
	  } 
	  fout << "  ]";
	}

	if ( Doc_L0.contains(std::string{ "CTD_Raw" }) || ( SBck[1]==1 && writeRawCTD && !isBEACON && !isBIST ) ) {
  	  fout << "," << std::endl; //finish the previous block
      	  fout << "  \"" << "CTD_Raw" << "\": [" << std::endl;
	  if ( Doc_L0.contains(std::string{ "CTD_Raw" }) ) {
	    write_CTD( fout, "CTD_Raw" , Doc_L0, 1 );
	  }
	  fout << "  ]";
	}

	if ( Doc_L0.contains(std::string{ "CTD_Binned" }) || ( writeBinCTD && !isBEACON && !isBIST ) ) {
  	  fout << "," << std::endl; //finish the previous block
      	  fout << "  \"" << "CTD_Binned" << "\": [" << std::endl;
	  if ( Doc_L0.contains(std::string{ "CTD_Binned" }) ) {
	    write_CTD( fout, "CTD_Binned" , Doc_L0, 0 );
	  }
	  fout << "  ]";
	}

	if ( Doc_L0.contains(std::string{ "CTD_Drift" }) || ( SBck[5]==1 && writeDriftCTD && !isBEACON && !isBIST && !isDPLY ) ) {
  	  fout << "," << std::endl; //finish the previous block
      	  fout << "  \"" << "CTD_Drift" << "\": [" << std::endl;
	  if ( Doc_L0.contains(std::string{ "CTD_Drift" }) ) {
	    write_CTD( fout, "CTD_Drift" , Doc_L0 , 2 );
	  }
	  fout << "  ]";
	}


	if ( Doc_L0.contains(std::string{ "DO_Discrete" }) || ( !isBIST && !isBEACON && BGCen[1] == 1 ) ) {
  	  fout << "," << std::endl; //finish the previous block
      	  fout << "  \"DO_Discrete\": [" << std::endl;
	  if ( Doc_L0.contains(std::string{ "DO_Discrete" }) ) {
 	    write_BGC( fout, SndDO, "DO_Discrete", Doc_L0 );
	  }
	  fout << "  ]";
	}


	if ( Doc_L0.contains(std::string{ "DO_Surface" }) || ( !isBIST && !isBEACON && writeDOair ) ) {
  	  fout << "," << std::endl; //finish the previous block
      	  fout << "  \"DO_Surface\": [" << std::endl;
	  if ( Doc_L0.contains(std::string{ "DO_Surface" }) ) {

	  //determine which DO_Surface PRES to remove
	  size_t iter = 0;
	  size_t limit;
	  const auto& matrixp = Doc_L0.at("DO_Surface");
	  float PrevPRES = 9999.;
	  bool found = false;
	  for (const auto& row : matrixp) {
	     iter++;
             for (auto const& [keyr,valr] : row.items()) {
	       if ( keyr == "PRES" ) {
	         float PresPRES = valr;
                 if ( round( 100 * PresPRES ) >= round( 100 * PrevPRES ) ) { //end reporting once the PRES stops decreasing
		   limit=iter;
		   found = true;
		   break;
		 } else {
	           PrevPRES = PresPRES;
		 }
	       }
	     }
	     if (found) {
	       break;
	     }
	  }

	  const auto& matrix = Doc_L0.at("DO_Surface");
	  size_t i = 0;
	  for (const auto& row : matrix) {
             i++; 
             fout << "    { ";
             size_t j = 0;
             for (auto const& [keyr,valr] : row.items()) {
               j++; 
	       if ( keyr == "PRES" || ( j == 1 && SndPr[1] == 1 ) ) {
	         if ( keyr == "PRES" && i < limit ) {
                   fout << "\"PRES\": " << decimal(row["PRES"],7,resp) << ", ";
		 } else {
                   fout << "\"PRES\": " << decimal(-999.,7,resp) << ", ";
		 }
	       }
	       if ( keyr != "PRES" ) {
                 fout << "\"" << keyr << "\": " << decimal(row[keyr],6,3);
     	         if ( j != row.size()) {
                   fout << ", ";
	         }
	       }
     	       if ( i == matrix.size() & j == row.size()) {
                 fout <<  " }" << std::endl;
	       } else if ( j == row.size()) {
                 fout <<  " }," << std::endl;
	       }
	     }
          } 
          } 
	  fout << "  ]";
	}

	if ( Doc_L0.contains(std::string{ "DO_Drift" }) || ( !isBIST && !isBEACON && !isDPLY && BGCen[1] == 1 && BGCed[1] == 1 ) ) {
  	  fout << "," << std::endl; //finish the previous block
      	  fout << "  \"DO_Drift\": [" << std::endl;
	  if ( Doc_L0.contains(std::string{ "DO_Drift" }) ) {
 	    write_BGC_Drift( fout, SndDO, "DO_Drift", BGCdd[1], BGCsd[1], Doc_L0 );
	  }
	  fout << "  ]";
	}

  
        if ( Doc_L0.contains(std::string{ "pH_Discrete" }) || ( !isBIST && !isBEACON && BGCen[2] == 1 ) ) {
  	  fout << "," << std::endl; //finish the previous block
      	  fout << "  \"pH_Discrete\": [" << std::endl;
	  if ( Doc_L0.contains(std::string{ "pH_Discrete" }) ) {

      	    if ( phDec > 1 ) { // if phDec > 1 then must send in modified Vk, Ik and Ib
              json Doc_pH; //create uninitialized json object
	      Doc_pH = Doc_L0.at(std::string{ "pH_Discrete" });
	      size_t sz = Doc_pH.size();
	      std::cout << "Doc_pH size " << Doc_pH.size() << sz << std::endl;
	      std::vector<float> Vk(sz,999.);
	      std::vector<float> Ik(sz,999.);
	      std::vector<float> Ib(sz,999.);
              int dec_lim = 0;
	      for ( const auto& row : Doc_pH ) { //save Vk, Ik, Ib to independent vectors
                for (auto const& [keyr,valr] : row.items()) {
      		     if ( keyr == "VK_PH" ) {
			     Vk[dec_lim]=valr;
		     } else if ( keyr == "IK_PH" ) {
			     Ik[dec_lim]=valr;
		     } else if ( keyr == "IB_PH" ) {
			     Ib[dec_lim]=valr;
		     }
	        }
		if ( Vk[dec_lim] < 999. || Ik[dec_lim] < 999. || Ib[dec_lim] < 999. ) { //is this a valid fillvalue?
		  dec_lim++;
		}
	      }
	      dec_lim -= 1; //set it back to the length of Vk
	      size_t iter = phDec ;
	      size_t i = 0;
	      for ( const auto& row : Doc_pH ) { //Now reapply the data to Doc_pH with Decimation
		if ( iter < phDec ) {
			Doc_pH["VK_PH"]=999.;
			Doc_pH["IK_PH"]=999.;
			Doc_pH["IB_PH"]=999.;
		        iter++;
		} else {
			Doc_pH["VK_PH"]=Vk[i];
			Doc_pH["IK_PH"]=Ik[i];
			Doc_pH["IB_PH"]=Ib[i];
			iter=0;
			i++;
			if ( i > dec_lim ) {
				std::cout << "Warning about to overrun pH Decimation array " << std::endl;
			}
		}
	      }
 	      write_BGC( fout, SndpH, "pH_Discrete", Doc_pH );
	    } else { // phDec ==1 so just send original json		   
 	      write_BGC( fout, SndpH, "pH_Discrete", Doc_L0 );
	    }
	  }
	  fout << "  ]";
	}


	if ( Doc_L0.contains(std::string{ "pH_Drift" }) || ( !isBIST && !isBEACON && !isDPLY && BGCen[2] == 1 && BGCed[2] == 1 ) ) {
  	  fout << "," << std::endl; //finish the previous block
      	  fout << "  \"DO_Drift\": [" << std::endl;
	  if ( Doc_L0.contains(std::string{ "pH_Drift" }) ) {
 	    write_BGC_Drift( fout, SndpH, "pH_Drift", BGCdd[2], BGCsd[2], Doc_L0 );
	  }
	  fout << "  ]";
	}

        if ( Doc_L0.contains(std::string{ "ECO_Discrete" }) || ( !isBIST && !isBEACON && BGCen[3] == 1 ) ) {
  	  fout << "," << std::endl; //finish the previous block
      	  fout << "  \"ECO_Discrete\": [" << std::endl;
	  if ( Doc_L0.contains(std::string{ "ECO_Discrete" }) ) {
 	    write_BGC( fout, SndECO, "ECO_Discrete", Doc_L0 );
	  }
	  fout << "  ]";
	}

	if ( Doc_L0.contains(std::string{ "ECO_Drift" }) || ( !isBIST && !isBEACON && !isDPLY && BGCen[3] == 1 && BGCed[3] == 1 ) ) {
  	  fout << "," << std::endl; //finish the previous block
      	  fout << "  \"ECO_Drift\": [" << std::endl;
	  if ( Doc_L0.contains(std::string{ "ECO_Drift" }) ) {
 	    write_BGC_Drift( fout, SndECO, "ECO_Drift", BGCdd[3], BGCsd[3], Doc_L0 );
	  }
	  fout << "  ]";
	}

        if ( Doc_L0.contains(std::string{ "OCR_Discrete" }) || ( !isBIST && !isBEACON && BGCen[4] == 1 ) ) {
  	  fout << "," << std::endl; //finish the previous block
      	  fout << "  \"OCR_Discrete\": [" << std::endl;
	  if ( Doc_L0.contains(std::string{ "OCR_Discrete" }) ) {
 	    write_BGC( fout, SndOCR, "OCR_Discrete", Doc_L0 );
	  }
	  fout << "  ]";
	}

	if ( Doc_L0.contains(std::string{ "OCR_Drift" }) || ( !isBIST && !isBEACON && !isDPLY && BGCen[4] == 1 && BGCed[4] == 1 ) ) {
  	  fout << "," << std::endl; //finish the previous block
      	  fout << "  \"OCR_Drift\": [" << std::endl;
	  if ( Doc_L0.contains(std::string{ "OCR_Drift" }) ) {
 	    write_BGC_Drift( fout, SndOCR, "OCR_Drift", BGCdd[4], BGCsd[4], Doc_L0 );
	  }
	  fout << "  ]";
	}

        if ( Doc_L0.contains(std::string{ "NITRATE_Discrete" }) || ( !isBIST && !isBEACON && BGCen[5] == 1 ) ) {
  	  fout << "," << std::endl; //finish the previous block
      	  fout << "  \"NITRATE_Discrete\": [" << std::endl;
	  if ( Doc_L0.contains(std::string{ "NITRATE_Discrete" }) ) {
 	    write_BGC( fout, SndNitrate, "NITRATE_Discrete", Doc_L0 );
	  }
	  fout << "  ]";
	}

	fout << std::endl;
        fout << "}" << std::endl;
        return;
}  // end of rewrite_json

/////////////////////////////////////////////////////////////////////////////////////////////
// 
int determine_resolution (int digits) {
    int res;
    if (digits > 1000) {
	  res=4;
    } else if ( digits > 100 ) {
	  res=3;
    } else if ( digits > 10 ) {
	  res=2;
    } else {
	  res=1;
    }
    return res;
}
/////////////////////////////////////////////////////////////////////////////////////////////
int convert_to_hours (float value) {
	int res;
        if ( value > 0 & value < 11 ) { //presume has been converted to hours with this range of values
          res = static_cast<int>(std::round(60 * value));
	} else {
          res = static_cast<int>(std::round(value));
	}
    return res;
}
/////////////////////////////////////////////////////////////////////////////////////////////
void write_CTD (std::ofstream& fout, std::string varname, json Doc_L0c, int chan) {
        float ShalCutoff = -5.0; //define non reachable default for other channels
	if ( chan == 0 ) { //identify shallow cutoff of Binned CTD data
	  if ( RunMission.contains("BLOK") && RunMission.contains("CTDofZ") ) {
            float cB = RunMission["BLOK"]["value"];
            float cZ = RunMission["CTDofZ"]["value"];
            ShalCutoff = cZ - ( cB / 2. );
	  }
//          std::cout << "ShalCutoff " << ShalCutoff << std::endl;
	}
	if ( Doc_L0c.contains(varname) ) {
	  const auto& matrix = Doc_L0c.at(varname);
          size_t i = 0;
          size_t skip = 0;
          std::vector<int> PTS(3,0);
	  for (const auto& row : matrix) {
	     if ( row.contains("PRES") ) { //escape if this data is shallower than ShalCutoff
	 	if (row["PRES"] <= ShalCutoff ) { //escape if this data is shallower than ShalCutoff
			 std::cout << "Not reporting Binned CTD data at PRES = " << row["PRES"] << std::endl;
			 ++skip;
			 continue;
	        }
	     }
             i++; 
             fout << "    { ";
             if ( i == 1 ) { //first iteration see if missing variables
               for (auto const& [keyr,valr] : row.items()) {
      		     if ( keyr == "PRES" ) {
			     PTS[0]=1;
		     } else if ( keyr == "TEMP" ) {
			     PTS[1]=1;
		     } else if ( keyr == "PSAL" ) {
			     PTS[2]=1;
		     }
	       }
	     }
             std::vector<int> tPTS = PTS;
             for (auto const& [keyr,valr] : row.items()) {
      	       if ( keyr == "PRES" | tPTS[0] == 0 ) {
                 if ( keyr == "PRES" ) {
                   fout << "\"PRES\": " << decimal(row["PRES"],7,resp) << ", ";
		 } else {
                   fout << "\"PRES\": " << decimal(-999,7,resp) << ", ";
	           tPTS[0]=1;
		 }
	       } 
      	       if ( keyr == "TEMP" | tPTS[1] == 0 ) {
                 if ( keyr == "TEMP" ) {
                   fout << "\"TEMP\": " <<  decimal(row["TEMP"],6,rest) << ", ";
		 } else {
                   fout << "\"TEMP\": " << decimal(-999,6,rest) << ", ";
	           tPTS[1]=1;
		 }
	       }
      	       if ( keyr == "PSAL" | tPTS[2] == 0 ) {
                 if ( keyr == "PSAL" ) {
                   fout << "\"PSAL\": " <<  decimal(row["PSAL"],7,ress);
		 } else {
                   fout << "\"PSAL\": " << decimal(-999,7,ress) << ", ";
	           tPTS[2]=1;
		 }
	       }
	     }
     	     if ( i == matrix.size() - skip ) {
               fout <<  " }" << std::endl;
	     } else {
               fout <<  " }," << std::endl;
	     }
          }
        }
	return; 
}
/////////////////////////////////////////////////////////////////////////////////////////////
// Function Declaration 
std::vector<int> ApplyDataT (std::string sensor, int dataT) {
std::vector<int> para(7,0); 
para[0]=1; //default to pressure being sent for this BGC sensor...might be changed upon return from function
if ( sensor == "DO" ) {
	para[1]=1; //always sent
	para[4]=1; //always sent
	switch (dataT) {
		case 0: //default
			break;
		case 1:
			para[2]=1;
			break;
		case 2:
			para[2]=1;
			para[3]=1;
			break;
		default:
			std::cout << "DO dataT undefined" << std::endl;
	}
} else if ( sensor == "pH" ) {
	para[1]=1; //always sent
	para[2]=1; //always sent
	para[3]=1; //always sent 
	para[4]=1; //always sent 
} else if ( sensor == "ECO" ) {
	para[1]=1; //always sent
	para[2]=1; //always sent
	para[3]=1; //always sent
} else if ( sensor == "OCR" ) {
	para[1]=1; //always sent
	para[2]=1; //always sent
	para[3]=1; //always sent
	para[4]=1; //always sent
} else if ( sensor == "Nitrate"  ) {
	para[1]=1; //always sent
	para[2]=1; //always sent
        para[6]=1; //Spectrum always sent
	switch (dataT) {
		case 0: //default
			break;
		case 1:
			para[3]=1;
			break;
		case 2:
			para[3]=1;
			para[4]=1;
			break;
		case 3:
			para[3]=1;
			para[4]=1;
			para[5]=1;
			break;
		default:
			std::cout << "Nitrate dataT undefined" << std::endl;
	}
} else {
	std::cout << "This sensor is not one that has valid dataT defined " << sensor << std::endl;
}
return para;
}
/////////////////////////////////////////////////////////////////////////////////////////////
// Function Declaration 
void write_BGC(std::ofstream& fout, std::vector<int> Snd, std::string varname, json Doc_L0c) {
	if ( Doc_L0c.contains(varname) ) {
	  const auto& matrix = Doc_L0c.at(varname);
          size_t i = 0;
	  for (const auto& row : matrix) {
             i++; 
             fout << "    { ";
  	     size_t j = 0;
             for (const auto& loop : config_template["prof"][varname].items()) { // loop over config.json
               j++; 
               if ( Snd[j-1] == 1 ) { //should have been sent
  	         size_t k = 0;
                 for (auto const& [keyr,valr] : row.items()) {
                   k++; 
	           if ( keyr == loop.key() ) { 
	             if ( keyr == "PRES" ) {
                       fout << "\"" << loop.key() << "\": " << decimal(valr,loop.value()["col_width"],resp);
	             } else {
                       fout << "\"" << loop.key() << "\": " << decimal(valr,loop.value()["col_width"],loop.value()["col_precision"]);
	             }
	             break;
	           } else if ( keyr == "Spectrum" ) { //special for spectrum
                     fout << "\"Spectrum\": " << valr; 
	             break;
	           }
	           if ( k == row.size() ) {  //write in fillvalue if cant find in present data
                     fout << "\"" << loop.key() << "\": " << decimal(-999,loop.value()["col_width"],loop.value()["col_precision"]);
	           }
	         }
     	         if ( j != config_template["prof"][varname].size() && loop.key() != "S01") { //include special exclusion for spectrum ("S01")
                   fout << ", ";
	         }
	       }
     	       if ( i == matrix.size() & j == config_template["prof"][varname].size()) {
                 fout <<  " }" << std::endl;
	       } else if ( j == config_template["prof"][varname].size()) {
                 fout <<  " }," << std::endl;
	       }
	     }
          } 
        } 
return; 
}
/////////////////////////////////////////////////////////////////////////////////////////////
// Function Declaration 
void write_BGC_Drift(std::ofstream& fout, std::vector<int> Snd, std::string varname, int dd, int sd, json Doc_L0c) {
	if ( Doc_L0c.contains(varname) ) {
//	  std::cout << "in Drift " << varname << " " << dd << " " << sd << std::endl;
	  const auto& matrix = Doc_L0c.at(varname);
// determine stretching of array by "dd" and "sd"
	  int nrec = matrix.size();
	  int n_dd_nrec = ( ( nrec - 1 ) * dd ) + 1;
	  int n_sd_nrec = (1 + sd/100.) * (n_dd_nrec - 1) + 1;
//	  std::cout << nrec << " " << n_dd_nrec << " " << n_sd_nrec << std::endl;
          std::stringstream fillLine; // initialize the drift decimation fillvalue line
          fillLine << "    { ";
  	  size_t j = 0;
          for (const auto& loop : config_template["prof"][varname].items()) { // loop over config.json to build fillLine
            j++; 
            if ( Snd[j-1] == 1 ) { //should have been sent
	      if ( loop.key() == "PRES" ) {
                fillLine << "\"" << loop.key() << "\": " << decimal(-999,loop.value()["col_width"],resp);
	      } else {
                fillLine << "\"" << loop.key() << "\": " << decimal(-999,loop.value()["col_width"],loop.value()["col_precision"]);
	      }
     	      if ( j != config_template["prof"][varname].size() ) { 
                fillLine << ", ";
	      }
	    }
	  }
          fillLine << " }";
//	  std::cout << "fillLine " << fillLine.str() << std::endl;
	  //output the number of initial fillLine due to sd
	  for (size_t i = 0; i < n_sd_nrec - n_dd_nrec; ++i ) {
             fout << fillLine.str() << std::endl;
          }
	  size_t i = 0;
	  for (const auto& row : matrix) {
             i++; 
             fout << "    { ";
  	     size_t j = 0;
             for (const auto& loop : config_template["prof"][varname].items()) { // loop over config.json
               j++; 
               if ( Snd[j-1] == 1 ) { //should have been sent
  	         size_t k = 0;
                 for (auto const& [keyr,valr] : row.items()) {
                   k++; 
	           if ( keyr == loop.key() ) { 
	             if ( keyr == "PRES" ) {
                       fout << "\"" << loop.key() << "\": " << decimal(valr,loop.value()["col_width"],resp);
	             } else {
                       fout << "\"" << loop.key() << "\": " << decimal(valr,loop.value()["col_width"],loop.value()["col_precision"]);
	             }
	             break;
	           }
	           if ( k == row.size() ) {  //write in fillvalue if cant find in present data
                     fout << "\"" << loop.key() << "\": " << decimal(-999,loop.value()["col_width"],loop.value()["col_precision"]);
	           }
	         }
     	         if ( j != config_template["prof"][varname].size() ) { 
                   fout << ", ";
	         }
	       }
     	       if ( i == matrix.size() & j == config_template["prof"][varname].size()) {
                 fout <<  " }" << std::endl;
	       } else if ( j == config_template["prof"][varname].size()) {
                 fout <<  " }," << std::endl;
                 // now put in DecDrift "dd" fillvalue lines
                 for (int i = 1;  i < dd; ++i) {
                     fout << fillLine.str() << std::endl;
                 }
	       }
	     }
          } 
        } 
return; 
}
/////////////////////////////////////////////////////////////////////////////////////////////
// Function Declaration 
void copyLine ( std::ofstream& fout, std::string item, std::string endc, json Doc_L0c ) {
	if ( Doc_L0c.contains(item) ) {
          if ( endc != "" ) {
		  fout << endc << std::endl;
	  }
          fout << "  \"" << item << "\": " << Doc_L0c.at(item);
	}
	return;
}

