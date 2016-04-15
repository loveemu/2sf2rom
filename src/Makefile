#!/bin/make
CC = gcc
CFLAGS = -O2 -Wall --std=c14
CXX = g++
CXXFLAGS = -O2 -Wall --std=c++14
LDFLAGS = -lm -lz -lzlibcomplete
TARGET = 2sf2rom
SRCS = $(TARGET).cpp psf_file.cpp
OBJS := $(SRCS:.cpp=.o)

.PHONY: all clean
.SUFFIXES: .c .cpp .o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) -o $@ $(OBJS) $(LDFLAGS)

clean:
	$(RM) $(TARGET)
	$(RM) $(OBJS)

.c.o:
	$(CC) $(CFLAGS) -o $@ -c $^
.cpp.o:
	$(CXX) $(CXXFLAGS) -o $@ -c $^
