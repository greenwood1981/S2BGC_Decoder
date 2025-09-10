.DEFAULT_GOAL: BGC_Decoder
folders := "logf"
OBJ := src/.objectfiles
OFILES=$(shell find $(OBJ) -name "*.o")

BGC_Decoder: src/version_increment src/.objectfiles/.empty src/.objectfiles/write_json.o src/.objectfiles/main.o src/.objectfiles/write_list.o src/.objectfiles/message.o src/.objectfiles/profile.o src/.objectfiles/pump.o src/.objectfiles/rise.o src/.objectfiles/fall.o src/.objectfiles/gps.o src/.objectfiles/bist.o src/.objectfiles/Engineering_Data.o src/.objectfiles/BIT.o src/.objectfiles/argo.o src/.objectfiles/SCI_parameter.o
	./src/version_increment
	g++ -g -std=c++20 $(OFILES) src/version.cpp -o BGC_Decoder

src/version_increment: src/version_increment.cpp
	g++ src/version_increment.cpp -o $@

# Creates necessary directory structure
src/.objectfiles/.empty:
	mkdir -p src/.objectfiles log data incoming
	touch src/.objectfiles/.empty

$(OBJ)/main.o: src/main.cpp src/output/write_log.h src/.objectfiles/hexfile.o src/.objectfiles/log.o
	g++ -g -std=c++20 -c src/main.cpp -lboost_filesystem -o $@

$(OBJ)/write_list.o: src/output/write_list.cpp
	g++ -g -std=c++20 -c src/output/write_list.cpp -o $@

$(OBJ)/write_json.o: src/output/write_json.cpp
	g++ -g -std=c++20 -c src/output/write_json.cpp -o $@

$(OBJ)/write_log.o: src/output/write_log.cpp
	g++ -g -std=c++20 -c src/output/write_log.cpp -o $@

$(OBJ)/hexfile.o: src/hexfile/hexfile.cpp src/hexfile/hexfile.h src/.objectfiles/write_json.o src/.objectfiles/write_list.o src/.objectfiles/message.o src/.objectfiles/profile.o src/.objectfiles/pump.o src/.objectfiles/rise.o src/.objectfiles/fall.o src/.objectfiles/gps.o src/.objectfiles/bist.o src/.objectfiles/Engineering_Data.o src/.objectfiles/BIT.o src/.objectfiles/argo.o
	g++ -g -std=c++20 -c src/hexfile/hexfile.cpp -o $@

$(OBJ)/message.o: src/hexfile/message.cpp src/hexfile/message.h src/.objectfiles/packet.o
	g++ -g -std=c++20 -c src/hexfile/message.cpp -o $@

$(OBJ)/packet.o: src/hexfile/packet.cpp src/hexfile/packet.h
	g++ -g -std=c++20 -c src/hexfile/packet.cpp -o $@

$(OBJ)/gps.o: src/gps/gps.cpp src/gps/gps.h
	g++ -g -std=c++20 -c src/gps/gps.cpp -o $@

$(OBJ)/argo.o: src/argo_mission/argo.cpp src/argo_mission/argo.h
	g++ -g -std=c++20 -c src/argo_mission/argo.cpp -o $@

$(OBJ)/fall.o: src/pressure_time_series/fall.cpp src/pressure_time_series/fall.h src/pressure_time_series/pressure_time_series.h
	g++ -g -std=c++20 -c src/pressure_time_series/fall.cpp -o $@

$(OBJ)/rise.o: src/pressure_time_series/rise.cpp src/pressure_time_series/rise.h src/pressure_time_series/pressure_time_series.h
	g++ -g -std=c++20 -c src/pressure_time_series/rise.cpp -o $@

$(OBJ)/pump.o: src/pump/pump.cpp src/pump/pump.h
	g++ -g -std=c++20 -c src/pump/pump.cpp -o $@

$(OBJ)/profile.o: src/profile/profile.cpp src/profile/profile.h src/.objectfiles/profile_segment.o
	g++ -g -std=c++20 -c src/profile/profile.cpp -o $@

$(OBJ)/profile_segment.o: src/profile/profile_segment.cpp src/profile/profile_segment.h
	g++ -g -std=c++20 -c src/profile/profile_segment.cpp -o $@

$(OBJ)/bist.o: src/bist/bist.cpp src/bist/bist.h
	g++ -g -std=c++20 -c src/bist/bist.cpp -o $@

$(OBJ)/Engineering_Data.o: src/Engineering/Engineering_Data.cpp src/Engineering/Engineering_Data.h
	g++ -g -std=c++20 -c src/Engineering/Engineering_Data.cpp -o $@

$(OBJ)/BIT.o: src/BIT/BIT.cpp src/BIT/BIT.h
	g++ -g -std=c++20 -c src/BIT/BIT.cpp -o $@

$(OBJ)/SCI_parameter.o: src/BIT/SCI_parameter.cpp src/BIT/SCI_parameter.h
	g++ -g -std=c++20 -c src/BIT/SCI_parameter.cpp -o $@

$(OBJ)/log.o: src/output/write_log.cpp src/output/write_log.h
	g++ -g -std=c++20 -c src/output/write_log.cpp -o $@

clean:
	rm BGC_Decoder $(OFILES)

L0toL1: src/L1/L0toL1_09Sep25.cpp
	g++ -g src/L1/L0toL1_09Sep25.cpp -o L0toL1

