/* ========================================================================== */
/*                                                                            */
/*   clapba.cpp                                                               */
/*   (c) 2016 Ondrej Krpec, xkrpec01@stud.fit.vutbr.cz                        */
/*                                                                            */
/*   Implementation of Carry Look Ahead Parallel Binary Adder                 */
/*   as second project for subject PRL at FIT VUT Brno.                       */
/*                                                                            */
/* ========================================================================== */

#include <iostream>
#include <sys/time.h>
#include <sys/types.h>
#include <cmath>
#include <fstream>
#include <vector>
#include<mpi.h>
#include<string>

using namespace std;

#define filename "numbers"

enum algorithmState {
    Generate = 0,
    Stop = 1,
    Propagate = 2
};


/**
 * Function reads data from input file defined by constant `filename` and returns them as a string.
 * @return Returns data from input file.
 */
string getInputAsString();

/**
 * Function transforms first line of data from string to vector of integers. Functions expects string to contain only binary numbers and newline character.
 * Any other character is ignored.
 * @param input Input string.
 * @return Returns first line of input data as vector of integers.
 */
vector<int> getFirstNum(string input);

/**
 * Returns second line of data. @see getFirstNum.
 */
vector<int> getSecondNum(string input);

/**
 * Function converts character to integer. Expects only '0' and '1' characters. For any other characters returns '1';
 * @param c Input character
 * @return Input character converted to int.
 */
int charToBinary(char c);

/**
 * Function adds to input numbers in binary and prints them out to stdin.
 * @param mpiSize Count of given cpus.
 * @param mpiRank Id of given cpu.
 * @param firstNum First number that will be used for computation.
 * @param secondNum Second number that will be used for computation.
 */
void parallelBinaryAdder(int mpiSize, int mpiRank, vector<int> firstNum, vector<int> secondNum);

/**
 * Functions prints out input bit and cpu rank in Standart format 'cpuRank:bit'. If input bit overflows, overflow is printed out.
 * @param bit Input bit
 * @param mpiRank Cpu rank
 * @param index Index of last cpu.
 */
void printOutput(int bit, int mpiRank, int index);


int main(int argc, char** argv) {
    int mpiSize, mpiRank;     
    string input = getInputAsString();
    
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);
    MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
    
    parallelBinaryAdder(mpiSize, mpiRank, getFirstNum(input), getSecondNum(input));
    
    MPI_Finalize();

}


void parallelBinaryAdder(int mpiSize, int mpiRank, vector<int> firstNum, vector<int> secondNum) {

    // FIXME tohle po hokeji presunout do fce
    if (firstNum.size() > secondNum.size()) {
        vector<int> tmp;
        for (int i = 0; i < firstNum.size() - secondNum.size(); i++) {
            tmp.push_back(0);
        }
        
        for (int i = 0; i < secondNum.size(); i++) {
            tmp.push_back(secondNum[i]);
        }
        
        secondNum = tmp;
    } else if (firstNum.size() < secondNum.size()) {
        vector<int> tmp;
        for (int i = 0; i < secondNum.size() - firstNum.size(); i++) {
            tmp.push_back(0);
        }
        
        for (int i = 0; i < firstNum.size(); i++) {
            tmp.push_back(firstNum[i]);
        }
        
        firstNum = tmp;
    }

    MPI_Status status;

    int node = (mpiSize + 1) / 2;
    int maxNode = mpiSize - 1; 
    int lastNode = maxNode - node;
    
    int stop = log2(node);
    int currFstNum, currSecNum;
    
    algorithmState value;
    int leftNode, rightNode;
    
    if (mpiRank == 0) {
        for (int i = 0; i < node; i++) {
            MPI_Send(&firstNum[node - i - 1], 1, MPI_INT, maxNode - i, 0, MPI_COMM_WORLD);
            MPI_Send(&secondNum[node - i - 1], 1, MPI_INT, maxNode - i, 0, MPI_COMM_WORLD);
        }
    } else if (mpiRank > lastNode) {
        MPI_Recv(&currFstNum, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
        MPI_Recv(&currSecNum, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);     
    }
    
    value = (algorithmState)(currFstNum + currSecNum);
    
    for (int i = 0; i < stop; i++) {
        if (mpiRank <= lastNode) {
            MPI_Recv(&leftNode, 1, MPI_INT, (mpiRank * 2) + 1, 0, MPI_COMM_WORLD, &status);
            MPI_Recv(&rightNode, 1, MPI_INT, (mpiRank * 2) + 2, 0, MPI_COMM_WORLD, &status);
            
            value = (leftNode == 1) ? (algorithmState)rightNode : (algorithmState)leftNode;
        }
        
        if (mpiRank != 0) {
            MPI_Send(&value, 1, MPI_INT, (mpiRank - 1) / 2, 0, MPI_COMM_WORLD);
        }
    }
    
    if (mpiRank == 0) {
        value = Stop;
    }
    
    for (int i = 0; i < stop; i++) {
        if (mpiRank != 0) {
            MPI_Recv(&value, 1, MPI_INT, (mpiRank - 1) / 2, 0, MPI_COMM_WORLD, &status);
        }
        
        if (mpiRank <= lastNode) {
            int tmpLeftNode = (rightNode == 1) ? value : rightNode;
            int tmpRightNode = value;
            
            MPI_Send(&tmpLeftNode, 1, MPI_INT, mpiRank * 2 + 1, 0, MPI_COMM_WORLD);
            MPI_Send(&tmpRightNode, 1, MPI_INT, mpiRank * 2 + 2, 0, MPI_COMM_WORLD);
        }
    }
    
    printOutput(currFstNum + currSecNum + (value == Propagate ? 1 : 0), mpiRank, lastNode);
}


string getInputAsString() {
    fstream stream;
    string input;
    
    stream.open(filename, ios::in);
    while (stream.good()) {
        input.push_back(stream.get());         
    }
    
    stream.close();
    
    return input;
}


vector<int> getFirstNum(string input) {
    vector<int> num;

    for (int i = 0; i < input.length(); i++) {
        if (input[i] == '\n') {
            return num;
        }
        
        num.push_back(charToBinary(input[i]));      
    }
    
    return num;
}


vector<int> getSecondNum(string input) {
    vector<int> num;
    bool skip = true;
    
    for (int i = 0; i < input.length(); i++) {
        if (skip && input[i] != '\n') {
            continue;   
        } else if (skip && input[i] == '\n') {
            skip = false;
        }
        
        if (input[i] == '0' || input[i] == '1') {
            num.push_back(charToBinary(input[i]));
        }
    }
    
    return num;
}


int charToBinary(char c) {
    return c == '0' ? 0 : 1;
}


void printOutput(int bit, int mpiRank, int index) {
    if (bit > 1) {
        bit = 0;
        
        if (mpiRank == index + 1) {
            cout << "overflow\n";
        }
    }
    
    if (mpiRank > index) {
        cout << mpiRank << ":" << bit << "\n";
    }
}
