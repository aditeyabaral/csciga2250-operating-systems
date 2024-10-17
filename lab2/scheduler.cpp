#include <iostream>
#include <getopt.h>
#include <iomanip>
#include <fstream>
#include <string>
#include <cstring>
#include <cctype>
#include <cstdlib>
#include <queue>

using namespace std;

// A Process class to store the process information
class Process
{
public:
    int processNumber;   // The process number
    int arrivalTime;     // The arrival time of the process
    int cpuTime;         // The CPU time of the process
    int cpuBurst;        // The defined CPU burst of the process
    int ioBurst;         // The defined I/O burst of the process
    int staticPriority;  // The static priority of the process
    int dynamicPriority; // The dynamic priority of the process
    int stateTimeStamp;  // The time stamp when the process changed state
};

// A state enum to store the state of the process
enum State
{
    CREATED, // The process is created
    READY,   // The process is ready
    RUNNING, // The process is running
    BLOCKED, // The process is blocked
};

// A transition enum to store the transition of the process
enum Transition
{
    TO_READY,   // The process transitions to the ready state
    TO_RUNNING, // The process transitions to the running state
    TO_BLOCKED, // The process transitions to the blocked state
    TO_PREEMPT, // The process is to be preempted
};

// An Event class to store the event information
class Event
{
public:
    int timeStamp;         // The event time
    Process *process;      // The process
    State oldState;        // The old state of the process
    State newState;        // The new state of the process
    Transition transition; // The transition of the process from the old state to the new state
};

// A Scheduler class to store the scheduler information
class Scheduler
{
public:
    int quantum = 10000;                           // The quantum of the scheduler. Default value is 10K
    int maxprios = 4;                              // The maximum number of priorities. Default value is 4
    queue<Process *> readyQueue;                   // The ready queue
    virtual void addProcess(Process *process) = 0; // A function to add a process to the ready queue
    virtual Process *getNextProcess() = 0;         // A function to get the next process from the ready queue
};

// A First Come First Serve (FCFS) Scheduler class
class FCFS : public Scheduler
{
public:
    void addProcess(Process *process)
    {
        readyQueue.push(process);
    }

    Process *getNextProcess()
    {
        if (readyQueue.empty())
            return NULL;
        Process *process = readyQueue.front();
        readyQueue.pop();
        return process;
    }
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

// A queue to store the events
queue<Event *> eventQueue;

// A function to read the input file and populate the eventQueue
void readInputFile(FILE *inputFile, int maxprios = 4)
{
    int processNumber = 0;
    // Read the input file and populate the eventQueue
    static char line[1024];
    while (fgets(line, 1024, inputFile) != NULL)
    {
        // Create a process and populate the process information
        Process *process = new Process();
        process->processNumber = processNumber++;
        process->arrivalTime = atoi(strtok(line, " "));
        process->cpuTime = atoi(strtok(NULL, " "));
        process->cpuBurst = atoi(strtok(NULL, " "));
        process->ioBurst = atoi(strtok(NULL, " "));
        process->staticPriority = randomNumberGenerator(maxprios);
        process->stateTimeStamp = 0; // TODO: What is the time stamp?

        // Create an event for the process and push it to the eventQueue
        Event *event = new Event();
        event->timeStamp = 0; // TODO: What is the time stamp?
        event->process = process;
        event->oldState = CREATED;
        event->newState = READY;
        event->transition = TO_READY;
        eventQueue.push(event);
    }
}

// A function to return the head of the eventQueue
Event *getEvent()
{
    if (eventQueue.empty())
        return NULL;
    Event *event = eventQueue.front();
    eventQueue.pop();
    return event;
}

// A function to add an event to the eventQueue
void addEvent(Event *event)
{
    eventQueue.push(event);
}

// A function to simulate the execution of events
void simulate(Scheduler *scheduler)
{
    Event *event;
    while ((event = getEvent()) != NULL)
    {
        Process *process = event->process;
        int currentTime = event->timeStamp; // TODO: What is CURRENT_TIME?
        int timeInPreviousState = currentTime - process->stateTimeStamp;
        Transition transition = event->transition;
        event = NULL;
        switch (transition)
        {
        case TO_READY:
            // Must come from CREATED or BLOCKED
            // Add the process to the ready queue, no event is generated
            scheduler->addProcess(process);
            break;
        case TO_PREEMPT:
            // Must come from RUNNING
            // Add the process to the ready queue, no event is generated
            scheduler->addProcess(process);
            break;
        case TO_RUNNING:
            // Create event for preemption or blocking
            // Must come from READY
            break;
        }
    }
}

// A function to parse the scheduler specification and return the time quantum and the maximum number of priorities
void parseSchedulerSpecificationNumMaxprios(int *quantum, int *maxprios, char *schedulerSpec)
{
    // Extract the quantum and maxprio, <num>[:<maxprio>]. maxprio is optional
    char *num, *maxpriosStr;
    num = strtok(schedulerSpec + 1, ":");
    maxpriosStr = strtok(NULL, ":");
    *quantum = atoi(num);
    if (maxpriosStr != NULL)
        *maxprios = atoi(maxpriosStr);
    cout << "Quantum: " << *quantum << ", Maxprios: " << *maxprios << endl;
}

// A function to initialise the scheduler based on the scheduler specification
Scheduler *initScheduler(char *schedulerSpec)
{
    Scheduler *scheduler = nullptr;
    int quantum = 10000, maxprios = 4; // Default values for quantum and maxprios
    if (schedulerSpec[0] == 'F')       // FCFS
    {
        scheduler = new FCFS();
    }
    else if (schedulerSpec[0] == 'L') // LCFS
    {
        // scheduler = LCFS();
        ;
    }
    else if (schedulerSpec[0] == 'S') // SRTF
    {
        // scheduler = SRTF();
        ;
    }
    else if (schedulerSpec[0] == 'R') // Round Robin
    {
        // Extract the quantum
        parseSchedulerSpecificationNumMaxprios(&quantum, &maxprios, schedulerSpec);
        // scheduler = RoundRobin(quantum);
    }
    else if (schedulerSpec[0] == 'P') // Priority
    {
        // Extract the quantum and maxprios
        parseSchedulerSpecificationNumMaxprios(&quantum, &maxprios, schedulerSpec);
        // scheduler = Priority(quantum, maxprios);
    }
    else if (schedulerSpec[0] == 'E') // Preemptive Priority
    {
        // Extract the quantum and maxprios
        parseSchedulerSpecificationNumMaxprios(&quantum, &maxprios, schedulerSpec);
        // scheduler = PreemptivePriority(quantum, maxprios);
    }

    return scheduler;
}

// Main function
int main(int argc, char *argv[])
{
    int opt;
    bool showHelp = false, verbose = false, traceEventExecution = false, showEventQueue = false, showPreemption = false;
    const char *optstring = "hvteps:";

    // Initialize the scheduler object. TODOL: Make it a global variable
    Scheduler *scheduler = nullptr;

    // Parse the command line arguments
    while ((opt = getopt(argc, argv, optstring)) != -1)
    {
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
            scheduler = initScheduler(optarg);
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
        inputFile = fopen(argv[optind++], "r");
        if (inputFile == NULL)
        {
            cout << "Error: Cannot open input file. Use -h for help." << endl;
            exit(1);
        }
        // Check if the random file has been specified
        if (optind < argc)
        {
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

    // Read the random values from the random file
    readRandomFile(randomFile);

    // Read the input file and populate the eventQueue
    readInputFile(inputFile, scheduler->maxprios);

    // Run the event simulation
    simulate(scheduler);

    return 0;
}
