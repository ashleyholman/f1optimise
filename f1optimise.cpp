#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
#include <map>
#include <unordered_map>

#define TRADE_TYPE_DRIVER 0
#define TRADE_TYPE_TEAM 1

using namespace std;

struct Tradeable {
  int id; // unique identifier
  string name;
  double avgPoints;
  double cost;
};

struct Trade {
  int type;
  int sell;
  int buy;
  float netCash;
  float netPoints;
};

// list of all drivers
vector<Tradeable> drivers;
// list of all teams
vector<Tradeable> teams;
// list of my drivers
int myDrivers[4];
// list of my teams
int myTeams[4];

// keep a counter of how many trades we examine.
int goodTradeCount = 0;
int allTradeCount = 0;

vector<Tradeable> loadTradeables(const char* fname) {
  int next_id=0;
  vector<Tradeable> tradeables;
  string line;
  ifstream ifs(fname);

  while (getline(ifs, line)) {
    stringstream ss(line);
    string name;
    double avgPoints;
    double cost;
    Tradeable t;

    getline(ss, t.name, ',');
    ss >> t.avgPoints;
    ss.ignore(1, ',');
    ss >> t.cost;
    t.id = next_id++;

    tradeables.push_back(t);
  }

  return tradeables;
}

// find the single best trade and return it in a vector
vector<Trade> findBestSingleTrade(vector<Trade> trades, double spareCash) {
  vector<Trade> bestTrades;
  Trade bestTrade;
  bool foundTrade = false;
  double highestPointsGain = 0;
  double bestCash;
  for (auto &t : trades) {
    if (spareCash + t.netCash > 0) {
      // this trade is possible
      allTradeCount++;
      if (t.netPoints > 0) {
        goodTradeCount++;
      }
      if (t.netPoints > highestPointsGain) {
        // this is our best looking trade so far
        bestTrade = t;
        foundTrade = true;
        highestPointsGain = t.netPoints;
        bestCash = t.netCash;
      } else if (t.netPoints == highestPointsGain && t.netCash > bestCash) {
        // this trade is equally as good as our best in terms of points, but cheaper.  so it is our new best.
        bestTrade = t;
        bestCash = t.netCash;
      }
    }
  }

  if (foundTrade) {
    bestTrades.push_back(bestTrade);
  }

  return bestTrades;
}

// find the best double trade and return both trades in a vector
vector<Trade> findBestDoubleTrade(vector<Trade> trades, double spareCash) {
  vector<Trade> bestTrades;
  Trade bestTrade1;
  Trade bestTrade2;
  bool foundTrade = false;
  double highestPointsGain = 0;
  double bestCash;

  // work out how many of each team we own, so we know how many we can
  // possibly trade into (limit is 2 of each team).
  vector<int> numTeamsOwned(teams.size());

  for (auto &t : teams) {
    for (int i = 0; i < 4; i++) {
      if (myTeams[i] == t.id) {
        numTeamsOwned[myTeams[i]]++;
      }
    }
  }

  // begin brute force
  for (int i = 0; i < trades.size(); i++) {
    for (int j = 0; j < trades.size(); j++) {
      // examine trade pair trades[i] and trades[j]
      double netPoints = trades[i].netPoints + trades[j].netPoints;
      double netCash = trades[i].netCash + trades[j].netCash;
      allTradeCount++;
      if (spareCash + netCash < 0) {
        // we don't have enough money to execute this trade pair
        continue;
      }
      if (netPoints <= 0) {
        // no point in this pair as it gains us no points
        continue;
      }
      // sanity checks for invalid trade combos
      if (   trades[i].type == TRADE_TYPE_DRIVER && trades[j].type == TRADE_TYPE_DRIVER
          && (trades[i].sell == trades[j].sell || trades[i].buy == trades[j].buy)) {
        // can't buy or sell the same driver twice.
        continue;
      }
      if (   trades[i].type == TRADE_TYPE_TEAM && trades[j].type == TRADE_TYPE_TEAM
          && trades[i].sell == trades[j].sell && numTeamsOwned[trades[i].sell] < 2) {
        // can't sell the same team twice if we only own 1 of them.
        continue;
      }
      if (   trades[i].type == TRADE_TYPE_TEAM && trades[j].type == TRADE_TYPE_TEAM
          && trades[i].buy == trades[j].buy && numTeamsOwned[trades[i].buy] > 0) {
        // can't buy the same team twice if we already own 1 or more of them.
        continue;
      }

      // now we are left with only valid trades we can afford that end in a net gain in average points
      // potentially we might be interested in reviewing all of these - but just return the best for now.
      goodTradeCount++;
      if (netPoints > highestPointsGain) {
        // we've found a better trade pair
        foundTrade = true;
        highestPointsGain = netPoints;
        bestTrade1 = trades[i];
        bestTrade2 = trades[j];
        bestCash = netCash;
      } else if (netPoints == highestPointsGain && netCash > bestCash) {
        // this trade equals our best in terms of points but nets us more cash.. so it's our new best.
        bestTrade1 = trades[i];
        bestTrade2 = trades[j];
        bestCash = netCash;
      }
      /*
      cout << "POSITIVE Trade Pair NET POINTS = " << netPoints << ", NET CASH = " << netCash << endl;
      if (trades[i].type == TRADE_TYPE_DRIVER) {
        cout << "TRADE 1: Driver " << drivers[trades[i].sell].name << " for " << drivers[trades[i].buy].name << endl;
      } else {
        cout << "TRADE 1: Team " << teams[trades[i].sell].name << " for " << teams[trades[i].buy].name << endl;
      }
      if (trades[j].type == TRADE_TYPE_DRIVER) {
        cout << "TRADE 2: Driver " << drivers[trades[j].sell].name << " for " << drivers[trades[j].buy].name << endl;
      } else {
        cout << "TRADE 2: Team " << teams[trades[j].sell].name << " for " << teams[trades[j].buy].name << endl;
      }
      */
    }
  }

  if (foundTrade) {
    bestTrades.push_back(bestTrade1);
    bestTrades.push_back(bestTrade2);
  }

  return bestTrades;
}

void findOptimalTrade(vector<int>avDrivers, vector<int>avTeams, double spareCash, bool singleTradeOnly) {
  vector<Trade> trades;
  vector<Trade> bestSingleTrade;
  vector<Trade> bestDoubleTrade;

  // iterate over all possible driver trades
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < avDrivers.size(); j++) {
      Trade t;
      t.type = TRADE_TYPE_DRIVER;
      t.sell = myDrivers[i];
      t.buy = avDrivers[j];
      t.netCash = drivers[t.sell].cost - drivers[t.buy].cost;
      t.netPoints= drivers[t.buy].avgPoints - drivers[t.sell].avgPoints;
      if (t.netCash <= 0 && t.netPoints <= 0) {
        // this trade doesn't gain us cash OR points. thus any trade sequence
        // involving this trade would be better off without it.  therefore we don't even
        // need to consider this as a possible trade.
        continue;
      }
      trades.push_back(t);
    }
  }

  // iterate over all possible team trades
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < avTeams.size(); j++) {
      Trade t;
      t.type = TRADE_TYPE_TEAM;
      t.sell = myTeams[i];
      t.buy = avTeams[j];
      t.netCash = teams[t.sell].cost - teams[t.buy].cost;
      t.netPoints= teams[t.buy].avgPoints - teams[t.sell].avgPoints;
      if (t.netCash <= 0 && t.netPoints <= 0) {
        // as with drivers, don't even consider these valueless trades
        continue;
      }
      trades.push_back(t);
    }
  }

  bestSingleTrade = findBestSingleTrade(trades, spareCash);
  if (!singleTradeOnly) {
    bestDoubleTrade = findBestDoubleTrade(trades, spareCash);
  }
  // work out if our best trade is the single or double one
  vector<Trade> bestTrades;
  if (bestSingleTrade.size() == 0) {
    bestTrades = bestDoubleTrade;
  } else if (bestDoubleTrade.size() == 0) {
    bestTrades = bestDoubleTrade;
  } else if (bestSingleTrade[0].netPoints > (bestDoubleTrade[0].netPoints + bestDoubleTrade[1].netPoints)) {
    bestTrades = bestSingleTrade;
  } else {
    bestTrades = bestDoubleTrade;
  }

  float overallPoints = 0;
  float overallCash = 0;

  cout << "Examined " << allTradeCount << " possible trades (" << goodTradeCount
       << " of which would result in net points gains)." << endl;
  if (bestTrades.size()) {
    for (auto &bt : bestTrades) {
      if (bt.type == TRADE_TYPE_DRIVER) {
        cout << "Trade " << drivers[bt.sell].name << " for " << drivers[bt.buy].name << " NET CASH = $" << bt.netCash
             << "M, NET POINTS = " << bt.netPoints << endl;
      } else {
        cout << "Trade " << teams[bt.sell].name << " for " << teams[bt.buy].name << " NET CASH = $" << bt.netCash
             << "M, NET POINTS = " << bt.netPoints << endl;
      }
      overallPoints += bt.netPoints;
      overallCash += bt.netCash;
    }

    cout << "Overall net points change: +" << overallPoints << endl;
    cout << "Overall net cash change: $" << overallCash << "M" << endl;
    printf("Final cash balance after trade: $%.1fM", overallCash + spareCash);
    cout << endl;
  } else {
    cout << "Optimal choice is to NOT trade, as all possible trades would result in a loss in average points." << endl;
  }
}

/* returns available items to trade based on current inventory. ie, drivers or teams you can currently trade into
   (regardless of price constraints).  maxOwnership should be set to 1 for drivers (can only own 1),
   and 2 for teams (can own 2 of each team) */
vector<int> getAvailableTradeables(vector<Tradeable> tradeables, int inventory[4], int maxOwnership) {
  vector<int> avail;

  for (auto &t : tradeables) {
    int haveAlready = 0;
    for (int i = 0; i < 4; i++) {
      if (t.id == inventory[i]) {
        haveAlready++;
      }
    }
    if (haveAlready < maxOwnership) {
      avail.push_back(t.id);
    }
  }

  return avail;
}

int main(int argc, char *argv[]) {
  bool single = false;
  if (argc > 2 && strcmp(argv[1], "--single") == 0) {
    single = true;
    --argc;
    ++argv;
  }

  if (argc != 10) {
    cerr << "Usage: f1optimise [--single] <RemainingCapM> <Driver1> <Driver2> <Driver3> <Driver4> <Team1> <Team2> <Team3> <Team4>" << endl;
    exit(1);
  }

  double spareCash = atof(argv[1]);

  drivers = loadTradeables("drivers.csv");
  teams = loadTradeables("teams.csv");

  unordered_map<string, int> driversByName;
  unordered_map<string, int> teamsByName;

  for (auto &d : drivers) {
    driversByName[d.name] = d.id;
  }
  for (auto &t : teams) {
    teamsByName[t.name] = t.id;
  }

  unordered_map<string, int>::iterator it;
  for (int i = 0; i < 4; i++) {
    if ((it = driversByName.find(argv[i+2])) != driversByName.end()) {
      myDrivers[i] = drivers[it->second].id;
    } else {
      cerr << "Unknown driver (no match in drivers.csv): \""<< argv[i+2] << "\"" << endl;
      exit(1);
    }

    if ((it = teamsByName.find(argv[i+6])) != teamsByName.end()) {
      myTeams[i] = teams[it->second].id;
    } else {
      cerr << "Unknown team (no match in teams.csv): \""<< argv[i+6] << "\"" << endl;
      exit(1);
    }
  }

  // construct "available" teams & drivers based on what's not in your team
  vector<int> avDrivers = getAvailableTradeables(drivers, myDrivers, 1);
  vector<int> avTeams = getAvailableTradeables(teams, myTeams, 2);

  findOptimalTrade(avDrivers, avTeams, spareCash, single);
}
