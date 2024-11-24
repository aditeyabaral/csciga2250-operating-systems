#include <iostream>
#include <getopt.h>
#include <iomanip>
#include <fstream>
#include <string>
#include <cstring>
#include <cctype>
#include <cstdlib>
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
    uint32_t FILE_MAPPED : 1;   // The file-mapped bit
};

// A Process class to store the process information
class Process
{
public:
    int processNumber;     // The process number
    int vma;               // The number of virtual memory areas. TODO: Rename this to vmaCount
    vector<VMA> vmas;      // The virtual memory areas
    vector<PTE> pageTable; // The page table for the process
    int unmaps;            // The number of unmaps
    int maps;              // The number of maps
    int ins;               // The number of ins
    int outs;              // The number of outs
    int fins;              // The number of fins
    int fouts;             // The number of fouts
    int zeros;             // The number of zeros
    int segv;              // The number of segv
    int segprot;           // The number of segprot

    // A function to check if the page is in the virtual memory area of the process
    bool checkPageInVMA(int vpage)
    {
        for (VMA vma : vmas)
        {
            if (vpage >= vma.startPage && vpage <= vma.endPage)
                return true;
        }
        return false;
    }

    // A function to get the VMA for the page
    VMA *getVMAForPage(int vpage)
    {
        for (VMA &vma : vmas)
        {
            if (vpage >= vma.startPage && vpage <= vma.endPage)
                return &vma;
        }
        return nullptr;
    }
};

// A Frame class to store the frame table entry information
class Frame
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
    char operation; // The operation (c/r/w/e)
    int num;        // The number associated with the operation. Can be procid or vpage
};

// Variables to store the maximum number of virtual pages and frames
const int MAX_VPAGES = 64;
int MAX_FRAMES;

// A global vector to represent the frame table
vector<Frame> frameTable;

// A global deque to represent the free frames
deque<Frame *> freeFrames = deque<Frame *>();

// A global vector to represent the processes
vector<Process *> processes;

// A Pager base class to implement the page replacement algorithms
class Pager
{
public:
    virtual Frame *selectVictimFrame() = 0; // Select the victim frame to replace
    // A function to allocate a frame from the free list
    Frame *allocateFrameFromFreeList()
    {
        if (!freeFrames.empty())
        {
            Frame *frame = freeFrames.front();
            freeFrames.pop_front();
            return frame;
        }
        else
            return nullptr;
    }

    // A function to add a frame to the free list
    void addFrameToFreeList(Frame *frame)
    {
        freeFrames.push_back(frame);
    }

    // A function to get a frame using the page replacement algorithm
    Frame *getFrame()
    {
        // Try to allocate a frame from the free list
        Frame *frame = allocateFrameFromFreeList();
        if (frame == nullptr)
            // If the free list is empty, select a victim frame using the page replacement algorithm
            frame = selectVictimFrame();
        return frame;
    }
};

// The FIFO Pager class to implement the FIFO page replacement algorithm
class FIFO : public Pager
{
    int index = 0;

public:
    // Select the victim frame to replace
    Frame *selectVictimFrame()
    {
        // Select the frame at the current index
        Frame *frame = &frameTable[index];
        // Move to the next frame
        index = (index + 1) % MAX_FRAMES;
        return frame;
    }
};

// A Clock Pager class to implement the CLOCK page replacement algorithm
class Clock : public Pager
{
    int index = 0;

public:
    Frame *selectVictimFrame()
    {
        // Start from the current index
        int inspectedFrames = 0;
        Frame *frame = &frameTable[index];
        while (processes[frame->processNumber]->pageTable[frame->pageNumber].REFERENCED)
        {
            // Reset the referenced bit if it is set
            processes[frame->processNumber]->pageTable[frame->pageNumber].REFERENCED = 0;
            // Move to the next frame
            index = (index + 1) % MAX_FRAMES;
        }
        return frame;
    }
};

// A global pager object to represent the paging algorithm
Pager *pager;

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
        // Initialize the page table for the process
        process->pageTable = vector<PTE>(MAX_VPAGES);

        // Initialize the process statistics
        process->unmaps = 0;
        process->maps = 0;
        process->ins = 0;
        process->outs = 0;
        process->fins = 0;
        process->fouts = 0;
        process->zeros = 0;
        process->segv = 0;
        process->segprot = 0;

        // Read each virtual memory area information
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

        // Initialize the page table for the process
        for (int j = 0; j < MAX_VPAGES; j++)
        {
            PTE pte = PTE();
            pte.PRESENT = 0;
            pte.MODIFIED = 0;
            pte.REFERENCED = 0;
            pte.PAGEDOUT = 0;
            pte.WRITE_PROTECT = 0;
            pte.FILE_MAPPED = 0;
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

// A function to get the next instruction from the input file
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

// A function to display the process statistics
void displayProcessStatistics(unsigned long instructionCount, unsigned long ctxSwitches, unsigned long processExits, unsigned long long cost)
{
    for (Process *process : processes)
    {
        cout << "PROC[" << process->processNumber << "]: U=" << process->unmaps
             << " M=" << process->maps << " I=" << process->ins
             << " O=" << process->outs << " FI=" << process->fins
             << " FO=" << process->fouts << " Z=" << process->zeros
             << " SV=" << process->segv << " SP=" << process->segprot << endl;
    }
    cout << "TOTALCOST " << instructionCount << " " << ctxSwitches << " " << processExits << " " << cost << " " << sizeof(PTE) << endl;
}

// A function to display the frame table
void displayFrameTable()
{
    cout << "FT:";
    for (Frame &frame : frameTable)
    {
        if (frame.processNumber == -1)
        {
            cout << " *";
        }
        else
        {
            cout << " " << frame.processNumber << ":" << frame.pageNumber;
        }
    }
    cout << endl;
}

// A function to display the page table
void displayPageTable()
{
    for (Process *process : processes)
    {
        cout << "PT[" << process->processNumber << "]: ";
        for (int i = 0; i < MAX_VPAGES; i++)
        {
            PTE pte = process->pageTable[i];
            if (pte.PRESENT)
            {
                cout << i << ":"
                     << (pte.REFERENCED ? "R" : "-")
                     << (pte.MODIFIED ? "M" : "-")
                     << (pte.PAGEDOUT ? "S" : "-") << " ";
            }
            else
            {
                cout << (pte.PAGEDOUT ? "#" : "*") << " ";
            }
        }
        cout << endl;
    }
}

// A function to simulate the virtual memory management
void simulate(FILE *inputFile, bool displayInstructionOutputFlag, bool displayPageTableFlag, bool displayFrameTableFlag, bool displayProcessStatisticsFlag, bool displayCurrentPageTableFlag, bool displayAllPageTablesFlag, bool displayFrameTableAfterInstructionFlag, bool displayAgingFlag)
{
    // Initialize the the current instruction, process and page table entry
    Instruction *instruction = new Instruction();
    Process *currentProcess = nullptr;
    PTE *currentPTE = nullptr;

    // Initialize the instruction count, context switches and process exits
    int vpage;
    unsigned long instructionCount = 0;
    unsigned long ctxSwitches = 0;
    unsigned long processExits = 0;
    unsigned long long cost = 0;
    while (getNextInstruction(inputFile, instruction))
    {
        if (displayInstructionOutputFlag)
            cout << instructionCount << ": ==> " << instruction->operation << " " << instruction->num << endl;

        switch (instruction->operation)
        {
        case 'c': // Context switch to new process
            currentProcess = processes[instruction->num];
            ctxSwitches++;
            // Add the cost for context switches
            cost += 130;
            break;
        case 'e': // Process exit
            for (int i = 0; i < MAX_VPAGES; i++)
            {
                currentPTE = &currentProcess->pageTable[i];
                if (currentPTE->PRESENT)
                {
                    if (displayInstructionOutputFlag)
                        cout << " UNMAP " << currentProcess->processNumber << ":" << i << endl;
                    currentProcess->unmaps++;
                    if (currentPTE->MODIFIED && currentPTE->FILE_MAPPED)
                    {
                        if (displayInstructionOutputFlag)
                            cout << " FOUT" << endl;
                        currentProcess->fouts++;
                    }
                    currentPTE->PRESENT = 0;
                    currentPTE->MODIFIED = 0;
                    currentPTE->REFERENCED = 0;
                    currentPTE->PAGEDOUT = 0;
                    currentPTE->WRITE_PROTECT = 0;
                    currentPTE->FILE_MAPPED = 0;
                }
                Frame *frame = &frameTable[currentPTE->FRAME];
                pager->addFrameToFreeList(frame);
                currentPTE->PRESENT = 0;
            }
            processExits++;
            // Add the cost for process exits
            cost += 1230;
            break;
        default: // Handle read and write operations
            // Add the cost for r/w operations
            cost += 1;
            vpage = instruction->num;
            // Fetch the page table entry for the virtual page
            currentPTE = &currentProcess->pageTable[vpage];
            // Check if the page is not present
            if (!currentPTE->PRESENT) // Page fault
            {
                // Check if the page is in the virtual memory area of the process
                if (!currentProcess->checkPageInVMA(vpage))
                {
                    // Not a valid page, move to the next instruction
                    currentProcess->segv++;
                    // Add the cost for segv
                    cost += 440;
                    if (displayInstructionOutputFlag)
                        cout << " SEGV" << endl;
                    break;
                }

                // Set the file-mapped and write-protected bits for the page table entry
                VMA *vma = currentProcess->getVMAForPage(vpage);
                currentPTE->FILE_MAPPED = vma->fileMapped;
                currentPTE->WRITE_PROTECT = vma->writeProtected;

                // Fetch a new frame for the page
                Frame *newFrame = pager->getFrame();
                // Check if the frame is mapped to a page table entry
                if (newFrame->processNumber != -1)
                {
                    // Identify the process and page table entry for the frame
                    Process *victimProcess = processes[newFrame->processNumber];
                    PTE *victimPTE = &victimProcess->pageTable[newFrame->pageNumber];
                    victimProcess->unmaps++;
                    // Add the cost for unmaps
                    cost += 410;

                    // Display the frame table unmapping
                    if (displayInstructionOutputFlag)
                        cout << " UNMAP " << newFrame->processNumber << ":" << newFrame->pageNumber << endl;

                    // Check if the page is modified and file mapped
                    if (victimPTE->MODIFIED)
                    {
                        // Page out to swap space
                        victimProcess->outs++;
                        // Add the cost for swapping out
                        cost += 2750;
                        if (displayInstructionOutputFlag)
                            cout << " OUT" << endl;
                        victimPTE->PAGEDOUT = 1;
                    }
                    else if (victimPTE->FILE_MAPPED)
                    {
                        // Page out to file
                        victimProcess->fouts++;
                        // Add the cost for fouts
                        cost += 2800;
                        if (displayInstructionOutputFlag)
                            cout << " FOUT" << endl;
                    }

                    // Reset the page table entry
                    victimPTE->PRESENT = 0;
                    victimPTE->MODIFIED = 0;
                    victimPTE->REFERENCED = 0;
                    victimPTE->WRITE_PROTECT = 0;
                    // victimPTE->FILE_MAPPED = 0;
                }

                // Map new frame to current page table entry
                newFrame->processNumber = currentProcess->processNumber;
                newFrame->pageNumber = vpage;

                // Map the page table entry to the new frame
                currentPTE->FRAME = newFrame->frameNumber;
                currentPTE->PRESENT = 1;

                // Check if the the page was never swapped out or file mapped
                if (!currentPTE->PAGEDOUT && !currentPTE->FILE_MAPPED)
                {
                    currentProcess->zeros++;
                    // Add the cost for zeroing
                    cost += 150;
                    if (displayInstructionOutputFlag)
                        cout << " ZERO" << endl;
                }
                // Check if the page was swapped out
                else if (currentPTE->PAGEDOUT)
                {
                    currentProcess->ins++;
                    // Add the cost for swapping in
                    cost += 3200;
                    if (displayInstructionOutputFlag)
                        cout << " IN" << endl;
                }
                // Check if the page was file mapped
                else if (currentPTE->FILE_MAPPED)
                {
                    currentProcess->fins++;
                    // Add the cost for fin
                    cost += 2350;
                    if (displayInstructionOutputFlag)
                        cout << " FIN" << endl;
                }

                // Display the frame table mapping
                currentProcess->maps++;
                // Add the cost for mapping
                cost += 350;
                if (displayInstructionOutputFlag)
                    cout << " MAP " << newFrame->frameNumber << endl;

                // Update the reference and modified bits
                currentPTE->REFERENCED = 1;
                if (instruction->operation == 'w')
                {
                    if (currentPTE->WRITE_PROTECT)
                    {
                        currentProcess->segprot++;
                        // Add the cost for segprot
                        cost += 410;
                        if (displayInstructionOutputFlag)
                            cout << " SEGPROT" << endl;
                    }
                    else
                        currentPTE->MODIFIED = 1;
                }
                break;
            }
        }
        instructionCount++;
    }

    // Display the Page Table
    if (displayPageTableFlag)
        displayPageTable();

    // Display the Frame Table
    if (displayFrameTableFlag)
        displayFrameTable();

    // Display the Process Statistics
    if (displayProcessStatisticsFlag)
        displayProcessStatistics(instructionCount, ctxSwitches, processExits, cost);
}

// A function to initialize the pager based on the algorithm
void initPager(char algo)
{
    switch (algo)
    {
    case 'f':
        pager = new FIFO(); // FIFO
        break;
    case 'c':
        pager = new Clock(); // CLOCK
        break;
    }
}

// Main function
int main(int argc, char *argv[])
{
    int opt;
    int numFrames;
    char algo;

    // TODO: Rename these variables
    bool displayInstructionOutputFlag = false,         // O
        displayPageTableFlag = false,                  // P
        displayFrameTableFlag = false,                 // F
        displayProcessStatsFlag = false,               // S
        displayCurrentPageTableFlag = false,           // x
        displayAllPageTablesFlag = false,              // y
        displayFrameTableAfterInstructionFlag = false, // f
        displayAgingFlag = false;                      // a

    const char *optstring = "f:a:o:";
    // Parse the command line arguments
    while ((opt = getopt(argc, argv, optstring)) != -1)
    {
        switch (opt)
        {
        case 'f': // The number of frames
            MAX_FRAMES = atoi(optarg);
            frameTable = vector<Frame>(MAX_FRAMES);
            // Initialize the frame table. TODO: Move this to a function
            for (int i = 0; i < MAX_FRAMES; i++)
            {
                Frame frame = Frame();
                frame.frameNumber = i;
                frame.processNumber = -1;
                frame.pageNumber = -1;
                frameTable[i] = frame;
            }
            break;
        case 'a': // The algorithm
            algo = *optarg;
            initPager(algo);
            break;
        case 'o': // The options
            for (int i = 0; i < strlen(optarg); i++)
            {
                switch (optarg[i])
                {
                case 'O':
                    displayInstructionOutputFlag = true;
                    break;
                case 'P':
                    displayPageTableFlag = true;
                    break;
                case 'F':
                    displayFrameTableFlag = true;
                    break;
                case 'S':
                    displayProcessStatsFlag = true;
                    break;
                case 'x':
                    displayCurrentPageTableFlag = true;
                    break;
                case 'y':
                    displayAllPageTablesFlag = true;
                    break;
                case 'f':
                    displayFrameTableAfterInstructionFlag = true;
                    break;
                case 'a':
                    displayAgingFlag = true;
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
    simulate(inputFile, displayInstructionOutputFlag, displayPageTableFlag, displayFrameTableFlag, displayProcessStatsFlag, displayCurrentPageTableFlag, displayAllPageTablesFlag, displayFrameTableAfterInstructionFlag, displayAgingFlag);

    // Close the input and random files
    fclose(inputFile);
    fclose(randomFile);

    return 0;
}
