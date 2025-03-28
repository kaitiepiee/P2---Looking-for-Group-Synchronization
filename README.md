
# P2 - Looking for Group Synchronization

A C++ multithreaded simulation where tanks, healers, and DPS parties are formed and assigned to dungeon instances. The system handles synchronization, thread safety, and prevents deadlocks/starvation using queues, condition variables, and mutexes.

## How to Run

Compile:

`g++ main.cpp Matchmaker.cpp -o myProgram -std=c++11 -pthread`

Run:

`./myProgram`

## Language and Environment
- C++11
- Developed on macOS. May require adjustments on other platforms.
- Uses standard C++ libraries: thread, mutex, condition_variable

## Project Structure
- main.cpp – Entry point
- Matchmaker.cpp – Core logic and thread handling
- Matchmaker.h – Shared declarations

## Input Validation
- Max instances must be > 0
- At least 1 tank, 1 healer, and 3 DPS
- Total players must be enough to form 1 party
- Min time > 0
- Max time > 0 and must be ≥ min time
- Warning for unbalanced DPS or too many instances

## Synchronization
- std::thread to run dungeons in parallel
- mutex to protect shared data
- lock_guard and unique_lock for safe access
- condition_variable to wait for valid party combinations

## Deadlock and Starvation Handling
- All players for a party are acquired together while holding the lock
- FIFO queues prevent starvation
- No circular waits or partial resource holding
"""
