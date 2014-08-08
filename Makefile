CXX = g++
LD = g++

OBJS = $(CXXFILES:.cpp=.o)

CXXFLAGS = `pkg-config --cflags glew sdl gl glu libpng` -Wall -g -O2 -std=c++0x
LIBS = `pkg-config --libs glew sdl gl glu libpng`

CXXFILES = \
	main.cpp \
	world.cpp \
	panic.cpp

TARGET = demo

.cpp.o:
	$(CXX) $(CXXFLAGS) -c $<

$(TARGET): $(OBJS)
	$(LD) $(OBJS) -o $@ $(LIBS)

depend: .depend

.depend: $(CXXFILES)
	rm -f .depend
	$(CXX) $(CXXFLAGS) -MM $^ > .depend;

clean:
	rm -f *o $(TARGET)

include .depend

.PHONY: all clean depend
