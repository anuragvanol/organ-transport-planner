#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <unordered_map>
#include <queue>
#include <stack>
#include <climits>
#include <iomanip>
using namespace std;

const int STEP = 5;
const int INF = INT_MAX / 2;

struct Edge {
    int to;
    int minutes;
    int junctions;
};

struct Donor {
    string id;
    string blood;
    int hospital;
    vector<string> organs;
    bool processed;
};

struct Recipient {
    string id;
    string blood;
    string organ;
    int urgency;
    int hospital;
    bool assigned;
    string result;
};

struct Route {
    bool found;
    int minutes;
    int junctions;
    stack<int> path;
};

vector<string> hospitalName;
unordered_map<string, int> hospitalId;
vector<vector<Edge>> road;
vector<Donor> donors;
vector<Recipient> recipients;
unordered_map<string, int> recipientIndex;
unordered_map<string, int> waitPos;
int totalOrgans = 0;
int totalPlaced = 0;
int fastMinSum = 0, fastJuncSum = 0;
int dpMinSum = 0, dpJuncSum = 0;

unordered_map<string, int> viabilityMinutes;
unordered_map<string, vector<string>> canDonateTo;

void setupRules() {
    viabilityMinutes["heart"] = 240;
    viabilityMinutes["lung"] = 240;
    viabilityMinutes["liver"] = 480;
    viabilityMinutes["kidney"] = 1440;

    canDonateTo["O"] = {"O", "A", "B", "AB"};
    canDonateTo["A"] = {"A", "AB"};
    canDonateTo["B"] = {"B", "AB"};
    canDonateTo["AB"] = {"AB"};
}

int getHospital(string name) {
    if (hospitalId.count(name) == 0) {
        cout << "Unknown hospital in data file: " << name << "\n";
        exit(1);
    }
    return hospitalId[name];
}

bool loadData(string fileName) {
    ifstream in(fileName);
    if (!in) return false;

    string label, a, b;
    int n, m, d, r;

    in >> label >> n;
    hospitalName.resize(n);
    for (int i = 0; i < n; i++) {
        in >> hospitalName[i];
        hospitalId[hospitalName[i]] = i;
    }
    road.resize(n);

    in >> label >> m;
    for (int i = 0; i < m; i++) {
        int mins, junc;
        in >> a >> b >> mins >> junc;
        int u = getHospital(a);
        int v = getHospital(b);
        Edge e1;
        e1.to = v;
        e1.minutes = mins;
        e1.junctions = junc;
        road[u].push_back(e1);
        Edge e2;
        e2.to = u;
        e2.minutes = mins;
        e2.junctions = junc;
        road[v].push_back(e2);
    }

    in >> label >> d;
    for (int i = 0; i < d; i++) {
        Donor x;
        int k;
        in >> x.id >> x.blood >> a >> k;
        x.hospital = getHospital(a);
        for (int j = 0; j < k; j++) {
            string organ;
            in >> organ;
            x.organs.push_back(organ);
        }
        x.processed = false;
        donors.push_back(x);
    }

    in >> label >> r;
    for (int i = 0; i < r; i++) {
        Recipient x;
        in >> x.id >> x.blood >> x.organ >> x.urgency >> a;
        x.hospital = getHospital(a);
        x.assigned = false;
        recipients.push_back(x);
    }
    return true;
}

string toLowerCase(string s) {
    for (int i = 0; i < (int)s.size(); i++) {
        if (s[i] >= 'A' && s[i] <= 'Z') s[i] = s[i] - 'A' + 'a';
    }
    return s;
}

string toUpperCase(string s) {
    for (int i = 0; i < (int)s.size(); i++) {
        if (s[i] >= 'a' && s[i] <= 'z') s[i] = s[i] - 'a' + 'A';
    }
    return s;
}

int readInt(string prompt, int lo, int hi) {
    while (true) {
        cout << prompt;
        string line;
        if (!getline(cin, line)) exit(0);
        stringstream ss(line);
        int value;
        string extra;
        if (ss >> value) {
            if (!(ss >> extra)) {
                if (value >= lo && value <= hi) return value;
            }
        }
        cout << "  Please enter a whole number from " << lo << " to " << hi << ".\n";
    }
}

string readOneWord(string prompt, bool &ok) {
    cout << prompt;
    string line;
    if (!getline(cin, line)) exit(0);
    stringstream ss(line);
    string word, extra;
    ok = false;
    if (ss >> word) {
        if (!(ss >> extra)) ok = true;
    }
    return word;
}

string readBloodGroup(string prompt) {
    while (true) {
        bool ok;
        string word = readOneWord(prompt, ok);
        if (ok) {
            word = toUpperCase(word);
            if (canDonateTo.count(word) > 0) return word;
        }
        cout << "  Not valid. Options: AB B A O \n";
    }
}

string readOrgan(string prompt) {
    while (true) {
        bool ok;
        string word = readOneWord(prompt, ok);
        if (ok) {
            word = toLowerCase(word);
            if (viabilityMinutes.count(word) > 0) return word;
        }
        cout << "  Not valid. Options: kidney liver lung heart \n";
    }
}

int pickHospital() {
    cout << "\n  Hospitals:\n";
    for (int i = 0; i < (int)hospitalName.size(); i++) {
        cout << "  " << setw(3) << i + 1 << ". " << hospitalName[i] << "\n";
    }
    int choice = readInt("  Hospital number: ", 1, hospitalName.size());
    return choice - 1;
}

bool compatible(Donor d, Recipient r) {
    vector<string> groups = canDonateTo[d.blood];
    for (int i = 0; i < (int)groups.size(); i++) {
        if (groups[i] == r.blood) return true;
    }
    return false;
}

vector<bool> bfsReachable(int src) {
    vector<bool> seen(road.size(), false);
    queue<int> q;
    q.push(src);
    seen[src] = true;
    while (!q.empty()) {
        int u = q.front();
        q.pop();
        for (int i = 0; i < (int)road[u].size(); i++) {
            int v = road[u][i].to;
            if (!seen[v]) {
                seen[v] = true;
                q.push(v);
            }
        }
    }
    return seen;
}

vector<int> dijkstra(int src, vector<int> &junc) {
    int n = road.size();
    vector<int> dist(n, INF);
    junc.assign(n, INF);
    priority_queue<pair<int, int>, vector<pair<int, int>>, greater<pair<int, int>>> pq;
    dist[src] = 0;
    junc[src] = 0;
    pq.push(make_pair(0, src));

    while (!pq.empty()) {
        int d = pq.top().first;
        int u = pq.top().second;
        pq.pop();
        if (d > dist[u]) continue;

        for (int i = 0; i < (int)road[u].size(); i++) {
            Edge e = road[u][i];
            int newDist = d + e.minutes;
            int newJunc = junc[u] + e.junctions;
            bool better = false;
            if (newDist < dist[e.to]) better = true;
            if (newDist == dist[e.to] && newJunc < junc[e.to]) better = true;
            if (better) {
                dist[e.to] = newDist;
                junc[e.to] = newJunc;
                pq.push(make_pair(newDist, e.to));
            }
        }
    }
    return dist;
}

Route greenCorridor(int src, int dst, int limitMinutes) {
    int T = limitMinutes / STEP;
    int n = road.size();

    vector<vector<int>> dp(T + 1, vector<int>(n, INF));
    vector<vector<int>> parentNode(T + 1, vector<int>(n, -1));
    vector<vector<int>> parentTime(T + 1, vector<int>(n, -1));
    vector<vector<int>> parentMinutes(T + 1, vector<int>(n, 0));
    dp[0][src] = 0;

    for (int t = 0; t <= T; t++) {
        for (int u = 0; u < n; u++) {
            if (dp[t][u] == INF) continue;
            for (int i = 0; i < (int)road[u].size(); i++) {
                Edge e = road[u][i];
                int steps = e.minutes / STEP;
                if (e.minutes % STEP != 0) steps++;
                int nt = t + steps;
                if (nt > T) continue;
                if (dp[t][u] + e.junctions < dp[nt][e.to]) {
                    dp[nt][e.to] = dp[t][u] + e.junctions;
                    parentNode[nt][e.to] = u;
                    parentTime[nt][e.to] = t;
                    parentMinutes[nt][e.to] = e.minutes;
                }
            }
        }
    }

    Route route;
    route.found = false;
    int bestT = -1;
    for (int t = 0; t <= T; t++) {
        if (dp[t][dst] == INF) continue;
        if (bestT == -1 || dp[t][dst] < dp[bestT][dst]) bestT = t;
    }
    if (bestT == -1) return route;

    route.found = true;
    route.junctions = dp[bestT][dst];
    route.minutes = 0;
    int node = dst;
    int t = bestT;
    route.path.push(node);
    while (!(node == src && t == 0)) {
        int prevNode = parentNode[t][node];
        int prevTime = parentTime[t][node];
        route.minutes += parentMinutes[t][node];
        node = prevNode;
        t = prevTime;
        route.path.push(node);
    }
    return route;
}

double score(Recipient r, int fastest, int limit) {
    int total = recipients.size();
    double waiting = 10.0 * (total - waitPos[r.id]) / total;
    double proximity = 10.0 * (1.0 - (double)fastest / limit);
    return 0.5 * r.urgency + 0.3 * waiting + 0.2 * proximity;
}

void sortOrgansByViability(vector<string> &organs) {
    int n = organs.size();
    for (int i = 0; i < n - 1; i++) {
        for (int j = 0; j < n - 1 - i; j++) {
            if (viabilityMinutes[organs[j]] > viabilityMinutes[organs[j + 1]]) {
                string temp = organs[j];
                organs[j] = organs[j + 1];
                organs[j + 1] = temp;
            }
        }
    }
}

void sortCandidatesHighestFirst(vector<pair<double, int>> &c) {
    int n = c.size();
    for (int i = 0; i < n - 1; i++) {
        for (int j = 0; j < n - 1 - i; j++) {
            if (c[j] < c[j + 1]) {
                pair<double, int> temp = c[j];
                c[j] = c[j + 1];
                c[j + 1] = temp;
            }
        }
    }
}

void allocateDonor(int d) {
    Donor &dn = donors[d];
    int src = dn.hospital;

    vector<int> compat;
    for (int r = 0; r < (int)recipients.size(); r++) {
        if (compatible(dn, recipients[r])) compat.push_back(r);
    }

    vector<string> organs = dn.organs;
    sortOrgansByViability(organs);

    cout << "\n============================================\n";
    cout << "Donor " << dn.id << " (" << dn.blood << ") at " << hospitalName[src]
         << " has " << organs.size() << " organs: ";
    for (int i = 0; i < (int)organs.size(); i++) {
        cout << organs[i];
        if (i + 1 < (int)organs.size()) cout << ", ";
        else cout << "\n";
    }

    vector<bool> reach = bfsReachable(src);
    vector<int> fastJunc;
    vector<int> fastest = dijkstra(src, fastJunc);
    int saved = 0;

    for (int k = 0; k < (int)organs.size(); k++) {
        string organ = organs[k];
        int limit = viabilityMinutes[organ];
        totalOrgans++;
        cout << "\n  [" << organ << "] viable for " << limit << " min\n";

        vector<pair<double, int>> candidates;
        for (int i = 0; i < (int)compat.size(); i++) {
            int r = compat[i];
            Recipient rc = recipients[r];
            if (rc.organ != organ) continue;
            if (rc.assigned) continue;
            if (!reach[rc.hospital]) continue;
            if (fastest[rc.hospital] > limit) continue;
            double s = score(rc, fastest[rc.hospital], limit);
            candidates.push_back(make_pair(s, r));
        }
        sortCandidatesHighestFirst(candidates);

        cout << "  Top candidates:\n";
        for (int i = 0; i < (int)candidates.size() && i < 3; i++) {
            Recipient rc = recipients[candidates[i].second];
            cout << "    " << rc.id << " (" << rc.blood << ", urgency " << rc.urgency << ") at "
                 << hospitalName[rc.hospital] << "  score = " << candidates[i].first << "\n";
        }

        bool done = false;
        for (int i = 0; i < (int)candidates.size(); i++) {
            Recipient &rc = recipients[candidates[i].second];
            Route route = greenCorridor(src, rc.hospital, limit);
            if (!route.found) continue;

            string path = "";
            while (!route.path.empty()) {
                path += hospitalName[route.path.top()];
                route.path.pop();
                if (!route.path.empty()) path += " -> ";
            }

            rc.assigned = true;
            rc.result = organ + " from donor " + dn.id + ", " + to_string(route.minutes) + " min, "
                      + to_string(route.junctions) + " junctions. Route: " + path;
            done = true;
            saved++;
            totalPlaced++;
            fastMinSum += fastest[rc.hospital];
            fastJuncSum += fastJunc[rc.hospital];
            dpMinSum += route.minutes;
            dpJuncSum += route.junctions;

            cout << "  -> Allocated to " << rc.id << " at " << hospitalName[rc.hospital] << "\n";
            cout << "     Fastest route (Dijkstra): " << fastest[rc.hospital] << " min, "
                 << fastJunc[rc.hospital] << " junctions to clear\n";
            cout << "     Green corridor (DP):      " << route.minutes << " min, "
                 << route.junctions << " junctions to clear\n";
            cout << "     Route: " << path << "\n";
            break;
        }
        if (!done) cout << "  -> No suitable recipient can be reached in time.\n";
    }

    dn.processed = true;
    cout << "\n  Donor " << dn.id << " saved " << saved << " of " << organs.size()
         << " possible lives.\n";
}

void showDonors() {
    cout << "\n  Donors:\n";
    for (int i = 0; i < (int)donors.size(); i++) {
        Donor dn = donors[i];
        cout << "  " << i + 1 << ". " << dn.id << " (" << dn.blood << ") at "
             << hospitalName[dn.hospital] << ": ";
        for (int j = 0; j < (int)dn.organs.size(); j++) {
            cout << dn.organs[j];
            if (j + 1 < (int)dn.organs.size()) cout << ", ";
        }
        if (dn.processed) cout << "   [allocated]\n";
        else cout << "   [waiting]\n";
    }
}

void runDonor() {
    showDonors();
    int d = readInt("  Donor number to allocate: ", 1, donors.size()) - 1;
    if (donors[d].processed) cout << "  This donor's organs have already been allocated.\n";
    else allocateDonor(d);
}

void addDonor() {
    Donor x;
    x.id = "D" + to_string(donors.size() + 1);
    cout << "\n  New donor " << x.id << "\n";
    x.blood = readBloodGroup("  Blood group (O, A, B, AB): ");
    x.hospital = pickHospital();
    int k = readInt("  How many organs (1-6)? ", 1, 6);
    for (int i = 0; i < k; i++) {
        string organ = readOrgan("  Organ " + to_string(i + 1) + " (heart, lung, liver, kidney): ");
        x.organs.push_back(organ);
    }
    x.processed = false;
    donors.push_back(x);
    cout << "  Donor " << x.id << " registered. Allocating now...\n";
    allocateDonor(donors.size() - 1);
}

void addPatient() {
    Recipient x;
    x.id = "R" + to_string(recipients.size() + 1);
    cout << "\n  New patient " << x.id << "\n";
    x.blood = readBloodGroup("  Blood group (O, A, B, AB): ");
    x.organ = readOrgan("  Organ needed (heart, lung, liver, kidney): ");
    x.urgency = readInt("  Urgency (1 = stable, 10 = critical): ", 1, 10);
    x.hospital = pickHospital();
    x.assigned = false;
    recipients.push_back(x);
    recipientIndex[x.id] = recipients.size() - 1;
    waitPos[x.id] = recipients.size() - 1;
    cout << "  Patient " << x.id << " added to the waiting list at position "
         << recipients.size() << ".\n";
}

void lookupPatient() {
    cout << "  Patient ID (for example R07): ";
    string line;
    if (!getline(cin, line)) exit(0);
    stringstream ss(line);
    string id;
    ss >> id;
    id = toUpperCase(id);
    if (recipientIndex.count(id) == 0) {
        cout << "  No patient with ID " << id << ".\n";
        return;
    }
    Recipient r = recipients[recipientIndex[id]];
    cout << "  " << r.id << ": needs " << r.organ << ", blood " << r.blood << ", urgency "
         << r.urgency << ", at " << hospitalName[r.hospital] << "\n";
    cout << "  Waiting-list position: " << waitPos[r.id] + 1 << " of " << recipients.size() << "\n";
    if (r.assigned) cout << "  Status: RECEIVED " << r.result << "\n";
    else cout << "  Status: still waiting\n";
}

void showSummary() {
    int waiting = 0;
    for (int i = 0; i < (int)recipients.size(); i++) {
        if (!recipients[i].assigned) waiting++;
    }
    cout << "\n  Organs placed: " << totalPlaced << " of " << totalOrgans << "\n";
    cout << "  Patients still waiting: " << waiting << " of " << recipients.size() << "\n";
    if (totalPlaced > 0) {
        cout << "  Fastest routes (Dijkstra):  " << fastMinSum << " min, " << fastJuncSum << " junctions in total\n";
        cout << "  Green corridors (DP):       " << dpMinSum << " min, " << dpJuncSum << " junctions in total\n";
    }
}

int main() {
    setupRules();
    if (!loadData("data.txt")) {
        cout << "Could not open data.txt\n";
        return 1;
    }

    queue<string> waitingList;
    for (int i = 0; i < (int)recipients.size(); i++) {
        waitingList.push(recipients[i].id);
    }
    int pos = 0;
    while (!waitingList.empty()) {
        waitPos[waitingList.front()] = pos;
        pos++;
        waitingList.pop();
    }
    for (int i = 0; i < (int)recipients.size(); i++) {
        recipientIndex[recipients[i].id] = i;
    }

    cout << fixed << setprecision(2);
    cout << "===== LifeLine: Green-Corridor Organ Planner =====\n";
    cout << "Loaded " << hospitalName.size() << " hospitals, " << donors.size()
         << " donors and " << recipients.size() << " patients.\n";

    while (true) {
        cout << "\n1. Show donors          2. Allocate a donor's organs\n";
        cout << "3. Register new donor   4. Register new patient\n";
        cout << "5. Look up a patient    6. Summary\n";
        cout << "0. Exit\n";
        int choice = readInt("Choose: ", 0, 6);
        if (choice == 0) break;
        if (choice == 1) showDonors();
        if (choice == 2) runDonor();
        if (choice == 3) addDonor();
        if (choice == 4) addPatient();
        if (choice == 5) lookupPatient();
        if (choice == 6) showSummary();
    }
    cout << "Goodbye.\n";
    return 0;
}
