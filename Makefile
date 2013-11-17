CXXFLAGS	= -I/usr/X11R6/include -DX11 -Wall
LDLIBS	= libglfw_linux.a -L/usr/X11R6/lib -lXmu -lXi -lXext -lX11 -lGL -lXrandr -lrt -lpthread -lm -L/usr/local/lib -lopencv_core -lopencv_highgui
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
