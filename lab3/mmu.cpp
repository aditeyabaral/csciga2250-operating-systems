#include <iostream>
#include <getopt.h>
#include <iomanip>
#include <fstream>
#include <string>
#include <cstring>
#include <cctype>
#include <cstdlib>
#include <queue>
#include <deque>
#include <algorithm>

using namespace std;

// A VMA class to store the virtual memory area information
class VMA
{
public:
    int startPage;       // The start page of the virtual memory area
    int endPage;         // The end page of the virtual memory area
    bool writeProtected; // The write protection status of the virtual memory area
    bool fileMapped;     // The file mapping status of the virtual memory area
};

// A Page Table Entry (PTE) class to store the page table entry information
class PTE
{
public:
    uint32_t FRAME : 7;         // The frame number
    uint32_t PRESENT : 1;       // The present bit
    uint32_t MODIFIED : 1;      // The modified bit
    uint32_t REFERENCED : 1;    // The referenced bit
    uint32_t PAGEDOUT : 1;      // The paged-out bit
    uint32_t WRITE_PROTECT : 1; // The write-protected bit
};

// A Process class to store the process information
class Process
{
public:
    int processNumber;     // The process number
    int vma;               // The number of virtual memory areas. TODO: Rename this to vmaCount
    vector<VMA> vmas;      // The virtual memory areas
    vector<PTE> pageTable; // The page table for the process
};

// A Frame Table Entry (FTE) class to store the frame table entry information
class FTE
{
public:
    uint32_t frameNumber;   // The frame number
    uint32_t processNumber; // The process number
    uint32_t pageNumber;    // The page number
};

// An Instruction class to store the instruction information
class Instruction
{
public:
    int id;         // The instruction number
    char operation; // The operation (c/r/w/e)
    int num;        // The number associated with the operation. Can be procid or vpage
};

// A Pager base class to implement the page replacement algorithms
class Pager
{
public:
    virtual FTE *selectVictimFrame() = 0; // Select the victim frame to replace
    FTE *allocateFrameFromFreeList()
    {
        for (FTE &frame : frameTable)
        {
            if (frame.processNumber == -1)
            {
                return &frame;
            }
        }
        return nullptr;
    }

    FTE *getFrame()
    {
        FTE *frame = allocateFrameFromFreeList();
        if (frame == nullptr)
            frame = selectVictimFrame();
        return frame;
    }
};

// Variables to store the maximum number of virtual pages and frames
const int MAX_VPAGES = 64;
int MAX_FRAMES;

// A global vector to represent the frame table
vector<FTE> frameTable;

// A global vector to represent the processes
vector<Process *> processes;

// The FIFO Pager class to implement the FIFO page replacement algorithm
class FIFO : public Pager
{
    int index = 0;

public:
    FTE *selectVictimFrame()
    {
        FTE *frame = &frameTable[index];
        index = (index + 1) % MAX_FRAMES;
        return frame;
    }
};

// A function to read the input file and populate the processes and instructions
void readInput(FILE *inputFile)
{
    static char line[1024];
    int numProcesses = 0;

    // Skip lines that begin with a '#'
    while (fgets(line, 1024, inputFile) != NULL && line[0] == '#')
        ;

    // Read the total number of processes
    numProcesses = atoi(line);

    // Read each process information
    for (int i = 0; i < numProcesses; i++)
    {
        // Skip lines that begin with a '#'
        while (fgets(line, 1024, inputFile) != NULL && line[0] == '#')
            ;

        // Create a new process
        Process *process = new Process();
        process->processNumber = i;
        // Read the number of virtual memory areas
        process->vma = atoi(line);

        for (int j = 0; j < process->vma; j++)
        {
            // Create a new virtual memory area
            VMA vma = VMA();
            // Read the virtual memory area information
            fgets(line, 1024, inputFile);
            vma.startPage = atoi(strtok(line, " "));
            vma.endPage = atoi(strtok(NULL, " "));
            vma.writeProtected = atoi(strtok(NULL, " "));
            vma.fileMapped = atoi(strtok(NULL, " "));
            // Add the virtual memory area to the process
            process->vmas.push_back(vma);
        }

        // Create the page table for the process. TODO: Use constant for 64
        process->pageTable = vector<PTE>(MAX_VPAGES);
        for (int j = 0; j < process->pageTable.size(); j++)
        {
            PTE pte = PTE();
            pte.PRESENT = 0;
            pte.MODIFIED = 0;
            pte.REFERENCED = 0;
            pte.PAGEDOUT = 0;
            pte.WRITE_PROTECT = 0;
            process->pageTable[j] = pte;
        }

        // Add the process to the processes vector
        processes.push_back(process);
    }
}

// Variables to store random values and the random index offset
int *randomValues;
int MAX_RANDOM_VALUES_LENGTH;
int randomIndexOffset = 0;

// A function to generate random numbers using the random values and the random index offset
int randomNumberGenerator(int burst)
{
    // TODO: Check indexing
    int value = 1 + (randomValues[randomIndexOffset] % burst);
    randomIndexOffset = (randomIndexOffset + 1) % MAX_RANDOM_VALUES_LENGTH;
    return value;
}

// A function to read the random values from the random file and populate the randomValues array
void readRandomFile(FILE *randomFile)
{
    // Read the random values from the random file. The first line is the number of random values and the rest are the random values
    static char line[1024];
    if (fgets(line, 1024, randomFile) != NULL)
    {
        MAX_RANDOM_VALUES_LENGTH = atoi(line);
        randomValues = (int *)calloc(MAX_RANDOM_VALUES_LENGTH, sizeof(int));
        for (int i = 0; i < MAX_RANDOM_VALUES_LENGTH; i++)
        {
            if (fgets(line, 1024, randomFile) != NULL)
                randomValues[i] = atoi(line);
        }
    }
}

// A function to display the process information
void displayProcessInfo()
{
    for (Process *process : processes)
    {
        cout << "Process " << process->processNumber << ": " << process->vma << " VMA" << endl;
        for (VMA vma : process->vmas)
            cout << "  VMA " << vma.startPage << " " << vma.endPage << " " << vma.writeProtected << " " << vma.fileMapped << endl;
    }
}

bool getNextInstruction(FILE *inputFile, Instruction *instruction)
{
    static char line[1024];

    // Skip lines that begin with a '#'
    while (fgets(line, 1024, inputFile) != NULL && line[0] == '#')
        ;

    if (feof(inputFile))
    {
        // End of file
        instruction = nullptr;
        return false;
    }
    else
    {
        // Read the instruction information
        instruction->operation = line[0];
        instruction->num = atoi(strtok(line + 2, " "));
        return true;
    }
}

// A function to check if the page is in the virtual memory area of a process
bool checkPageInVMA(Process *process, int vpage)
{
    for (VMA vma : process->vmas)
    {
        if (vpage >= vma.startPage && vpage <= vma.endPage)
            return true;
    }
    return false;
}

// A function to simulate the virtual memory management
void simulate(FILE *inputFile, bool displayOutput, bool displayPageTable, bool displayFrameTable, bool displayProcessStats, bool displayCurrentPageTable, bool displayAllPageTables, bool displayFrameTableAfterInstruction, bool displayAging)
{
    // Initialize the the current instruction, process and page table entry
    Instruction *instruction = new Instruction();
    Process *currentProcess = nullptr;
    PTE *currentPTE = nullptr;

    // Initialize the instruction count, context switches and process exits
    int vpage;
    unsigned long long instructionCount = 0;
    unsigned long long ctxSwitches = 0;
    unsigned long long processExits = 0;
    while (getNextInstruction(inputFile, instruction))
    {
        cout << instructionCount << ": ==> " << instruction->operation << " " << instruction->num << endl;
        switch (instruction->operation)
        {
        case 'c':
            // Context switch to new process
            currentProcess = processes[instruction->num];
            ctxSwitches++;
            break;
        case 'e':
            // Process exits
            // TODO: Implement the exit operation
            processExits++;
            break;
        default:
            vpage = instruction->num;
            // Fetch the page table entry for the virtual page
            currentPTE = &currentProcess->pageTable[vpage];
            // Check if the page is not present
            if (!currentPTE->PRESENT) // Page fault
            {
                // Check if the page is in the virtual memory area of the process
                bool isPageInVMA = checkPageInVMA(currentProcess, vpage);
                if (!isPageInVMA)
                {
                    cout << " SEGV" << endl;
                }
                else
                {
                    ;
                }
                break;
            }
            instructionCount++;
        }
    }
}

// Main function
int main(int argc, char *argv[])
{
    int opt;
    int numFrames;
    char algo;

    // TODO: Rename these variables
    bool displayOutput = false,                    // O
        displayPageTable = false,                  // P
        displayFrameTable = false,                 // F
        displayProcessStats = false,               // S
        displayCurrentPageTable = false,           // x
        displayAllPageTables = false,              // y
        displayFrameTableAfterInstruction = false, // f
        displayAging = false;                      // a

    const char *optstring = "f:a:o:";
    // Parse the command line arguments
    while ((opt = getopt(argc, argv, optstring)) != -1)
    {
        switch (opt)
        {
        case 'f': // The number of frames
            MAX_FRAMES = atoi(optarg);
            break;
        case 'a': // The algorithm
            algo = *optarg;
            // TODO: Directly initialize the algorithm
            break;
        case 'o': // The options
            for (int i = 0; i < strlen(optarg); i++)
            {
                switch (optarg[i])
                {
                case 'O':
                    displayOutput = true;
                    break;
                case 'P':
                    displayPageTable = true;
                    break;
                case 'F':
                    displayFrameTable = true;
                    break;
                case 'S':
                    displayProcessStats = true;
                    break;
                case 'x':
                    displayCurrentPageTable = true;
                    break;
                case 'y':
                    displayAllPageTables = true;
                    break;
                case 'f':
                    displayFrameTableAfterInstruction = true;
                    break;
                case 'a':
                    displayAging = true;
                    break;
                }
            }
            break;
        }
    }

    // File pointers for the input and random files
    FILE *inputFile, *randomFile;
    // Check if the input file has been specified
    if (optind < argc)
    {
        // Open the input file
        inputFile = fopen(argv[optind++], "r");
        if (inputFile == NULL) // Check if the input file can be opened
        {
            cout << "Error: Cannot open input file." << endl;
            exit(1);
        }
        // Check if the random file has been specified
        if (optind < argc)
        {
            // Open the random file
            randomFile = fopen(argv[optind++], "r");
            if (randomFile == NULL) // Check if the random file can be opened
            {
                cout << "Error: Cannot open random file." << endl;
                exit(1);
            }
        }
        else // No random file specified
        {
            cout << "Error: No random file specified." << endl;
            exit(1);
        }
    }
    else // No input file specified
    {
        cout << "Error: No input file specified." << endl;
        exit(1);
    }

    // Read the input file and populate the processes and instructions
    readInput(inputFile);

    // Read the random values from the random file
    readRandomFile(randomFile);

    // Run the event simulation
    simulate(inputFile, displayOutput, displayPageTable, displayFrameTable, displayProcessStats, displayCurrentPageTable, displayAllPageTables, displayFrameTableAfterInstruction, displayAging);

    // Close the input and random files
    fclose(inputFile);
    fclose(randomFile);

    return 0;
}
