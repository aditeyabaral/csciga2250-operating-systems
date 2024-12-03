#include <iostream>
#include <getopt.h>
#include <cstring>
#include <iomanip>
#include <cctype>
#include <deque>
#include <queue>
#include <algorithm>

using namespace std;

// An IO class to store the IO operation information
class IO
{
public:
    int id;          // The ID of the IO operation
    int arrivalTime; // The arrival time of the IO operation
    int startTime;   // The start time of the IO operation
    int endTime;     // The end time of the IO operation
    int track;       // The track number of the IO operation
    bool completed;  // Whether the IO operation has been completed
};

// An IO Scheduler class to implement the IO scheduling algorithms
class IOScheduler
{
public:
    int head = 0;                                                                                  // The current head position
    int direction = 1;                                                                             // The current movement direction
    deque<IO *> ioQueue = deque<IO *>();                                                           // The IO queue to store the IO requests
    void setDirection(int track) { direction = (track >= head) ? ((track == head) ? 0 : 1) : -1; } // Set the movement direction based on the track
    void moveHead(int step = 1) { head += direction * step; }                                      // Move the head based on the direction
    void addIORequest(IO *io) { ioQueue.push_back(io); }                                           // Add an IO request to the IO queue
    virtual IO *getIORequest() = 0;                                                                // Get the next IO request
};

// A First In First Out (FIFO) IO Scheduler class
class FIFO : public IOScheduler
{
public:
    // Get the next IO request from the IO queue
    IO *getIORequest()
    {
        if (!ioQueue.empty()) // Check if the IO queue is not empty
        {
            // Get the next IO request
            IO *io = ioQueue.front();
            ioQueue.pop_front();
            return io;
        }
        else // The IO queue is empty
            return nullptr;
    }
};

// A Shortest Seek Time First (SSTF) IO Scheduler class
class SSTF : public IOScheduler
{
public:
    // Get the next IO request from the IO queue
    IO *getIORequest()
    {
        if (!ioQueue.empty()) // Check if the IO queue is not empty
        {
            // Initialize the closest IO request and distance
            IO *closestIO = nullptr;
            int closestIoIndex, distance, minDistance = INT32_MAX;
            // Find the closest IO request
            for (int i = 0; i < ioQueue.size(); i++)
            {
                // Find the absolute distance between the head and the IO request track
                distance = abs(ioQueue[i]->track - head);
                if (distance < minDistance) // If the distance is less than the minimum distance
                {
                    minDistance = distance; // Update the minimum distance
                    closestIO = ioQueue[i]; // Update the closest IO request
                    closestIoIndex = i;     // Update the index of the closest IO request
                }
            }
            // Remove the closest IO request from the IO queue
            ioQueue.erase(ioQueue.begin() + closestIoIndex);
            // Return the closest IO request
            return closestIO;
        }
        else // The IO queue is empty
            return nullptr;
    }
};

// A global IOScheduler object to represent the IO scheduling algorithm
IOScheduler *scheduler;

// A global queue to process IO operations sequentially
queue<IO *> operations = queue<IO *>();

// A function to read the input file and populate the IO operations
void readInput(FILE *inputFile)
{
    int numOperations = 0;  // The number of IO operations
    static char line[1024]; // A buffer to store the line read from the file

    // Read the IO operations line by line and store them in the IO operations queue
    while (fgets(line, 1024, inputFile) != NULL)
    {
        // Skip lines that begin with a '#'
        if (line[0] == '#')
            continue;
        // Create a new IO operation
        IO *io = new IO();
        // Read the IO operation information
        io->arrivalTime = atoi(strtok(line, " "));
        io->track = atoi(strtok(NULL, " "));
        // Set the ID and completion status of the IO operation
        io->id = numOperations++;
        io->completed = false;
        // Add the IO operation to the IO operations vector
        operations.push(io);
    }
}

// A function to simulate the IO scheduling
void simulate(bool displayExecutionTraceFlag, bool displayIOQueueAndMovementDirectionFlag, bool displayAdditionalQueueInformationFlookFlag)
{
    int currentTime = 0;                   // The current time
    IO *currentIO = nullptr,               // The current IO operation
        *arrivedIO = nullptr;              // The newly arrived IO operation
    int totalMovement = 0;                 // The total head movement
    int maxWaitTime = 0;                   // The maximum wait time
    int numOperations = operations.size(); // The total number of IO operations
    double totalIoBusyTime = 0;            // The total time the IO is busy
    double totalTurnaroundTime = 0;        // The total turnaround time
    double totalWaitTime = 0;              // The total wait time

    while (true)
    {
        // Check if a new IO operation has arrived at the current time
        arrivedIO = operations.front();
        if (!arrivedIO->completed && arrivedIO->arrivalTime == currentTime)
        {
            // Add the IO operation to the IO queue
            scheduler->addIORequest(arrivedIO);
            // Display the arrival of the IO operation
            if (displayExecutionTraceFlag)
                cout << currentTime << ": " << arrivedIO->id << " add " << arrivedIO->track << "\n";
            // Remove the IO operation from the head and add it to the tail
            operations.pop();
            operations.push(arrivedIO);
        }

        // Check if the current IO operation has completed
        if (currentIO != nullptr && currentIO->completed)
        {
            // Set the end time of the IO operation
            currentIO->endTime = currentTime;
            // Set the completion status of the IO operation
            currentIO->completed = true;
            // Add the turnaround time of the IO operation
            totalTurnaroundTime += currentIO->endTime - currentIO->arrivalTime;
            // Display the completion of the IO operation
            if (displayExecutionTraceFlag)
                cout << currentTime << ": " << currentIO->id << " finish " << (currentIO->endTime - currentIO->arrivalTime) << "\n";
            // Set the current IO operation to null
            currentIO = nullptr;
        }

        // Check if no IO operations are active
        if (currentIO == nullptr)
        {
            // Check if there are pending IO operations in the IO queue
            if (!scheduler->ioQueue.empty())
            {
                // Get the next IO operation from the IO queue
                currentIO = scheduler->getIORequest();
                // Calculate the wait time of the IO operation
                int waitTime = currentTime - currentIO->arrivalTime;
                // Update the maximum wait time
                maxWaitTime = (waitTime > maxWaitTime) ? waitTime : maxWaitTime;
                // Add the wait time of the IO operation
                totalWaitTime += waitTime;
                // Display the start of the IO operation
                if (displayExecutionTraceFlag)
                    cout << currentTime << ": " << currentIO->id << " issue " << currentIO->track << " " << scheduler->head << "\n";
                // Set the start time of the IO operation
                currentIO->startTime = currentTime;
                // Set the direction based on the track
                scheduler->setDirection(currentIO->track);
            }
            // Check if there are no pending IO operations to be processed
            else if (operations.front()->completed)
                break;
        }

        // Check if the current IO operation is not null
        if (currentIO != nullptr)
        {
            // Check if the head needs to be moved
            if (scheduler->head != currentIO->track)
            {
                scheduler->moveHead(); // Move the head in the direction
                totalMovement++;       // Increment the total head movement
                totalIoBusyTime++;     // Increment the total IO busy time
            }
            else // The head is already at the track
            {
                // Set the completion status of the IO operation
                currentIO->completed = true;
                continue; // Continue scheduling at the same timestamp
            }
        }
        currentTime++; // Increment the current time
    }

    // Display the summary of the IO scheduling
    while (!operations.empty())
    {
        IO *io = operations.front();
        cout << setw(5) << io->id << ": " << setw(5) << io->arrivalTime << " " << setw(5) << io->startTime << " " << setw(5) << io->endTime << "\n";
        operations.pop();
    }
    cout << "SUM: " << currentTime << " " << totalMovement << " " << fixed << setprecision(4) << totalIoBusyTime / currentTime << " " << setprecision(2) << totalTurnaroundTime / numOperations << " " << totalWaitTime / numOperations << " " << maxWaitTime << "\n";
}

// A function to initialize the IO Scheduler based on the algorithm
void initScheduler(char algo)
{
    switch (algo)
    {
    case 'N':
        scheduler = new FIFO(); // FIFO Algorithm
        break;
    case 'S':
        scheduler = new SSTF(); // SSTF Algorithm
        break;
    // case 'L':
    //     scheduler = new LOOK(); // LOOK Algorithm
    //     break;
    // case 'C':
    //     scheduler = new CLOOK(); // CLOOK Algorithm
    //     break;
    // case 'F':
    //     scheduler = new FLOOK(); // FLOOK Algorithm
    //     break;
    default:
        cout << "Error: Invalid IO scheduler algorithm." << endl;
        exit(1);
    }
}

// Main function
int main(int argc, char *argv[])
{
    int opt;
    char algo = 'N';                                        // The algorithm (default: N [FIFO])
    const char *optstring = "s:vqfh";                       // The options
    bool displayExecutionTraceFlag = false,                 // v
        displayIOQueueAndMovementDirectionFlag = false,     // q
        displayAdditionalQueueInformationFlookFlag = false; // f

    // Parse the command line arguments
    while ((opt = getopt(argc, argv, optstring)) != -1)
    {
        switch (opt)
        {
        case 's': // The algorithm
            algo = *optarg;
            // Initialize the IO scheduler based on the algorithm
            initScheduler(algo);
            break;
        case 'v': // Whether to display the execution trace
            displayExecutionTraceFlag = true;
            break;
        case 'q': // Whether to display the IO queue and movement direction
            displayIOQueueAndMovementDirectionFlag = true;
            break;
        case 'f': // Whether to display additional queue information for FLOOK
            displayAdditionalQueueInformationFlookFlag = true;
            break;
        case 'h': // Show help message
            cout << "Usage: " << argv[0] << " [-s <scheduler>] [-v] [-q] [-f] inputFile" << endl;
            cout << "Options:" << endl;
            cout << "  -s <scheduler>  The scheduler algorithm (N, S, L, C, F)" << endl;
            cout << "  -v              Display the execution trace" << endl;
            cout << "  -q              Display the IO queue and movement direction" << endl;
            cout << "  -f              Display additional queue information for FLOOK" << endl;
            exit(0);
        default: // Invalid option
            cout << "Usage: " << argv[0] << " [-s <scheduler>] [-v] [-q] [-f] [-h] inputFile" << endl;
            exit(1);
        }
    }

    // File pointer for the input file
    FILE *inputFile;
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
    }
    else // No input file specified
    {
        cout << "Error: No input file specified." << endl;
        exit(1);
    }

    // Read the input file and populate the processes
    readInput(inputFile);

    // Run the event simulation
    simulate(displayExecutionTraceFlag, displayIOQueueAndMovementDirectionFlag, displayAdditionalQueueInformationFlookFlag);

    // Close the input file
    fclose(inputFile);

    return 0;
}
