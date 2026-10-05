CXX ?= g++
ZLIB := ../project64-develop/Source/3rdParty/zlib
CXXFLAGS := -O2 -Wall -I$(ZLIB)

all: PJ64KeyGen.exe PJ64KeyGenGui.exe

PJ64KeyGen.exe: PJ64KeyGen.cpp PJ64KeyCore.h
	$(CXX) $(CXXFLAGS) PJ64KeyGen.cpp -lz -o PJ64KeyGen.exe -static

PJ64KeyGenGui.exe: PJ64KeyGenGui.cpp PJ64KeyGenGui.h PJ64KeyGenGui.rc PJ64KeyGenGui.manifest PJ64KeyCore.h
	windres -I. -O coff -o PJ64KeyGenGui.res PJ64KeyGenGui.rc
	$(CXX) $(CXXFLAGS) -mwindows PJ64KeyGenGui.cpp PJ64KeyGenGui.res -lz -ladvapi32 -luser32 -lcomdlg32 -lcomctl32 -o PJ64KeyGenGui.exe -static

clean:
	-del /q PJ64KeyGen.exe PJ64KeyGenGui.exe PJ64KeyGenGui.res

.PHONY: all clean