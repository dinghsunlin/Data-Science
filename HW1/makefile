CPP		=g++

SRC		=./111062684_hw1.cpp

RM		=rm

EXE		=./111062684_hw1

all :: opt
opt:
	$(CPP) -std=c++2a -pthread -fopenmp -O2 -o $(EXE) $(SRC)
out1:
	$(EXE) 0.2 input1.txt output1.txt
out2:
	$(EXE) 0.6 input2.txt output2.txt
out3:
	$(EXE) 0.25 input3.txt output3.txt
out4:
	$(EXE) 0.2 input4.txt output4.txt
clean:
	$(RM) -f $(EXE)