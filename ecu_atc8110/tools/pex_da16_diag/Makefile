CXX ?= g++
CXXFLAGS ?= -O2 -std=c++17 -Wall -Wextra

TARGET = pex_da16_diag
SRC = pex_da16_diag.cpp

INCLUDES = -I../..

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET) ecu_output_map.json
