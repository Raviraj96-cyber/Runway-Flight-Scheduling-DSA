/*
 * Runway Flight Scheduling and Management (DSA Mini Project) - C++ version
 *
 * Data structures:
 *   - Custom binary MIN-HEAP ................. next flight to use a runway
 *   - unordered_map (id -> heap index) ....... lookup, update and cancellation
 *   - vector<Runway> ......................... runway availability
 *   - vector<string> ........................ operation log
 *
 * Compile: g++ -std=c++17 -o flight_scheduler src/flight_scheduler.cpp
 * Run:     ./flight_scheduler
 */
#include <algorithm>
#include <cctype>
#include <climits>
#include <cstdio>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
using namespace std;

enum Priority { EMERGENCY = 0, LOW_FUEL = 1, NORMAL = 2 };
enum Operation { LANDING = 0, TAKEOFF = 1 };

const char* priorityName(Priority p) {
    return p == EMERGENCY ? "EMERGENCY" : (p == LOW_FUEL ? "LOW_FUEL" : "NORMAL");
}
const char* opName(Operation o) { return o == LANDING ? "LANDING" : "TAKEOFF"; }
int durationOf(Operation o) { return o == LANDING ? 5 : 3; }

string fmt(int t) {
    char buf[8];
    snprintf(buf, sizeof(buf), "%02d:%02d", (t / 60) % 24, t % 60);
    return buf;
}

struct Flight {
    string id, airline;
    Operation op;
    Priority priority;
    int fuel;
    long seq;

    bool before(const Flight& o) const {
        if (priority != o.priority) return priority < o.priority;
        if (op != o.op) return op < o.op;
        if (fuel != o.fuel) return fuel < o.fuel;
        return seq < o.seq;
    }
};

class MinHeap {
    vector<Flight> a;
    unordered_map<string, int> pos;

    void swapAt(int i, int j) {
        swap(a[i], a[j]);
        pos[a[i].id] = i;
        pos[a[j].id] = j;
    }
    void siftUp(int i) {
        while (i > 0) {
            int p = (i - 1) / 2;
            if (a[i].before(a[p])) { swapAt(i, p); i = p; }
            else break;
        }
    }
    void siftDown(int i) {
        int n = static_cast<int>(a.size());
        while (true) {
            int l = 2 * i + 1, r = 2 * i + 2, s = i;
            if (l < n && a[l].before(a[s])) s = l;
            if (r < n && a[r].before(a[s])) s = r;
            if (s == i) break;
            swapAt(i, s);
            i = s;
        }
    }
    void removeAt(int i) {
        int last = static_cast<int>(a.size()) - 1;
        string removedId = a[i].id;
        if (i != last) swapAt(i, last);
        a.pop_back();
        pos.erase(removedId);
        if (i < static_cast<int>(a.size())) {
            string movedId = a[i].id;
            siftUp(i);
            siftDown(pos[movedId]);
        }
    }

public:
    int size() const { return static_cast<int>(a.size()); }
    bool empty() const { return a.empty(); }
    bool contains(const string& id) const { return pos.count(id) > 0; }
    Flight* get(const string& id) {
        auto it = pos.find(id);
        return it == pos.end() ? nullptr : &a[it->second];
    }
    void insert(const Flight& f) {
        a.push_back(f);
        pos[f.id] = static_cast<int>(a.size()) - 1;
        siftUp(static_cast<int>(a.size()) - 1);
    }
    bool extractMin(Flight& out) {
        if (a.empty()) return false;
        out = a[0];
        removeAt(0);
        return true;
    }
    bool remove(const string& id) {
        auto it = pos.find(id);
        if (it == pos.end()) return false;
        removeAt(it->second);
        return true;
    }
    void update(const string& id) {
        auto it = pos.find(id);
        if (it == pos.end()) return;
        siftUp(it->second);
        siftDown(pos[id]);
    }
    vector<string> ids() const {
        vector<string> v;
        for (const auto& f : a) v.push_back(f.id);
        return v;
    }
    vector<Flight> sortedSnapshot() const {
        vector<Flight> c = a;
        sort(c.begin(), c.end(),
             [](const Flight& x, const Flight& y) { return x.before(y); });
        return c;
    }
};

struct Runway {
    int id, freeAt, handled = 0;
    string last = "-";
};

class Scheduler {
    MinHeap queue;
    vector<Runway> runways;
    vector<string> logLines;
    int clock = 8 * 60;
    long seqCounter = 0;

    void record(const string& msg) {
        string line = "[" + fmt(clock) + "] " + msg;
        logLines.push_back(line);
        cout << line << "\n";
    }
    static void autoClassify(Flight& f) {
        if (f.op != LANDING || f.priority == EMERGENCY) return;
        if (f.fuel <= 10) f.priority = EMERGENCY;
        else if (f.fuel <= 30 && f.priority == NORMAL) f.priority = LOW_FUEL;
    }

public:
    explicit Scheduler(int n) {
        for (int i = 0; i < n; i++) runways.push_back({i + 1, clock});
    }

    void addFlight(const string& id, const string& airline, Operation op, Priority p, int fuel) {
        if (queue.contains(id)) {
            cout << "Error: flight " << id << " is already in the queue.\n";
            return;
        }
        Flight f{id, airline, op, p, op == LANDING ? fuel : INT_MAX, seqCounter++};
        Priority requested = f.priority;
        autoClassify(f);
        queue.insert(f);
        record("Request queued: " + id + " (" + airline + ") " + opName(op) +
               " [" + priorityName(f.priority) + "]" +
               (f.priority != requested ? "  <- auto-escalated by fuel level" : ""));
    }

    void declareEmergency(const string& id) {
        Flight* f = queue.get(id);
        if (!f) { cout << "Error: flight " << id << " not in queue.\n"; return; }
        if (f->priority == EMERGENCY) {
            cout << id << " is already an EMERGENCY.\n";
            return;
        }
        f->priority = EMERGENCY;
        queue.update(id);
        record("EMERGENCY declared for " + id + " -> moved to front of the queue");
    }

    void cancelFlight(const string& id) {
        if (!queue.remove(id)) cout << "Error: flight " << id << " not in queue.\n";
        else record("Flight " + id + " removed from queue (cancelled / diverted)");
    }

    void showQueue() {
        if (queue.empty()) { cout << "The queue is empty.\n"; return; }
        cout << "\nWaiting queue (in order of service):\n";
        printf("%-4s %-8s %-14s %-8s %-10s %-6s\n",
               "No.", "Flight", "Airline", "Type", "Priority", "Fuel");
        int n = 1;
        for (const auto& f : queue.sortedSnapshot()) {
            string fuel = f.op == LANDING ? to_string(f.fuel) + " min" : "-";
            printf("%-4d %-8s %-14s %-8s %-10s %-6s\n", n++, f.id.c_str(),
                   f.airline.c_str(), opName(f.op), priorityName(f.priority), fuel.c_str());
        }
    }

    void dispatchNext() {
        Flight f;
        if (!queue.extractMin(f)) { cout << "No flights waiting.\n"; return; }
        Runway* r = &runways[0];
        for (auto& x : runways) if (x.freeAt < r->freeAt) r = &x;
        int start = max(clock, r->freeAt);
        int end = start + durationOf(f.op);
        r->freeAt = end;
        r->handled++;
        r->last = f.id;
        record(string(f.op == LANDING ? "LAND   " : "TAKEOFF") + " " + f.id +
               " [" + priorityName(f.priority) + "] -> Runway " + to_string(r->id) +
               "  (" + fmt(start) + " - " + fmt(end) + ")");
        if (f.op == LANDING && f.fuel - (start - clock) < 0)
            cout << "   !! WARNING: " << f.id << " would run out of fuel before touchdown.\n";
    }

    void dispatchAll() {
        if (queue.empty()) { cout << "No flights waiting.\n"; return; }
        while (!queue.empty()) dispatchNext();
    }

    void advanceTime(int minutes) {
        if (minutes < 0) {
            cout << "Time advance cannot be negative.\n";
            return;
        }
        clock += minutes;
        for (const string& id : queue.ids()) {
            Flight* f = queue.get(id);
            if (!f || f->op != LANDING) continue;
            f->fuel -= minutes;
            Priority before = f->priority;
            if (f->fuel <= 0) {
                f->priority = EMERGENCY;
                cout << "   !! " << id << " has EXHAUSTED its fuel - immediate landing required!\n";
            } else autoClassify(*f);
            int newFuel = f->fuel;
            Priority now = f->priority;
            queue.update(id);
            if (now != before)
                cout << "   Auto-escalation: " << id << " " << priorityName(before) << " -> "
                     << priorityName(now) << " (fuel " << newFuel << " min)\n";
        }
        record("Clock advanced by " + to_string(minutes) + " min");
    }

    void showRunways() {
        cout << "\nCurrent time: " << fmt(clock) << "\n";
        printf("%-8s %-10s %-10s %-10s %-8s\n",
               "Runway", "Status", "Free at", "Last", "Handled");
        for (auto& r : runways)
            printf("%-8d %-10s %-10s %-10s %-8d\n", r.id,
                   r.freeAt > clock ? "BUSY" : "FREE",
                   fmt(max(r.freeAt, clock)).c_str(), r.last.c_str(), r.handled);
    }

    void showLog() {
        cout << "\n--- Operation log ---\n";
        if (logLines.empty()) cout << "Log is empty.\n";
        for (const auto& l : logLines) cout << l << "\n";
    }

    void loadDemo() {
        addFlight("AI101", "Air India", LANDING, NORMAL, 60);
        addFlight("6E202", "IndiGo", TAKEOFF, NORMAL, 0);
        addFlight("UK303", "Vistara", LANDING, NORMAL, 25);
        addFlight("EM911", "MedAir", LANDING, EMERGENCY, 40);
        addFlight("SG404", "SpiceJet", TAKEOFF, NORMAL, 0);
        addFlight("AI505", "Air India", LANDING, NORMAL, 90);
    }
};

int readInt(const string& prompt) {
    while (true) {
        cout << prompt;
        string line;
        if (!getline(cin, line)) return 0;
        try {
            size_t used = 0;
            int value = stoi(line, &used);
            if (used != line.size()) throw invalid_argument("extra characters");
            return value;
        } catch (...) {
            cout << "Please enter a valid whole number.\n";
        }
    }
}

string readText(const string& prompt) {
    while (true) {
        cout << prompt;
        string line;
        if (!getline(cin, line)) return "";
        size_t b = line.find_first_not_of(" \t\r\n");
        size_t e = line.find_last_not_of(" \t\r\n");
        if (b != string::npos) return line.substr(b, e - b + 1);
        cout << "Input cannot be empty.\n";
    }
}

string upper(string s) {
    for (auto& c : s) c = static_cast<char>(toupper(static_cast<unsigned char>(c)));
    return s;
}

void printMenu() {
    cout << "\n---------------- MENU ----------------\n"
            " 1. Add flight request\n"
            " 2. Declare emergency for a queued flight\n"
            " 3. Cancel flight\n"
            " 4. Show waiting queue\n"
            " 5. Assign NEXT flight to a runway\n"
            " 6. Process ALL waiting flights\n"
            " 7. Advance time (fuel burns, auto-escalation)\n"
            " 8. Runway status\n"
            " 9. Operation log\n"
            "10. Load demo data\n"
            " 0. Exit\n"
            "--------------------------------------\n";
}

void addFlightUI(Scheduler& s) {
    string id = upper(readText("Flight ID (e.g. AI101): "));
    string airline = readText("Airline: ");

    int operationChoice;
    do {
        operationChoice = readInt("Operation (1 = Landing, 2 = Takeoff): ");
        if (operationChoice != 1 && operationChoice != 2)
            cout << "Please choose 1 or 2.\n";
    } while (operationChoice != 1 && operationChoice != 2);

    Operation op = operationChoice == 1 ? LANDING : TAKEOFF;
    int fuel = 0;
    if (op == LANDING) {
        do {
            fuel = readInt("Fuel remaining (minutes): ");
            if (fuel < 0) cout << "Fuel cannot be negative.\n";
        } while (fuel < 0);
    }

    int p;
    do {
        p = readInt("Priority (1 = Emergency, 2 = Low fuel, 3 = Normal): ");
        if (p < 1 || p > 3) cout << "Please choose 1, 2 or 3.\n";
    } while (p < 1 || p > 3);

    Priority pr = p == 1 ? EMERGENCY : (p == 2 ? LOW_FUEL : NORMAL);
    s.addFlight(id, airline, op, pr, fuel);
}

int main() {
    Scheduler s(2);
    cout << "===== Runway Flight Scheduling System (2 runways) =====\n";
    while (true) {
        printMenu();
        switch (readInt("Choice: ")) {
            case 1: addFlightUI(s); break;
            case 2: s.declareEmergency(upper(readText("Flight ID: "))); break;
            case 3: s.cancelFlight(upper(readText("Flight ID: "))); break;
            case 4: s.showQueue(); break;
            case 5: s.dispatchNext(); break;
            case 6: s.dispatchAll(); break;
            case 7: s.advanceTime(readInt("Minutes to advance: ")); break;
            case 8: s.showRunways(); break;
            case 9: s.showLog(); break;
            case 10: s.loadDemo(); break;
            case 0: cout << "Shutting down. Safe skies!\n"; return 0;
            default: cout << "Invalid choice.\n";
        }
    }
}
