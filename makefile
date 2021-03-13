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

all: $(OBJ)
	$(CC) $(LIBS) $(OBJ) -o bimayinstv -lstdc++fs

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o bimayinstv