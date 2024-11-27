#include <iostream>
#include <getopt.h>
#include <cstring>
#include <cctype>
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
    uint32_t FRAME : 7;             // The frame number
    uint32_t PRESENT : 1;           // The present bit
    uint32_t MODIFIED : 1;          // The modified bit
    uint32_t REFERENCED : 1;        // The referenced bit
    uint32_t PAGEDOUT : 1;          // The paged-out bit
    uint32_t WRITE_PROTECT : 1;     // The write-protected bit
    uint32_t WRITE_PROTECT_SET : 1; // The write-protected set bit
    uint32_t FILE_MAPPED : 1;       // The file-mapped bit
    uint32_t FILE_MAPPED_SET : 1;   // The file-mapped set bit
};

// A Process class to store the process information
class Process
{
public:
    int processNumber;     // The process number
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

    // A function to fetch the VMA in which the page is present
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
    uint32_t timeOfLastUse; // The time of last use
    uint32_t age;           // The aging bit vector
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
    int index = 0;                           // The current index/hand to select the victim frame
    unsigned long currentTime = 0;           // The current time, set to the instruction count
    virtual Frame *selectVictimFrame() = 0;  // Select the victim frame to replace
    virtual void resetAge(Frame *frame) = 0; // Reset the aging bit vector

    // A function to allocate a frame from the free list
    Frame *allocateFrameFromFreeList()
    {
        // Check if the free list is not empty
        if (!freeFrames.empty())
        {
            // Allocate a frame from the free list
            Frame *frame = freeFrames.front();
            freeFrames.pop_front();
            return frame;
        }
        else // The free list is empty
            return nullptr;
    }

    // A function to add a frame to the free list
    void addFrameToFreeList(Frame *frame)
    {
        // Add the frame to the free list
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
public:
    void resetAge(Frame *frame) {} // No implementation needed

    // Select the victim frame to replace
    Frame *selectVictimFrame()
    {
        // Select the frame at the current index
        Frame *victimFrame = &frameTable[index];
        // Move to the next frame
        index = (index + 1) % MAX_FRAMES;
        // Return the selected frame
        return victimFrame;
    }
};

// Variables to store random values and the random index offset
int *randomValues;
int MAX_RANDOM_VALUES_LENGTH;

// The Random Pager class to implement the Random page replacement algorithm
class Random : public Pager
{
public:
    void resetAge(Frame *frame) {} // No implementation needed

    // Select the victim frame to replace
    Frame *selectVictimFrame()
    {
        // Select a random frame
        int randomIndex = randomNumberGenerator(MAX_FRAMES);
        // Return the selected frame
        return &frameTable[randomIndex];
    }

    // A function to generate random numbers using the random values and the random index offset
    int randomNumberGenerator(int numFrames)
    {
        // Generate a random number using the random values and the random index offset
        int value = randomValues[index] % numFrames;
        // Move to the next random value
        index = (index + 1) % MAX_RANDOM_VALUES_LENGTH;
        return value;
    }
};

// A Clock Pager class to implement the Clock page replacement algorithm
class Clock : public Pager
{
public:
    void resetAge(Frame *frame) {} // No implementation needed

    // Select the victim frame to replace
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
            frame = &frameTable[index];
        }
        // Move to the next frame
        index = (index + 1) % MAX_FRAMES;
        // Return the selected frame
        return frame;
    }
};

// An Enhanced Second Chance Pager class to implement the Enhanced Second Chance page replacement algorithm
class ESC : public Pager
{
    int lastReset = -1; // The instruction number of the last reset
    int interval = 47;  // The number of instructions before resetting the reference bits
public:
    void resetAge(Frame *frame) {} // No implementation needed

    // Select the victim frame to replace
    Frame *selectVictimFrame()
    {
        // Initialise vectors to store the class frames and their indices
        Frame *victimFrame = nullptr;
        vector<Frame *> classFrames = vector<Frame *>(4, nullptr);
        vector<int> classFrameIndices = vector<int>(4, -1);
        // Set the start index and the number of instructions since the last reset
        int startIndex = index,
            instructionsSinceLastReset = currentTime - lastReset;

        do
        {
            Frame *frame = &frameTable[index];
            PTE *pte = &processes[frame->processNumber]->pageTable[frame->pageNumber];
            // Calculate the class index
            int classIndex = 2 * pte->REFERENCED + pte->MODIFIED;

            // Check if the class frame is empty
            if (classFrames[classIndex] == nullptr)
            {
                // Store the frame and its index
                classFrames[classIndex] = frame;
                classFrameIndices[classIndex] = index;
            }

            // Check if the reference bits need to be reset
            if (instructionsSinceLastReset > interval)
            {
                // Reset the reference bits
                pte->REFERENCED = 0;
                // Update the last reset instruction number
                lastReset = currentTime;
            }

            // Move to the next frame
            index = (index + 1) % MAX_FRAMES;
        } while (index != startIndex);

        // Select the victim frame as the first non-empty class frame
        for (int i = 0; i < 4; i++)
        {
            // Check if the class frame is not empty
            if (classFrames[i] != nullptr)
            {
                // Select the victim frame and move to the next frame
                victimFrame = classFrames[i];
                index = (classFrameIndices[i] + 1) % MAX_FRAMES;
                break;
            }
        }
        // Return the selected frame
        return victimFrame;
    }
};

// An Aging Pager class to implement the Aging page replacement algorithm
class Aging : public Pager
{
public:
    // Shift the aging bit vector to the right
    void shiftAgeRight(Frame *frame)
    {
        frame->age = frame->age >> 1;
    }

    // Set the leading bit of the aging bit vector
    void setLeadingBit(Frame *frame)
    {
        frame->age = frame->age | 0x80000000;
    }

    // Reset the aging bit vector
    void resetAge(Frame *frame)
    {
        frame->age = 0x0;
    }

    // Select the victim frame to replace
    Frame *selectVictimFrame()
    {
        // Initialise the variables to store the victim frame and its index
        Frame *victimFrame = nullptr;
        int victimFrameIndex,
            startIndex = index,
            smallestAge = UINT32_MAX;

        do
        {
            Frame *frame = &frameTable[index];
            PTE *pte = &processes[frame->processNumber]->pageTable[frame->pageNumber];

            // Shift the age right
            shiftAgeRight(frame);
            // Check if the page is referenced
            if (pte->REFERENCED)
            {
                // Set the leading bit if the page is referenced
                setLeadingBit(frame);
                // Reset the referenced bit
                pte->REFERENCED = 0;
            }

            // Check if the age is smaller than the smallest age
            if (frame->age < smallestAge)
            {
                // Store the victim frame and its age
                victimFrame = frame;
                smallestAge = frame->age;
                victimFrameIndex = index;
            }
            // TODO: Add option flag
            // cout << frame->frameNumber << ": " << hex << frame->age << dec << endl;

            // Move to the next frame
            index = (index + 1) % MAX_FRAMES;
        } while (index != startIndex);

        // Move to the next frame
        index = (victimFrameIndex + 1) % MAX_FRAMES;
        // Return the selected frame
        return victimFrame;
    }
};

// A Working Set Pager class to implement the Working Set page replacement algorithm
class WorkingSet : public Pager
{
    int tau = 49; // The time interval tau
public:
    void resetAge(Frame *frame) {} // No implementation needed

    // Select the victim frame to replace
    Frame *selectVictimFrame()
    {
        // Initialise the variables to store the victim frame and its index
        Frame *oldestFrame = nullptr;
        int age, oldestTimeLastUsed = INT32_MAX;
        int oldestFrameIndex = -1, startIndex = index;

        do
        {
            Frame *frame = &frameTable[index];
            PTE *pte = &processes[frame->processNumber]->pageTable[frame->pageNumber];

            // Calculate the age of the frame
            age = currentTime - frame->timeOfLastUse;
            // TODO: Add option flag
            // cout << frame->frameNumber << "(" << pte->REFERENCED << " " << frame->processNumber << ":" << frame->pageNumber << " " << frame->timeOfLastUse << " " << age << ") " << endl;

            // Check if the page is referenced
            if (pte->REFERENCED)
            {
                // Reset the referenced bit
                pte->REFERENCED = 0;
                // Record the time of last use
                frame->timeOfLastUse = currentTime;
            }
            else // The page is not referenced
            {
                // Check if the age is greater than tau
                if (age > tau)
                {
                    // Select the frame
                    index = (index + 1) % MAX_FRAMES;
                    // Return the selected frame
                    return frame;
                }
                else
                {
                    // Check if the frame is the oldest
                    if (frame->timeOfLastUse < oldestTimeLastUsed)
                    {
                        // Store the oldest frame
                        oldestTimeLastUsed = frame->timeOfLastUse;
                        oldestFrame = frame;
                        oldestFrameIndex = index;
                    }
                }
            }

            // Move to the next frame
            index = (index + 1) % MAX_FRAMES;
        } while (index != startIndex);

        // If the oldestFrame is null, we have not found a victim frame
        if (oldestFrame == nullptr)
            // Recursively call the selectVictimFrame function to find a victim frame
            return selectVictimFrame();
        else // Victim frame found
        {
            // Move to the next frame
            index = (oldestFrameIndex + 1) % MAX_FRAMES;
            // Return the selected frame
            return oldestFrame;
        }
    }
};

// A global pager object to represent the paging algorithm
Pager *pager;

// A function to read the input file and populate the processes and instructions
void readInput(FILE *inputFile)
{
    static char line[1024];   // A buffer to store the line read from the file
    int numProcesses, numVMA; // Variables to store the number of processes and virtual memory areas

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
        numVMA = atoi(line);
        process->vmas = vector<VMA>();
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
        for (int j = 0; j < numVMA; j++)
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

// A function to read the random values from the random file and populate the randomValues array
void readRandomFile(FILE *randomFile)
{
    static char line[1024];                    // A buffer to store the line read from the file
    if (fgets(line, 1024, randomFile) != NULL) // Read the first line to get the number of random values
    {
        MAX_RANDOM_VALUES_LENGTH = atoi(line);                               // Set the number of random values
        randomValues = (int *)calloc(MAX_RANDOM_VALUES_LENGTH, sizeof(int)); // Allocate memory for the random values
        for (int i = 0; i < MAX_RANDOM_VALUES_LENGTH; i++)
        {
            // Read each random value
            if (fgets(line, 1024, randomFile) != NULL)
                randomValues[i] = atoi(line);
        }
    }
}

// A function to get the next instruction from the input file
bool getNextInstruction(FILE *inputFile, Instruction *instruction)
{
    static char line[1024]; // A buffer to store the line read from the file

    // Skip lines that begin with a '#'
    while (fgets(line, 1024, inputFile) != NULL && line[0] == '#')
        ;

    if (feof(inputFile)) // Check if the end of file has been reached
    {
        // End of file
        instruction = nullptr; // Set the instruction to null
        return false;          // Return false
    }
    else
    {
        // Read the instruction information
        instruction->operation = line[0];               // Read the operation
        instruction->num = atoi(strtok(line + 2, " ")); // Read the number
        return true;                                    // Return true
    }
}

// A function to display the statistics of a single process
void displayProcessStatistics(Process *process)
{
    cout << "PROC[" << process->processNumber << "]: U=" << process->unmaps
         << " M=" << process->maps << " I=" << process->ins
         << " O=" << process->outs << " FI=" << process->fins
         << " FO=" << process->fouts << " Z=" << process->zeros
         << " SV=" << process->segv << " SP=" << process->segprot << endl;
}

// A function to display the statistics of all processes
void displayAllProcessStatistics(unsigned long instructionCount, unsigned long ctxSwitches, unsigned long processExits, unsigned long long cost)
{
    for (Process *process : processes)
        displayProcessStatistics(process);
    cout << "TOTALCOST " << instructionCount << " " << ctxSwitches << " " << processExits << " " << cost << " " << sizeof(PTE) << endl;
}

// A function to display the frame table
void displayFrameTable()
{
    cout << "FT:";
    for (Frame &frame : frameTable)
    {
        if (frame.processNumber == -1)
            cout << " *";
        else
            cout << " " << frame.processNumber << ":" << frame.pageNumber;
    }
    cout << endl;
}

// A function to display the page table of a process
void displayProcessPageTable(Process *process)
{
    cout << "PT[" << process->processNumber << "]:";
    for (int i = 0; i < MAX_VPAGES; i++)
    {
        PTE pte = process->pageTable[i];
        if (pte.PRESENT)
            cout << " " << i << ":"
                 << (pte.REFERENCED ? "R" : "-")
                 << (pte.MODIFIED ? "M" : "-")
                 << (pte.PAGEDOUT ? "S" : "-");
        else
            cout << " " << (pte.PAGEDOUT ? "#" : "*");
    }
    cout << endl;
}

// A function to display the page table of all processes
void displayAllProcessPageTable()
{
    for (Process *process : processes)
        displayProcessPageTable(process);
}

// A function to simulate the virtual memory management
void simulate(FILE *inputFile,
              bool displayInstructionOutcomeFlag,
              bool displayPageTableAfterSimulationFlag,
              bool displayFrameTableAfterSimulationFlag,
              bool displayProcessStatisticsAfterSimulaitonFlag,
              bool displayCurrentPageTableAfterInstructionFlag,
              bool displayAllPageTablesAfterInstructionFlag,
              bool displayFrameTableAfterInstructionFlag,
              bool displayAgingFlag)
{
    // Initialize the the current instruction, process and page table entry
    Instruction *instruction = new Instruction();
    Process *currentProcess = nullptr;
    PTE *currentPTE = nullptr;

    // Initialize the cost, instruction count, context switches and process exits
    int vpage;
    unsigned long long cost = 0;
    unsigned long instructionCount = 0, ctxSwitches = 0, processExits = 0;
    while (getNextInstruction(inputFile, instruction)) // Get the next instruction
    {
        // Display the current instruction
        if (displayInstructionOutcomeFlag)
            cout << instructionCount << ": ==> " << instruction->operation << " " << instruction->num << endl;

        // Set the current time to the instruction count
        pager->currentTime = instructionCount;

        switch (instruction->operation) // Handle the operation
        {
        case 'c':                                         // Context switch to new process
            currentProcess = processes[instruction->num]; // Set the current process to the new process
            ctxSwitches++;                                // Increment the context switches
            cost += 130;                                  // Add the cost for context switches
            break;
        case 'e': // Process exit
            // Display the process exit
            if (displayInstructionOutcomeFlag)
                cout << "EXIT current process " << currentProcess->processNumber << endl;

            // Reset the page table entries for the current process
            for (int i = 0; i < MAX_VPAGES; i++)
            {
                currentPTE = &currentProcess->pageTable[i]; // Fetch the page table entry
                if (currentPTE->PRESENT)                    // Check if the page is present
                {
                    cost += 410; // Add the cost for unmaps
                    // Unmap the page and display the outcome
                    if (displayInstructionOutcomeFlag)
                        cout << " UNMAP " << currentProcess->processNumber << ":" << i << endl;
                    currentProcess->unmaps++; // Increment the unmaps count

                    // If a page is modified and file mapped, write it to the file
                    if (currentPTE->MODIFIED && currentPTE->FILE_MAPPED)
                    {
                        if (displayInstructionOutcomeFlag) // Display the outcome
                            cout << " FOUT" << endl;
                        currentProcess->fouts++; // Increment the fouts count
                        cost += 2800;            // Add the cost for fouts
                    }

                    // Release the frame and add it to the free list
                    Frame *frame = &frameTable[currentPTE->FRAME];
                    frame->processNumber = -1;
                    frame->pageNumber = -1;
                    pager->addFrameToFreeList(frame);
                }
                // Reset the page table entry
                currentPTE->PRESENT = 0;
                currentPTE->MODIFIED = 0;
                currentPTE->REFERENCED = 0;
                currentPTE->PAGEDOUT = 0;
                currentPTE->WRITE_PROTECT = 0;
                // currentPTE->WRITE_PROTECT_SET = 0; // TODO: Check if this is needed
                currentPTE->FILE_MAPPED = 0;
                // currentPTE->FILE_MAPPED_SET = 0; //TODO: Check if this is needed
            }
            processExits++; // Increment the process exits count
            cost += 1230;   // Add the cost for process exits
            break;
        default:       // Handle read and write operations
            cost += 1; // Add the cost for r/w operations
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
                    currentProcess->segv++;            // Increment the segv count
                    cost += 440;                       // Add the cost for segv
                    if (displayInstructionOutcomeFlag) // Display the outcome
                        cout << " SEGV" << endl;
                    break;
                }

                // Set the file-mapped and write-protected bits for the page table entry.
                // TODO: Check if a loop can be done only once
                VMA *vma = currentProcess->getVMAForPage(vpage); // Fetch the VMA for the page
                // Set the file-mapped and write-protected bits
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
                    victimProcess->unmaps++; // Increment the unmaps count
                    cost += 410;             // Add the cost for unmaps

                    // Display the frame table unmapping
                    if (displayInstructionOutcomeFlag)
                        cout << " UNMAP " << newFrame->processNumber << ":" << newFrame->pageNumber << endl;

                    // Check if the page is modified
                    if (victimPTE->MODIFIED)
                    {
                        if (victimPTE->FILE_MAPPED) // Check if the page is file mapped
                        {
                            // Page out to file
                            victimProcess->fouts++;            // Increment the fouts count
                            cost += 2800;                      // Add the cost for fouts
                            if (displayInstructionOutcomeFlag) // Display the outcome
                                cout << " FOUT" << endl;
                        }
                        else
                        {
                            // Page out to swap space
                            victimProcess->outs++;             // Increment the outs count
                            cost += 2750;                      // Add the cost for swapping out
                            if (displayInstructionOutcomeFlag) // Display the outcome
                                cout << " OUT" << endl;
                            victimPTE->PAGEDOUT = 1; // Set the paged-out bit
                        }
                    }

                    // Reset the page table entry
                    victimPTE->PRESENT = 0;
                    victimPTE->MODIFIED = 0;
                    victimPTE->REFERENCED = 0;
                    // victimPTE->WRITE_PROTECT = 0; // TODO: Check this
                    // victimPTE->FILE_MAPPED = 0; // TODO: Check this
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
                    currentProcess->zeros++;           // Increment the zeros count
                    cost += 150;                       // Add the cost for zeroing
                    if (displayInstructionOutcomeFlag) // Display the outcome
                        cout << " ZERO" << endl;
                }
                else if (currentPTE->PAGEDOUT) // Check if the page was swapped out
                {
                    currentProcess->ins++;             // Increment the ins count
                    cost += 3200;                      // Add the cost for swapping in
                    if (displayInstructionOutcomeFlag) // Display the outcome
                        cout << " IN" << endl;
                }
                else if (currentPTE->FILE_MAPPED) // Check if the page was file mapped
                {
                    currentProcess->fins++;            // Increment the fins count
                    cost += 2350;                      // Add the cost for fin
                    if (displayInstructionOutcomeFlag) // Display the outcome
                        cout << " FIN" << endl;
                }

                currentProcess->maps++; // Increment the maps count
                // Set the time of last use for the frame
                newFrame->timeOfLastUse = pager->currentTime;
                pager->resetAge(newFrame);         // Reset the aging bit vector
                cost += 350;                       // Add the cost for mapping
                if (displayInstructionOutcomeFlag) // Display the outcome
                    cout << " MAP " << newFrame->frameNumber << endl;
            }

            // Update the reference bit
            currentPTE->REFERENCED = 1;
            // Check if the operation is write
            if (instruction->operation == 'w')
            {
                if (currentPTE->WRITE_PROTECT) // Check if the page is write-protected
                {
                    currentProcess->segprot++;         // Increment the segprot count
                    cost += 410;                       // Add the cost for segprot
                    if (displayInstructionOutcomeFlag) // Display the outcome
                        cout << " SEGPROT" << endl;
                }
                else
                    currentPTE->MODIFIED = 1; // Set the modified bit
            }
            break;
        }
        // Increment the instruction count
        instructionCount++;
    }

    // Display the Page Table
    if (displayPageTableAfterSimulationFlag)
        displayAllProcessPageTable();

    // Display the Frame Table
    if (displayFrameTableAfterSimulationFlag)
        displayFrameTable();

    // Display the Process Statistics
    if (displayProcessStatisticsAfterSimulaitonFlag)
        displayAllProcessStatistics(instructionCount, ctxSwitches, processExits, cost);
}

// A function to initialize the pager based on the algorithm
void initPager(char algo)
{
    switch (algo)
    {
    case 'f':
        pager = new FIFO(); // FIFO Algorithm
        break;
    case 'c':
        pager = new Clock(); // Clock Algorithm
        break;
    case 'r':
        pager = new Random(); // Random Algorithm
        break;
    case 'e':
        pager = new ESC(); // Enhanced Second Chance Algorithm
        break;
    case 'a':
        pager = new Aging(); // Aging Algorithm
        break;
    case 'w':
        pager = new WorkingSet(); // Working Set Algorithm
        break;
    }
}

// Main function
int main(int argc, char *argv[])
{
    int opt;
    char algo;                                               // The algorithm
    const char *optstring = "f:a:o:";                        // The options
    bool displayInstructionOutcomeFlag = false,              // O
        displayPageTableAfterSimulationFlag = false,         // P
        displayFrameTableAfterSimulationFlag = false,        // F
        displayProcessStatisticsAfterSimulaitonFlag = false, // S
        displayCurrentPageTableAfterInstructionFlag = false, // x
        displayAllPageTablesAfterInstructionFlag = false,    // y
        displayFrameTableAfterInstructionFlag = false,       // f
        displayAgingFlag = false;                            // a

    // Parse the command line arguments
    while ((opt = getopt(argc, argv, optstring)) != -1)
    {
        switch (opt)
        {
        case 'f': // The number of frames
            MAX_FRAMES = atoi(optarg);
            // Initialize the frame table and the free frames
            frameTable = vector<Frame>(MAX_FRAMES);
            for (int i = 0; i < MAX_FRAMES; i++)
            {
                Frame frame = Frame();
                frame.frameNumber = i;
                frame.processNumber = -1;
                frame.pageNumber = -1;
                frame.timeOfLastUse = -1;
                frame.age = 0x0;
                frameTable[i] = frame;
                freeFrames.push_back(&frameTable[i]);
            }
            break;
        case 'a': // The algorithm
            algo = *optarg;
            // Initialize the pager based on the algorithm
            initPager(algo);
            break;
        case 'o': // The options
            for (int i = 0; i < strlen(optarg); i++)
            {
                switch (optarg[i]) // Set the display flags based on the options
                {
                case 'O':
                    displayInstructionOutcomeFlag = true;
                    break;
                case 'P':
                    displayPageTableAfterSimulationFlag = true;
                    break;
                case 'F':
                    displayFrameTableAfterSimulationFlag = true;
                    break;
                case 'S':
                    displayProcessStatisticsAfterSimulaitonFlag = true;
                    break;
                case 'x':
                    displayCurrentPageTableAfterInstructionFlag = true;
                    break;
                case 'y':
                    displayAllPageTablesAfterInstructionFlag = true;
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

    // Read the input file and populate the processes
    readInput(inputFile);

    // Read the random values from the random file
    readRandomFile(randomFile);

    // Run the event simulation
    simulate(inputFile, displayInstructionOutcomeFlag, displayPageTableAfterSimulationFlag, displayFrameTableAfterSimulationFlag, displayProcessStatisticsAfterSimulaitonFlag, displayCurrentPageTableAfterInstructionFlag, displayAllPageTablesAfterInstructionFlag, displayFrameTableAfterInstructionFlag, displayAgingFlag);

    // Close the input and random files
    fclose(inputFile);
    fclose(randomFile);

    return 0;
}
