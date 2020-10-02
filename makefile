CC=g++

all:
	$(CC) -std=c++17 -Iinclude/ -lcurl BimayInstantVicon/main.cpp -o bimayinstv -lstdc++fs