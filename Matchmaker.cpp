#include "Matchmaker.h"
#include <iostream>
#include <random>
#include <chrono>
#include <ctime>
#include <queue>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <algorithm>

using namespace std;

const int PARTY_SIZE = 5;

int maxInstances, numTanks, numHealers, numDPS;
int minTime, maxTime;

queue<int> tankQueue, healerQueue, dpsQueue;
mutex mtx;
condition_variable cv;

int activeInstances = 0;
int totalPartiesServed = 0;
int totalTimeServed = 0;

vector<string> instanceStatus;
vector<int> instancePartyCount;

struct Party {
    int id;
    int tankID;
    int healerID;
    vector<int> dpsIDs;
    int instanceID;
};

vector<Party> partyLog;

random_device rd;
mt19937 gen(rd());
uniform_int_distribution<int> timeDist;

string getCurrentTime() {
    time_t now = time(0);
    tm* localtm = localtime(&now);
    char buf[10];
    strftime(buf, sizeof(buf), "%H:%M:%S", localtm);
    return string(buf);
}

void displayBanner() {
    cout << "==========================================" << endl;
    cout << "  P2 - Looking for Group Synchronization     " << endl;
    cout << "==========================================" << endl;
}

// Getting input + input validation 
void getInput() {
    cout << "Please enter the following configuration\n" << endl;

    // instances
    do {
        cout << "Max concurrent instances (n): ";
        cin >> maxInstances;
        if (maxInstances <= 0)
            cout << "Must be greater than 0.\n";
        else if (maxInstances > 100)
            cout << "Warning: More than 100 instances may slow down the system.\n";
    } while (maxInstances <= 0);

    // tanks
    do {
        cout << "Number of TANK players: ";
        cin >> numTanks;
        if (numTanks < 1)
            cout << "At least 1 tank required.\n";
    } while (numTanks < 1);

    // healers
    do {
        cout << "Number of HEALER players: ";
        cin >> numHealers;
        if (numHealers < 1)
            cout << "At least 1 healer required.\n";
    } while (numHealers < 1);

    // DPS
    do {
        cout << "Number of DPS players: ";
        cin >> numDPS;
        if (numDPS < 3)
            cout << "At least 3 DPS required.\n";
    } while (numDPS < 3);

    // player count validation
    while ((numTanks + numHealers + numDPS) < PARTY_SIZE) {
        cout << "Not enough total players to form a party. Please re-enter all player counts.\n\n";

        do {
            cout << "Number of TANK players: ";
            cin >> numTanks;
        } while (numTanks < 1);

        do {
            cout << "Number of HEALER players: ";
            cin >> numHealers;
        } while (numHealers < 1);

        do {
            cout << "Number of DPS players: ";
            cin >> numDPS;
        } while (numDPS < 3);
    }

    int possibleParties = min({ numTanks, numHealers, numDPS / 3 });
    if (possibleParties == 0) {
        cout << "Error: Not enough players to form any complete party.\n";
        exit(1);
    }

    // warn if DPS count is disproportionately high compared to tanks and healers
    // may cause some DPS players to be left unmatched
    if (numDPS > (numTanks + numHealers) * 5) {
        cout << "Note: DPS player count is much higher than tanks/healers.\n";
        cout << "Some DPS may never be matched into a party.\n";
    }

    // t1
    do {
        cout << "Minimum clear time (t1): ";
        cin >> minTime;
        if (minTime <= 0)
            cout << "Must be greater than 0.\n";
    } while (minTime <= 0);

    // t2
    do {
        cout << "Maximum clear time (t2): ";
        cin >> maxTime;
        if (maxTime <= 0)
            cout << "Must be greater than 0.\n";
        else if (minTime > maxTime)
            cout << "Max time must be greater than or equal to min time.\n";
    } while (maxTime <= 0 || minTime > maxTime);

    cout << "\nInitializing matchmaking system...\n" << endl;
}

void initialize() {
    instanceStatus = vector<string>(maxInstances, "empty");
    instancePartyCount = vector<int>(maxInstances, 0);
    timeDist = uniform_int_distribution<int>(minTime, maxTime);
}

void runDungeon(int instanceID, int partyID) {
    int dungeonTime = timeDist(gen);

    {
        lock_guard<mutex> lock(mtx);
        instanceStatus[instanceID] = "active";
        cout << "\n[" << getCurrentTime() << "] [Instance " << instanceID + 1
             << "] Serving Party " << partyID << "   → ACTIVE" << endl;

        for (const Party& p : partyLog) {
            if (p.id == partyID) {
                cout << "  Tank: Player " << p.tankID << endl;
                cout << "  Healer: Player " << p.healerID << endl;
                cout << "  DPS: Player " << p.dpsIDs[0]
                     << ", Player " << p.dpsIDs[1]
                     << ", Player " << p.dpsIDs[2] << endl;
                break;
            }
        }
    }

    this_thread::sleep_for(chrono::seconds(dungeonTime));

    {
        lock_guard<mutex> lock(mtx);
        instanceStatus[instanceID] = "empty";
        activeInstances--;
        totalPartiesServed++;
        totalTimeServed += dungeonTime;
        cout << "\n[" << getCurrentTime() << "] [Instance " << instanceID + 1
             << "] Finished Party " << partyID << "  → EMPTY" << endl;
        cv.notify_all();
    }
}

void partyManager() {
    int partyID = 1;

    while (true) {
        unique_lock<mutex> lock(mtx);
        cv.wait(lock, [] {
            return !tankQueue.empty() && !healerQueue.empty() && dpsQueue.size() >= 3 && activeInstances < maxInstances;
        });

        int tankID = tankQueue.front(); tankQueue.pop();
        int healerID = healerQueue.front(); healerQueue.pop();
        vector<int> dpsIDs;
        for (int i = 0; i < 3; i++) {
            dpsIDs.push_back(dpsQueue.front());
            dpsQueue.pop();
        }

        int instanceID = -1;
        for (int i = 0; i < instanceStatus.size(); i++) {
            if (instanceStatus[i] == "empty") {
                instanceStatus[i] = "reserved";
                instanceID = i;
                break;
            }
        }

        if (instanceID == -1) {
            cv.notify_all();
            continue;
        }

        activeInstances++;
        int currentPartyID = partyID++;

        Party party = { currentPartyID, tankID, healerID, dpsIDs, instanceID };
        partyLog.push_back(party);
        instancePartyCount[instanceID]++;

        std::thread([party] {
            runDungeon(party.instanceID, party.id);
        }).detach();
    }
}

void addPlayersToQueue() {
    for (int i = 0; i < numTanks; i++) {
        unique_lock<mutex> lock(mtx);
        tankQueue.push(i + 1);
        cv.notify_all();
    }
    for (int i = 0; i < numHealers; i++) {
        unique_lock<mutex> lock(mtx);
        healerQueue.push(i + 1);
        cv.notify_all();
    }
    for (int i = 0; i < numDPS; i++) {
        unique_lock<mutex> lock(mtx);
        dpsQueue.push(i + 1);
        cv.notify_all();
    }
}

void showSummary() {
    lock_guard<mutex> lock(mtx);
    cout << "\n====================\n";
    cout << "       Summary\n";
    cout << "====================\n";
    cout << "Total parties served: " << totalPartiesServed << endl;
    cout << "Total time served: " << totalTimeServed << " seconds" << endl;
    cout << "\nInstance usage breakdown:\n";
    for (int i = 0; i < instancePartyCount.size(); i++) {
        cout << "Instance " << (i + 1) << " served " << instancePartyCount[i] << " parties\n";
    }
    cout << "====================\n";
}
