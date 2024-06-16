TARGET = Pinger

CXX = g++
CXXFLAGS = -Wall -std=c++17

HEADERS = Application.h Pinger.h

SRCS = main.cpp Application.cpp Pinger.cpp

OBJS = $(SRCS:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)

%.o: %.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
