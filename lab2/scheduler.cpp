#include <iostream>
#include <getopt.h>
#include <iomanip>
#include <fstream>
#include <string>
#include <cstring>
#include <cctype>
#include <cstdlib>
#include <vector>

using namespace std;

// A Process class to store the process information
class Process
{
public:
    int processNumber; // The process number
    int arrivalTime;   // The arrival time of the process
    int cpuTime;       // The CPU time of the process
    int cpuBurst;      // The CPU burst of the process
    int ioBurst;       // The I/O burst of the process
};

// A state enum to store the state of the process
enum State
{
    CREATED, // The process is created
    READY,   // The process is ready
    RUNNING, // The process is running
    BLOCKED, // The process is blocked
};

// An Event class to store the event information
class Event
{
public:
    int arrivalTime;  // The event time
    Process *process; // The process
    State oldState;   // The old state of the process
    State newState;   // The new state of the process
};

// A Scheduler class to store the scheduler information
class Scheduler
{
public:
    int currentTime; // The current time
};

// Variables to store random values and the random index offset
int *randomValues;
int MAX_RANDOM_VALUES_LENGTH;
int randomIndexOffset = 0;

// A function to generate random numbers using the random values and the random index offset
int randomNumberGenerator(int burst)
{
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

// Main function
int main(int argc, char *argv[])
{
    int opt;
    bool showHelp = false, verbose = false, traceEventExecution = false, showEventQueue = false, showPreemption = false;
    const char *optstring = "hvteps:";

    // Initialise a Scheduler object
    Scheduler scheduler;

    // Parse the command line arguments
    while ((opt = getopt(argc, argv, optstring)) != -1)
    {
        cout << (char)opt << endl;
        switch (opt)
        {
        case 'h':
            showHelp = true;
            break;
        case 'v':
            verbose = true;
            break;
        case 't':
            traceEventExecution = true;
            break;
        case 'e':
            showEventQueue = true;
            break;
        case 'p':
            showPreemption = true;
            break;
        case 's':
            // scheduler = initScheduler(optarg);
            cout << "Scheduler: " << optarg << endl;
            break;
        default:
            cout << "Usage: " << argv[0] << " [-h] [-v] [-t] [-e] [-p] [-s <scheduler>] inputFile randFile" << endl;
            exit(1);
        }
    }

    // Show the help message if the -h flag is set
    if (showHelp)
    {
        cout << "Usage: " << argv[0] << " [-h] [-v] [-t] [-e] [-p] [-s <schedspec>] inputfile randfile" << endl;
        cout << "Options:" << endl;
        cout << "  -h        Show help message" << endl;
        cout << "  -v        Verbose mode" << endl;
        cout << "  -t        Trace event execution" << endl;
        cout << "  -e        Print event queue" << endl;
        cout << "  -p        Print preemption" << endl;
        cout << "  -s        Scheduler specification (FLS | R<num> | P<num>[:<maxprio>] | E<num>[:<maxprios>])\n";
        exit(0);
    }

    // File pointers for the input and random files
    FILE *inputFile, *randomFile;
    // Check if the input file has been specified
    if (optind < argc)
    {
        // Open the input file
        cout << "Input file: " << argv[optind] << endl;
        inputFile = fopen(argv[optind++], "r");
        if (inputFile == NULL)
        {
            cout << "Error: Cannot open input file. Use -h for help." << endl;
            exit(1);
        }
        // Check if the random file has been specified
        if (optind < argc)
        {
            cout << "Random file: " << argv[optind] << endl;
            // Open the random file
            randomFile = fopen(argv[optind++], "r");
            if (randomFile == NULL)
            {
                cout << "Error: Cannot open random file. Use -h for help." << endl;
                exit(1);
            }
        }
        else // No random file specified
        {
            cout << "Error: No random file specified. Use -h for help." << endl;
            exit(1);
        }
    }
    else // No input file specified
    {
        cout << "Error: No input file specified. Use -h for help." << endl;
        exit(1);
    }

    return 0;
}