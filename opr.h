#ifndef OPR_H
#define OPR_H

#include "input_data.h"
#include "../tba/data.h"

Team to_team(tba::Team_key const&);
std::map<Team,double> solve(std::vector<std::pair<std::vector<Team>,int>> const&);
std::map<Team,double> solve(std::vector<std::tuple<std::vector<Team>,std::vector<Team>,int>> const&);

#endif
