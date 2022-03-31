#ifndef STRATEGY_H
#define STRATEGY_H

#include "input_data.h"

using Alliance_capabilities=std::array<Robot_capabilities,3>;

#define ROBOT_STRATEGY_ITEMS(X)\
	X(Starting_location,starting_location)\
	X(bool,defend)\
	X(bool,climb)

struct Robot_strategy{
	ROBOT_STRATEGY_ITEMS(INST)
};

std::ostream& operator<<(std::ostream&,Robot_strategy const&);

using Alliance_strategy=std::array<Robot_strategy,3>;

std::pair<double,Alliance_strategy> expected_score(Alliance_capabilities const&);

double points(Endgame_px const&);
double points(Robot_capabilities const&);

using Picklist_row=std::tuple<Team,double,std::vector<std::pair<Team,double>>>;
using Picklist=std::pair<Team,std::vector<Picklist_row>>;

std::variant<Picklist,std::string> make_picklist(Team const&,std::map<Team,Robot_capabilities>);

#endif
