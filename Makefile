CC=gcc
CXX=g++
COMMON_FLAGS=-m32 -g2 -fstack-protector-all -O2
IDIOMAN_FLAGS=$(COMMON_FLAGS) -Ifmt/include -DFMT_HEADER_ONLY

all: idioman

Main.o: Main.cpp ByteFile.h Error.h
	$(CXX) -o $@ $(IDIOMAN_FLAGS) -c Main.cpp

ByteFile.o: ByteFile.cpp ByteFile.h Error.h
	$(CXX) -o $@ $(IDIOMAN_FLAGS) -c ByteFile.cpp

idioman: Main.o ByteFile.o
	$(CXX) -o $@ $(IDIOMAN_FLAGS) Main.o ByteFile.o

clean:
	$(RM) *.a *.o *~ idioman
