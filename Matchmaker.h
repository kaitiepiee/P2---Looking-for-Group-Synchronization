#ifndef MATCHMAKER_H
#define MATCHMAKER_H

#include <queue>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>

extern int maxInstances, numTanks, numHealers, numDPS;
extern int minTime, maxTime;
extern const int PARTY_SIZE;

void displayBanner();
void getInput();
void initialize();
void partyManager();
void addPlayersToQueue();
void showSummary();

#endif
