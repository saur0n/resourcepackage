################################################################################
#   Resource management library
#   Build rules
#   
#   © 2026, Sauron
################################################################################

CC=$(if $(TOOLCHAIN), /usr/bin/$(TOOLCHAIN)-)g++
CFLAGS=-Os -Wall #-Wextra -Wno-unused-parameter
CXXFLAGS=$(CFLAGS)
CXXFLAGS_LIB=$(CXXFLAGS) -shared -fPIC
SOURCES=*.cpp
HEADERS=*.hpp
LIBRARIES=-lunix++

all: libresourcepackage.so mkresourcepackage

libresourcepackage.so: $(SOURCES) $(HEADERS)
	$(CC) $(CXXFLAGS_LIB) -o $@ $(SOURCES) $(LIBRARIES)

mkresourcepackage: tool/*.cpp
	$(CC) $(CXXFLAGS) -o $@ $^ $(LIBRARIES)

unittest: ut/unittest.cpp tool/unescape.cpp
	$(CC) $(CXXFLAGS) -o $@ $^

test: unittest
	./unittest

install: libresourcepackage.so mkresourcepackage
	install --strip libresourcepackage.so /usr/local/lib64
	install --strip mkresourcepackage /usr/local/bin
	install -m 644 $(HEADERS) /usr/include/

clean:
	rm -f *.o *.so
	rm -f unittest

.PHONY:
	all release test install clean
