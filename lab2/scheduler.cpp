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

// A Process class to store the process information
class Process
{
public:
    int processNumber;    // The process number
    int arrivalTime;      // The arrival time of the process
    int cpuTime;          // The CPU time of the process
    int remainingCpuTime; // The remaining CPU time of the process
    int cpuBurst;         // The defined CPU burst of the process
    int currentCpuBurst;  // The current CPU burst of the process
    int ioBurst;          // The defined I/O burst of the process
    int staticPriority;   // The static priority of the process
    int dynamicPriority;  // The dynamic priority of the process
    int stateTimeStamp;   // The time stamp when the process changed state
    int finishTime;       // The finish time of the process
    int turnaroundTime;   // The turnaround time of the process
    int ioTime = 0;       // The time spent in I/O
    int cpuWaitTime = 0;  // The time spent waiting for the CPU in the ready state
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
    string name;                                   // The name of the scheduler
    int quantum = 10000;                           // The quantum of the scheduler. Default value is 10K
    int maxprios = 4;                              // The maximum number of priorities. Default value is 4
    int ioTime = 0;                                // The time spent in I/O
    vector<vector<int>> ioTimeStamps;              // The time stamps for I/O
    int cpuTime = 0;                               // The time spent by CPU
    virtual void addProcess(Process *process) = 0; // A function to add a process to the ready queue
    virtual Process *getNextProcess() = 0;         // A function to get the next process from the ready queue
};

// Initialize the scheduler object.
Scheduler *scheduler = nullptr;

// A First Come First Serve (FCFS) Scheduler class
class FCFS : public Scheduler
{
    deque<Process *> readyQueue; // The ready queue to store the processes
public:
    // Constructor to initialize the name of the scheduler
    FCFS()
    {
        name = "FCFS";
    }
    // Add a process to the ready queue
    void addProcess(Process *process)
    {
        readyQueue.push_back(process);
    }

    // Get the next process from the ready queue. The first process added is returned first
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
    deque<Process *> readyQueue; // The ready queue to store the processes
public:
    // Constructor to initialize the name of the scheduler
    LCFS()
    {
        name = "LCFS";
    }
    // Add a process to the ready queue
    void addProcess(Process *process)
    {
        readyQueue.push_back(process);
    }

    // Get the next process from the ready queue. The last process added is returned first
    Process *getNextProcess()
    {
        if (readyQueue.empty())
            return NULL;
        Process *process = readyQueue.back();
        readyQueue.pop_back();
        return process;
    }
};

// A Shortest Remaining Time First (SRTF) Scheduler class
class SRTF : public Scheduler
{
    deque<Process *> readyQueue; // The ready queue to store the processes
public:
    // Constructor to initialize the name of the scheduler
    SRTF()
    {
        name = "SRTF";
    }
    // Add a process to the ready queue
    void addProcess(Process *process)
    {
        readyQueue.push_back(process);
    }

    // Get the next process from the ready queue. The process with the shortest remaining CPU time is returned first
    Process *getNextProcess()
    {
        if (readyQueue.empty())
            return NULL;
        Process *shortestProcess = readyQueue.front();
        for (int i = 1; i < readyQueue.size(); i++)
        {
            if (readyQueue[i]->remainingCpuTime < shortestProcess->remainingCpuTime)
                shortestProcess = readyQueue[i];
        }
        readyQueue.erase(remove(readyQueue.begin(), readyQueue.end(), shortestProcess), readyQueue.end());
        return shortestProcess;
    }
};

// A Round Robin (RR) Scheduler class
class RoundRobin : public Scheduler
{
    deque<Process *> readyQueue; // The ready queue to store the processes
public:
    // Constructor to initialize the name of the scheduler and the quantum
    RoundRobin(int quantum)
    {
        name = "RR " + to_string(quantum);
        this->quantum = quantum;
    }
    // Add a process to the ready queue
    void addProcess(Process *process)
    {
        readyQueue.push_back(process);
    }

    // Get the next process from the ready queue. The process is returned in a round-robin fashion based on the quantum.
    Process *getNextProcess()
    {
        if (readyQueue.empty())
            return NULL;
        Process *process = readyQueue.front();
        readyQueue.pop_front();
        return process;
    }
};

// A Priority Scheduler class
class Priority : public Scheduler
{
    // Initialise an active queue and an expired queue.
    deque<Process *> *activeQueue;
    deque<Process *> *expiredQueue;

public:
    // Constructor to initialize the name of the scheduler, the quantum, and the maximum number of priorities
    Priority(int quantum, int maxprios)
    {
        name = "PRIO " + to_string(quantum);
        this->quantum = quantum;
        this->maxprios = maxprios;
        activeQueue = new deque<Process *>[maxprios];
        expiredQueue = new deque<Process *>[maxprios];
    }
    // Add a process to the active queue based on the dynamic priority
    void addProcess(Process *process)
    {
        if (process->dynamicPriority <= -1)
        {
            process->dynamicPriority = process->staticPriority - 1;
            expiredQueue[process->dynamicPriority].push_back(process);
        }
        else
            activeQueue[process->dynamicPriority].push_back(process);
    }

    // Get the next process from the active queue based on the dynamic priority
    Process *getNextProcess()
    {
        Process *process = NULL;
        for (int i = maxprios - 1; i >= 0; i--)
        {
            if (!activeQueue[i].empty())
            {
                process = activeQueue[i].front();
                activeQueue[i].pop_front();
                return process;
            }
        }
        // If no process is found in the active queue, swap the active and expired queues
        deque<Process *> *temp = activeQueue;
        activeQueue = expiredQueue;
        expiredQueue = temp;

        // Find the next process in the active queue
        for (int i = maxprios - 1; i >= 0; i--)
        {
            if (!activeQueue[i].empty())
            {
                process = activeQueue[i].front();
                activeQueue[i].pop_front();
                return process;
            }
        }
        // Return NULL if no process is found
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

// A vector to store the processes
vector<Process *> processes;

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

// A function to display the event queue
void displayEventQueue()
{
    if (eventQueue.empty())
        cout << "()";
    else
        for (int i = 0; i < eventQueue.size(); i++)
        {
            {
                cout << "(t=" << eventQueue[i]->timeStamp << " "
                     << "pid=" << eventQueue[i]->process->processNumber << " "
                     << "prio=" << eventQueue[i]->process->dynamicPriority << " "
                     << stateToString(eventQueue[i]->oldState) << "->"
                     << stateToString(eventQueue[i]->newState) << ") | ";
            }
        }
}

// A function to add an event to the eventQueue in order of the timestamp
void addEvent(Event *event, bool showEventQueue = false)
{
    if (showEventQueue)
    {
        cout << "AddEvent(t=" << event->timeStamp
             << " pid=" << event->process->processNumber
             << " prio=" << event->process->dynamicPriority
             << " trans=" << stateToString(event->oldState) << "->" << stateToString(event->newState) << "): ";
        displayEventQueue();
        cout << " => ";
    }

    // If the eventQueue is empty, add the event to the eventQueue
    if (eventQueue.empty())
        eventQueue.push_back(event);
    else
    {
        bool added = false;
        // Find the correct position to insert the event based on the timestamp
        for (int i = 0; i < eventQueue.size(); i++)
        {
            if (eventQueue[i]->timeStamp > event->timeStamp)
            {
                eventQueue.insert(eventQueue.begin() + i, event);
                added = true;
                break;
            }
        }
        // Add the event to the end of the eventQueue if the timestamp is greater than all the events in the eventQueue
        if (!added)
            eventQueue.push_back(event);
    }

    if (showEventQueue)
    {
        displayEventQueue();
        cout << endl;
    }
}

// A function to read the input file and populate the eventQueue
void readInputFile(FILE *inputFile, int maxprios = 4, bool showEventQueue = false)
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
        process->remainingCpuTime = process->cpuTime;
        process->cpuBurst = atoi(strtok(NULL, " "));
        process->currentCpuBurst = 0;
        process->ioBurst = atoi(strtok(NULL, " "));
        process->staticPriority = randomNumberGenerator(maxprios);
        process->dynamicPriority = process->staticPriority - 1;
        process->stateTimeStamp = process->arrivalTime;
        processes.push_back(process);

        // Create an event for the process and push it to the eventQueue
        Event *event = new Event();
        event->timeStamp = process->arrivalTime;
        event->process = process;
        event->oldState = CREATED;
        event->newState = READY;
        event->transition = TO_READY;
        addEvent(event, showEventQueue);
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

// A function to get the timestamp of the next event in the eventQueue
int getNextEventTimeStamp()
{
    if (eventQueue.empty())
        return -1;
    return eventQueue.front()->timeStamp;
}

// A function to print the verbose output during state transitions
void displayStateTransition(int currentTime, int processNumber, int timeInPreviousState, string oldState = "", string newState = "", string message = "")
{
    string transitionStr;
    if (oldState == "" && newState == "")
        transitionStr = "DONE";
    else
        transitionStr = oldState + " -> " + newState;
    cout << "t=" << currentTime << " pid=" << processNumber << " tps=" << timeInPreviousState << ": "
         << transitionStr << " " << message << endl;
}

// A function to simulate the execution of events
void simulate(bool showStateTransition, bool showRunQueue, bool showEventQueue, bool showPreemptionDecision)
{
    Event *event;

    // Initialize the variables to call the scheduler and the current running process
    bool callScheduler = false;
    Process *currentRunningProcess = nullptr;

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

        // Execute the event based on the transition
        switch (transition)
        {
        case TO_READY: // Must come from CREATED, RUNNING or BLOCKED
            // If there is a running process and it is the same as the process from the current event,
            // then set the currentRunningProcess to NULL as the process is transitioning to the ready state
            if (currentRunningProcess != nullptr && currentRunningProcess->processNumber == process->processNumber)
                currentRunningProcess = nullptr;

            if (oldState == BLOCKED) // If the process is returning from I/O
            {
                // Reset the dynamic priority
                process->dynamicPriority = process->staticPriority - 1;
                // Add the I/O time to the process and add the timestamp to the ioTimeStamps vector
                process->ioTime += timeInPreviousState;
                scheduler->ioTimeStamps.push_back({process->stateTimeStamp, currentTime});
            }

            // Set the state timestamp of the process as the current time
            process->stateTimeStamp = currentTime;
            // Add the process to the ready queue, no event is generated
            scheduler->addProcess(process);
            // Call the scheduler to get the next process
            callScheduler = true;

            // Print the state transition if the showStateTransition flag is set
            if (showStateTransition)
                displayStateTransition(currentTime, process->processNumber, timeInPreviousState, oldStateStr, newStateStr);
            break;

        case TO_PREEMPT: // Must come from RUNNING
            // Set the currentRunningProcess to NULL as the process is being preempted
            currentRunningProcess = nullptr;
            // Set the state timestamp of the process as the current time
            process->stateTimeStamp = currentTime;

            // Print the state transition if the showStateTransition flag is set
            if (showStateTransition)
            {
                string message = "[cb=" + to_string(process->currentCpuBurst) + " rem=" + to_string(process->remainingCpuTime) + " prio=" + to_string(process->dynamicPriority) + "]";
                displayStateTransition(currentTime, process->processNumber, timeInPreviousState, oldStateStr, newStateStr, message);
            }

            // Call the scheduler to get the next process
            callScheduler = true;
            if (process->remainingCpuTime > 0) // If the process has remaining CPU time
            {
                // Decrement the dynamic priority of the process
                process->dynamicPriority--;
                // Add the process to the ready queue, no event is generated
                scheduler->addProcess(process);
            }
            else // Process has no remaining CPU time, so it is done executing
            {
                // Set the current time as the finish time of the process
                process->finishTime = currentTime;
                // Calculate the turnaround time of the process
                process->turnaroundTime = currentTime - process->arrivalTime;
                // Print the state transition if the showStateTransition flag is set
                if (showStateTransition)
                    displayStateTransition(currentTime, process->processNumber, timeInPreviousState, "", "");
            }
            break;

        case TO_RUNNING: // Must come from READY
        {
            // Set the currentRunningProcess to the process as it is now running
            currentRunningProcess = process;
            // Set the CPU wait time of the process as the time spent in the ready state
            process->cpuWaitTime += timeInPreviousState;
            // Fetch the defined CPU burst and the remaining execution time of the process
            int cpuBurst = process->cpuBurst;
            int remainingExecutionTime = process->remainingCpuTime;
            // Generate the CPU burst only if the current CPU burst is 0, else use the current CPU burst
            process->currentCpuBurst = process->currentCpuBurst > 0 ? process->currentCpuBurst : randomNumberGenerator(cpuBurst);

            // Print the state transition if the showStateTransition flag is set
            if (showStateTransition)
            {
                string message = "[cb=" + to_string(process->currentCpuBurst) + " rem=" + to_string(remainingExecutionTime) + " prio=" + to_string(process->dynamicPriority) + "]";
                displayStateTransition(currentTime, process->processNumber, timeInPreviousState, oldStateStr, newStateStr, message);
            }

            // Check if the process is yet to finish executing
            if (remainingExecutionTime > 0)
            {
                // Check if the process needs to be preempted
                bool preempt = false;
                // Fetch the current CPU burst for execution
                int currentCpuBurstForExecution = process->currentCpuBurst;
                // If the generated CPU burst is greater than the quantum, then the process needs to be preempted
                if (currentCpuBurstForExecution > scheduler->quantum)
                {
                    preempt = true;
                    // Set the current CPU burst for execution as the quantum
                    currentCpuBurstForExecution = scheduler->quantum;
                }
                // If the current CPU burst is greater than the remaining execution time,
                // then set the current CPU burst as the remaining execution time
                if (remainingExecutionTime < currentCpuBurstForExecution)
                    currentCpuBurstForExecution = remainingExecutionTime;
                // Update the remaining execution time of the process
                process->currentCpuBurst -= currentCpuBurstForExecution;

                // Calculate the time to the next event and the remaining execution time after the current CPU burst
                int timeToNextEvent = currentTime + currentCpuBurstForExecution;
                int remainingExecutionTimeAfterCurrentCpuBurst = remainingExecutionTime - currentCpuBurstForExecution;
                process->remainingCpuTime = remainingExecutionTimeAfterCurrentCpuBurst;
                // Set the state timestamp of the process as the current time
                process->stateTimeStamp = currentTime;

                // Create an event for the process to transition to READY or BLOCKED
                Event *event = new Event();
                event->timeStamp = timeToNextEvent;
                event->process = process;
                if (preempt)
                {
                    // Preempt the process and transition to READY
                    event->oldState = RUNNING;
                    event->newState = READY;
                    event->transition = TO_PREEMPT;
                }
                else
                {
                    // Finish the CPU burst and transition to BLOCKED
                    event->oldState = RUNNING;
                    event->newState = BLOCKED;
                    event->transition = TO_BLOCKED;
                }
                addEvent(event, showEventQueue);
            }
            break;
        }

        case TO_BLOCKED: // Must come from RUNNING
            // Set the currentRunningProcess to NULL as the process is transitioning to the blocked state
            currentRunningProcess = nullptr;
            // Call the scheduler to get the next process
            callScheduler = true;

            // Create an event for when process becomes READY again
            if (process->remainingCpuTime > 0) // Execute only if the process has remaining CPU time
            {
                // Calculate the I/O burst
                int ioBurst = process->ioBurst;
                int currentIoBurst = randomNumberGenerator(ioBurst);
                // Calculate the time to the next event and set the state timestamp of the process as the current time
                int timeToNextEvent = currentTime + currentIoBurst;
                process->stateTimeStamp = currentTime;

                // Print the state transition if the showStateTransition flag is set
                if (showStateTransition)
                {
                    string message = "[ib=" + to_string(currentIoBurst) + " rem=" + to_string(process->remainingCpuTime) + "]";
                    displayStateTransition(currentTime, process->processNumber, timeInPreviousState, oldStateStr, newStateStr, message);
                }

                // Create an event for the process to transition to READY
                Event *event = new Event();
                event->timeStamp = timeToNextEvent;
                event->process = process;
                event->oldState = BLOCKED;
                event->newState = READY;
                event->transition = TO_READY;
                addEvent(event, showEventQueue);
            }
            else // Process has no remaining CPU time, so it is done executing
            {
                // Set the current time as the finish time of the process
                process->finishTime = currentTime;
                // Calculate the turnaround time of the process
                process->turnaroundTime = currentTime - process->arrivalTime;
                // Print the state transition if the showStateTransition flag is set
                if (showStateTransition)
                    displayStateTransition(currentTime, process->processNumber, timeInPreviousState, "", "");
            }
            break;
        }

        // Call the scheduler to get the next process
        if (callScheduler)
        {
            if (getNextEventTimeStamp() == currentTime)
                // If the next event is at the same time as the current time, process the next event
                continue;

            callScheduler = false;
            if (currentRunningProcess == nullptr) // If there is no running process
            {
                // Get the next process from the scheduler
                currentRunningProcess = scheduler->getNextProcess();
                if (currentRunningProcess == nullptr) // If there are no more processes in the ready queue
                    continue;

                // Create an event for the process to transition to RUNNING
                Event *event = new Event();
                event->timeStamp = currentTime;
                event->process = currentRunningProcess;
                event->oldState = READY;
                event->newState = RUNNING;
                event->transition = TO_RUNNING;
                addEvent(event, showEventQueue);
            }
        }
    }
}

// A function to compute the total I/O time by merging the overlapping I/O intervals
void computeSchedulerTotalIoTime()
{
    if (scheduler->ioTimeStamps.empty()) // If there are no I/O time stamps, return
        return;

    // Sort the intervals based on the start time
    sort(scheduler->ioTimeStamps.begin(), scheduler->ioTimeStamps.end());

    // Merge overlapping intervals
    int start = scheduler->ioTimeStamps[0][0];
    int end = scheduler->ioTimeStamps[0][1];

    // Iterate over the intervals
    for (int i = 1; i < scheduler->ioTimeStamps.size(); ++i)
    {
        // Check for overlap
        if (scheduler->ioTimeStamps[i][0] <= end)
            // Overlapping intervals, update the end time
            end = max(end, scheduler->ioTimeStamps[i][1]);
        else
        {
            // No overlap, add the duration and reset start and end
            scheduler->ioTime += end - start;
            start = scheduler->ioTimeStamps[i][0];
            end = scheduler->ioTimeStamps[i][1];
        }
    }
    // Add the last interval
    scheduler->ioTime += end - start;
}

// A function to display the process information
void displayProcessInfo()
{
    // Variables to store summary statistics
    int simulationFinishTime = 0, totalTurnaroundTime = 0, totalWaitTime = 0;
    // Print the process information
    cout << scheduler->name << endl;
    for (int i = 0; i < processes.size(); i++)
    {
        Process *process = processes[i];
        printf("%04d: %4d %4d %4d %4d %1d | %5d %5d %5d %5d\n",
               process->processNumber, process->arrivalTime, process->cpuTime, process->cpuBurst, process->ioBurst, process->staticPriority,
               process->finishTime, process->turnaroundTime, process->ioTime, process->cpuWaitTime);

        // TODO: Rewrite the above printf statement using cout
        // cout << setw(4) << setfill('0') << process->processNumber << ": ";
        // cout << setw(4) << setfill(' ') << process->arrivalTime << " ";
        // cout << setw(4) << process->cpuTime << " ";
        // cout << setw(4) << process->cpuBurst << " ";
        // cout << setw(4) << process->ioBurst << " ";
        // cout << setw(4) << process->staticPriority << " |";
        // cout << setw(4) << process->finishTime << " ";
        // cout << setw(4) << process->turnaroundTime << " ";
        // cout << setw(4) << process->ioTime << " ";
        // cout << setw(4) << process->cpuWaitTime << endl;

        // Update the simulation finish time if the current process finish time is greater
        simulationFinishTime = (process->finishTime > simulationFinishTime)
                                   ? process->finishTime
                                   : simulationFinishTime;

        // TODO: Computer scheduler total CPU time in simulate function
        // Update the scheduler statistics from the current process
        scheduler->cpuTime += process->cpuTime;
        totalTurnaroundTime += process->turnaroundTime;
        totalWaitTime += process->cpuWaitTime;
    }

    // Calculate the summary statistics
    // TODO: Compute processes.size() only once
    computeSchedulerTotalIoTime();
    double cpuUtilization = 100.0 * (scheduler->cpuTime / (double)simulationFinishTime);
    double ioUtilization = 100.0 * (scheduler->ioTime / (double)simulationFinishTime);
    double throughput = 100.0 * (processes.size() / (double)simulationFinishTime);
    double avgTurnaroundTime = totalTurnaroundTime / (double)processes.size();
    double avgWaitTime = totalWaitTime / (double)processes.size();

    printf("SUM: %d %.2lf %.2lf %.2lf %.2lf %.3lf\n", simulationFinishTime, cpuUtilization, ioUtilization, avgTurnaroundTime, avgWaitTime, throughput);

    // TODO: Rewrite the above printf statement using cout
    // cout << "SUM: "
    //      << simulationFinishTime << " "
    //      << fixed << setprecision(2) << cpuUtilization << " "
    //      << fixed << setprecision(2) << ioUtilization << " "
    //      << fixed << setprecision(2) << avgTurnaroundTime << " "
    //      << fixed << setprecision(2) << avgWaitTime << " "
    //      << fixed << setprecision(3) << throughput << endl;
}

// A function to parse the scheduler specification and return the time quantum and the maximum number of priorities
void parseSchedulerSpecificationNumMaxprios(int *quantum, int *maxprios, char *schedulerSpec)
{
    // Extract the quantum and maxprio values from the scheduler specification
    char *num, *maxpriosStr;
    num = strtok(schedulerSpec + 1, ":");
    maxpriosStr = strtok(NULL, ":");
    *quantum = atoi(num);
    if (maxpriosStr != NULL) // If the maxprios value is specified
        *maxprios = atoi(maxpriosStr);
}

// A function to initialise the scheduler based on the scheduler specification
void initScheduler(char *schedulerSpec)
{
    int quantum = 10000, maxprios = 4; // Default values for quantum and maxprios
    if (schedulerSpec[0] == 'F')       // FCFS
        scheduler = new FCFS();
    else if (schedulerSpec[0] == 'L') // LCFS
        scheduler = new LCFS();
    else if (schedulerSpec[0] == 'S') // SRTF
        scheduler = new SRTF();
    else if (schedulerSpec[0] == 'R') // Round Robin
    {
        // Extract the quantum
        parseSchedulerSpecificationNumMaxprios(&quantum, &maxprios, schedulerSpec);
        scheduler = new RoundRobin(quantum);
    }
    else if (schedulerSpec[0] == 'P') // Priority
    {
        // Extract the quantum and maxprios
        parseSchedulerSpecificationNumMaxprios(&quantum, &maxprios, schedulerSpec);
        scheduler = new Priority(quantum, maxprios);
    }
    else if (schedulerSpec[0] == 'E') // Preemptive Priority
    {
        // Extract the quantum and maxprios
        parseSchedulerSpecificationNumMaxprios(&quantum, &maxprios, schedulerSpec);
        // scheduler = new PreemptivePriority(quantum, maxprios);
    }
}

// Main function
int main(int argc, char *argv[])
{
    int opt;
    bool showHelp = false, showStateTransition = false, showRunQueue = false, showEventQueue = false, showPreemptionDecision = false;
    const char *optstring = "hvteps:";

    // Parse the command line arguments
    while ((opt = getopt(argc, argv, optstring)) != -1)
    {
        switch (opt)
        {
        case 'h': // Show help message
            showHelp = true;
            break;
        case 'v': // Verbose mode to show state transitions
            showStateTransition = true;
            break;
        case 't':
            showRunQueue = true; // Show run queue
            break;
        case 'e':
            showEventQueue = true; // Show event queue
            break;
        case 'p':
            showPreemptionDecision = true; // Show preemption decision for PREPRIO
            break;
        case 's':
            initScheduler(optarg); // Initialise the scheduler based on the scheduler specification
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
        cout << "  -h        show help message" << endl;
        cout << "  -v        show state transitions" << endl;
        cout << "  -t        show run queue before and after insertion" << endl;
        cout << "  -e        show event queue before and after insertion" << endl;
        cout << "  -p        show preemption decision for PREPRIO" << endl;
        cout << "  -s        scheduler specification (FLS | R<num> | P<num>[:<maxprio>] | E<num>[:<maxprios>])\n";
        exit(0);
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
            cout << "Error: Cannot open input file. Use -h for help." << endl;
            exit(1);
        }
        // Check if the random file has been specified
        if (optind < argc)
        {
            // Open the random file
            randomFile = fopen(argv[optind++], "r");
            if (randomFile == NULL) // Check if the random file can be opened
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
    readInputFile(inputFile, scheduler->maxprios, showEventQueue);

    // Run the event simulation
    simulate(showStateTransition, showRunQueue, showEventQueue, showPreemptionDecision);

    // Close the input and random files
    fclose(inputFile);
    fclose(randomFile);

    // Print the process statistics
    displayProcessInfo();

    return 0;
}
