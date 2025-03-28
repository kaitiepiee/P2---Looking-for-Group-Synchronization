#include <thread>     // add this
#include <chrono>     // add this

#include "Matchmaker.h"

int main() {
    displayBanner();
    getInput();
    initialize();

    std::thread managerThread(partyManager);
    addPlayersToQueue();

    std::this_thread::sleep_for(std::chrono::seconds(maxTime * ((numTanks + numHealers + numDPS) / PARTY_SIZE) + 5));
    
    showSummary();

    managerThread.detach();
    return 0;
}