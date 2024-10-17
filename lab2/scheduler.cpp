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
    deque<Process *> readyQueue;                   // The ready queue to store the processes
    virtual void addProcess(Process *process) = 0; // A function to add a process to the ready queue
    virtual Process *getNextProcess() = 0;         // A function to get the next process from the ready queue
};

// Initialize the scheduler object.
Scheduler *scheduler = nullptr;

// A First Come First Serve (FCFS) Scheduler class
class FCFS : public Scheduler
{
public:
    // Add a process to the ready queue
    void addProcess(Process *process)
    {
        readyQueue.push_back(process);
    }

    // Get the next process from the ready queue. The first process added is returned first.
    Process *getNextProcess()
    {
        if (readyQueue.empty())
            return NULL;
        Process *process = readyQueue.front();
        readyQueue.pop_front();
        return process;
    }
};

// A Last Come First Serve (LCFS) Scheduler class
class LCFS : public Scheduler
{
public:
    // Add a process to the ready queue
    void addProcess(Process *process)
    {
        readyQueue.push_back(process);
    }

    // Get the next process from the ready queue. The last process added is returned first.
    Process *getNextProcess()
    {
        if (readyQueue.empty())
            return NULL;
        Process *process = readyQueue.back();
        readyQueue.pop_back();
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

// A queue to store the events, implemented as a vector for inserting events in order of the timestamp
vector<Event *> eventQueue;

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
        process->dynamicPriority = process->staticPriority - 1;
        process->stateTimeStamp = process->arrivalTime;

        // Create an event for the process and push it to the eventQueue
        Event *event = new Event();
        event->timeStamp = process->arrivalTime;
        event->process = process;
        event->oldState = CREATED;
        event->newState = READY;
        event->transition = TO_READY;
        eventQueue.push_back(event);
    }
}

// A function to return the head of the eventQueue
Event *getEvent()
{
    if (eventQueue.empty())
        return NULL;
    Event *event = eventQueue.front();
    eventQueue.erase(eventQueue.begin());
    return event;
}

// A function to add an event to the eventQueue in order of the timestamp
void addEvent(Event *event)
{
    if (eventQueue.empty())
    {
        eventQueue.push_back(event);
        return;
    }
    for (int i = 0; i < eventQueue.size(); i++)
    {
        if (eventQueue[i]->timeStamp > event->timeStamp)
        {
            eventQueue.insert(eventQueue.begin() + i, event);
            return;
        }
    }
    eventQueue.push_back(event);
}

// A function to get the timestamp of the next event in the eventQueue
int getNextEventTimeStamp()
{
    if (eventQueue.empty())
        return -1;
    return eventQueue.front()->timeStamp;
}

// A function to map the state enum to a string representation
string stateToString(State state)
{
    switch (state)
    {
    case CREATED:
        return "CREATED";
    case READY:
        return "READY";
    case RUNNING:
        return "RUNNING";
    case BLOCKED:
        return "BLOCKED";
    }
    return "";
}

// A function to print the verbose output during state transitions
void verbosePrint(int currentTime, int processNumber, int timeInPreviousState, string oldState = "", string newState = "", string message = "")
{
    string transitionStr;
    if (oldState == "" && newState == "")
        transitionStr = "DONE";
    else
        transitionStr = oldState + " -> " + newState;
    cout << currentTime << " " << processNumber << " " << timeInPreviousState << ": "
         << transitionStr << " " << message << endl;
}

// A function to simulate the execution of events
void simulate(bool verbose, bool traceEventExecution, bool showEventQueue, bool showPreemption)
{
    Event *event;
    while ((event = getEvent()) != NULL)
    {
        // Extract the process information from the event
        Process *process = event->process;
        // Set the current time to the event timestamp
        int currentTime = event->timeStamp;
        // Calculate the time spent in the previous state as the difference between the current time and the state timestamp
        int timeInPreviousState = currentTime - process->stateTimeStamp;

        // Obtain the transition, old state, and new state from the event
        Transition transition = event->transition;
        State oldState = event->oldState, newState = event->newState;
        string oldStateStr = stateToString(oldState), newStateStr = stateToString(newState);
        event = NULL;

        // Initialize the variables to call the scheduler and the current running process
        bool callScheduler = false;
        Process *currentRunningProcess = nullptr;

        // Execute the event based on the transition
        switch (transition)
        {
        case TO_READY:
            // Must come from CREATED or BLOCKED
            // Add the process to the ready queue, no event is generated
            process->stateTimeStamp = currentTime;
            scheduler->addProcess(process);
            callScheduler = true;
            // Print the verbose output if the verbose flag is set
            if (verbose)
                verbosePrint(currentTime, process->processNumber, timeInPreviousState, oldStateStr, newStateStr);
            break;
        case TO_PREEMPT:
            // Must come from RUNNING
            // Add the process to the ready queue, no event is generated
            process->stateTimeStamp = currentTime;
            scheduler->addProcess(process);
            callScheduler = true;
            break;
        case TO_RUNNING:
        {
            // Create event for preemption or blocking
            // Must come from READY
            int cpuBurst = process->cpuBurst;
            int currentCpuBurst = randomNumberGenerator(cpuBurst);
            int quantum = scheduler->quantum;
            int remainingExecutionTime = process->cpuTime;
            if (remainingExecutionTime < currentCpuBurst)
                currentCpuBurst = remainingExecutionTime;
            // Print the verbose output if the verbose flag is set if (verbose)
            {
                string message = "cb=" + to_string(currentCpuBurst) + " rem=" + to_string(remainingExecutionTime) + " prio=" + to_string(process->dynamicPriority);
                verbosePrint(currentTime, process->processNumber, timeInPreviousState, oldStateStr, newStateStr, message);
            }
            if (remainingExecutionTime > 0)
            {
                int timeToNextEvent = currentTime + currentCpuBurst;
                int remainingExecutionTimeAfterCurrentCpuBurst = remainingExecutionTime - currentCpuBurst;
                process->cpuTime = remainingExecutionTimeAfterCurrentCpuBurst;
                process->stateTimeStamp = currentTime;
                process->dynamicPriority--;
                if (process->dynamicPriority <= 0)
                    process->dynamicPriority = process->staticPriority - 1;

                // Create an event for the process to transition to BLOCKED
                Event *event = new Event();
                event->timeStamp = timeToNextEvent;
                event->process = process;
                event->oldState = RUNNING;
                event->newState = BLOCKED;
                event->transition = TO_BLOCKED;
                addEvent(event);
            }
            break;
        }
        case TO_BLOCKED:
            // //create an event for when process becomes READY again
            if (process->cpuTime > 0) // Execute only if the process has remaining CPU time
            {
                // Calculate the I/O burst and the time to the next event
                callScheduler = true;
                int ioBurst = process->ioBurst;
                int currentIoBurst = randomNumberGenerator(ioBurst);
                int timeToNextEvent = currentTime + currentIoBurst;
                process->stateTimeStamp = currentTime;

                // Print the verbose output if the verbose flag is set
                if (verbose)
                {
                    string message = "ib=" + to_string(currentIoBurst) + " rem=" + to_string(process->cpuTime);
                    verbosePrint(currentTime, process->processNumber, timeInPreviousState, oldStateStr, newStateStr, message);
                }

                // Create an event for the process to transition to READY
                Event *event = new Event();
                event->timeStamp = timeToNextEvent;
                event->process = process;
                event->oldState = BLOCKED;
                event->newState = READY;
                event->transition = TO_READY;
                addEvent(event);
            }
            else // Process has no remaining CPU time, so it is done executing
            {
                if (verbose)
                    verbosePrint(currentTime, process->processNumber, timeInPreviousState, "", "");
            }
            break;
        }

        // Call the scheduler to get the next process
        if (callScheduler)
        {
            if (getNextEventTimeStamp() == currentTime)
                continue;
            callScheduler = false;
            if (currentRunningProcess == nullptr)
            {
                // Get the next process from the scheduler
                currentRunningProcess = scheduler->getNextProcess();
                if (currentRunningProcess == nullptr)
                    continue;

                // Create an event for the process to transition to RUNNING
                Event *event = new Event();
                event->timeStamp = currentTime;
                event->process = currentRunningProcess;
                event->oldState = READY;
                event->newState = RUNNING;
                event->transition = TO_RUNNING;
                addEvent(event);
            }
        }
    }
}

// A function to parse the scheduler specification and return the time quantum and the maximum number of priorities
void parseSchedulerSpecificationNumMaxprios(int *quantum, int *maxprios, char *schedulerSpec)
{
    // Extract the quantum and maxprio values from the scheduler specification
    char *num, *maxpriosStr;
    num = strtok(schedulerSpec + 1, ":");
    maxpriosStr = strtok(NULL, ":");
    *quantum = atoi(num);
    if (maxpriosStr != NULL)
        *maxprios = atoi(maxpriosStr);
}

// A function to initialise the scheduler based on the scheduler specification
void initScheduler(char *schedulerSpec)
{
    int quantum = 10000, maxprios = 4; // Default values for quantum and maxprios
    if (schedulerSpec[0] == 'F')       // FCFS
    {
        scheduler = new FCFS();
    }
    else if (schedulerSpec[0] == 'L') // LCFS
    {
        scheduler = new LCFS();
    }
    else if (schedulerSpec[0] == 'S') // SRTF
    {
        // scheduler = new SRTF();
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
}

// Main function
int main(int argc, char *argv[])
{
    int opt;
    bool showHelp = false, verbose = false, traceEventExecution = false, showEventQueue = false, showPreemption = false;
    const char *optstring = "hvteps:";

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
            initScheduler(optarg);
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
    simulate(verbose, traceEventExecution, showEventQueue, showPreemption);

    return 0;
}
