CC=gcc
CXX=g++
COMMON_FLAGS=-m32 -g2 -fstack-protector-all -O2
IDIOMAN_FLAGS=$(COMMON_FLAGS) -Ifmt/include -DFMT_HEADER_ONLY

all: idioman

Main.o: Main.cpp Analyzer.h ByteFile.h Error.h
	$(CXX) -o $@ $(IDIOMAN_FLAGS) -c Main.cpp

ByteFile.o: ByteFile.cpp ByteFile.h Error.h
	$(CXX) -o $@ $(IDIOMAN_FLAGS) -c ByteFile.cpp

Analyzer.o: Analyzer.cpp Analyzer.h Inst.h ByteFile.h Error.h
	$(CXX) -o $@ $(IDIOMAN_FLAGS) -c Analyzer.cpp

idioman: Main.o ByteFile.o Analyzer.o
	$(CXX) -o $@ $(IDIOMAN_FLAGS) Main.o ByteFile.o Analyzer.o

clean:
	$(RM) *.a *.o *~ idioman
