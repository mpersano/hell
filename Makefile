CXX = g++
LD = g++

OBJS = $(CXXFILES:.cpp=.o)

CXXFLAGS = `pkg-config --cflags glew sdl gl glu` -Wall -g -O2 -std=c++0x # -DDUMP_FRAMES
LIBS = `pkg-config --libs glew sdl gl glu`

CXXFILES = \
	main.cpp \
	world.cpp \
	panic.cpp \
	piece_pattern.cpp

TARGET = hell

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
