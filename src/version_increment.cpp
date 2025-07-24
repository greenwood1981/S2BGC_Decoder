#include <iostream>
#include <fstream>

int main() {
	std::string header,major,minor;

	std::ifstream fin("src/version.cpp");

	std::getline(fin,header);
	std::getline(fin,major);
	std::getline(fin,minor);
	fin.close();

	int minor_version = std::stoi( minor.substr(20) );
	minor_version++;

	std::cout << "Compiling MINOR_VERSION " << minor_version << std::endl;

	std::ofstream fout("src/version.cpp",std::ios::out);
	fout << header << std::endl;
	fout << major << std::endl;
	fout << "int MINOR_VERSION = " << minor_version << ";" << std::endl;
	fout.close();

	return 0;
}
