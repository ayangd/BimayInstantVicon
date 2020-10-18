CC=g++

all: BimayInstantVicon/main.cpp BimayInstantVicon/main.hpp
	$(CC) -std=c++17 -Iinclude/ -lcurl -lcryptopp BimayInstantVicon/main.cpp -o bimayinstv -lstdc++fs