testpi: lora.cpp
	g++ -O -o lora lora.cpp -lwiringPi

listen: lora-listen.cpp
	g++ -O -o listen lora-listen.cpp -lwiringPi

rxlog: lora-listen.cpp
	g++ -O -o rxlog lora-rxlog.cpp -lwiringPi
