CXX ?= g++
CXXFLAGS ?= -O2 -std=c++17 -Wall -Wextra

TARGET = pex_scan
SRC = pex_da16_auto_mapper.cpp

INCLUDES = -I../adquisicion_datos

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET) ecu_io_map.json
