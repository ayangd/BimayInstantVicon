CC   = g++
IDIR = include
ODIR = BimayInstantVicon
SDIR = BimayInstantVicon
STD  = c++17
LIBS = -lcurl -lcryptopp

_OBJ = BimayInterface Credential main Utils
OBJ  = $(patsubst %,$(ODIR)/%.o,$(_OBJ))

$(ODIR)/%.o: $(SDIR)/%.cpp
	$(CC) -std=$(STD) -O2 -c -o $@ $< -I$(IDIR)

include/base64.o: include/base64.cpp include/base64.h
	$(CC) -std=$(STD) -O2 -c -o $@ $<

all: $(OBJ) include/base64.o
	$(CC) $(LIBS) $(OBJ) include/base64.o -o bimayinstv -lstdc++fs

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o bimayinstv
	rm -f include/base64.o
