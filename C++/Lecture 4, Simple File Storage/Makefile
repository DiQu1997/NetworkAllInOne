CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -pedantic
TARGET = file_storage_server

all: $(TARGET)

$(TARGET): main.cpp FileStorageServer.cpp FileStorageServer.h
	$(CXX) $(CXXFLAGS) -o $(TARGET) main.cpp FileStorageServer.cpp

clean:
	rm -f $(TARGET)
