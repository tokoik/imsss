CXXFLAGS	= -I/usr/X11R6/include -DX11 -Wall
LDLIBS	= libglfw_linux.a -L/usr/X11R6/lib -lX11 -lGL -lXrandr -lrt -lpthread -lm
OBJECTS	= $(patsubst %.cpp,%.o,$(wildcard *.cpp))
TARGET	= imsss

.PHONY: clean depend

$(TARGET): $(OBJECTS)
	$(LINK.cc) $^ $(LOADLIBES) $(LDLIBS) -o $@

clean:
	-$(RM) $(TARGET) *.o *~ .*~ core

depend:
	$(CXX) $(CXXFLAGS) -MM *.cpp > $(TARGET).dep

-include $(wildcard *.dep)
