#ifndef SCOUT_H
#define SCOUT_H

#include "input_data.h"

std::map<Team,Robot_capabilities> parse_csv(std::string const&,bool verbose=0);
void team_details(std::string const&,Team const&);

#endif
