#ifndef SCOUT_H
#define SCOUT_H

#include "input_data.h"

std::map<Team,Robot_capabilities> parse_csv(std::string const&,bool verbose=0);
void team_details(std::string const&,Team const&);
int parse_pit_scouting(std::string const&);

#define USEFUL_ITEMS(X)\
	X(Team,team,TeamNumber)\
	X(int,match,MatchNum)\
	X(tba::Alliance_color,alliance,Alliance)\
	X(bool,taxi,auto__tarmac)\
	X(Starting_location,start_position,auto__starting_position)\
	X(unsigned,auto_high,auto__highhub)\
	X(unsigned,auto_low,auto__lowhub)\
	X(unsigned,tele_high,teleop__highhub)\
	X(unsigned,tele_low,teleop__lowhub)\
	X(Endgame,endgame,endgame__climb_position)\
	X(unsigned,climbtime,endgame__climbtime)

struct Useful_data{
	#define X(A,B,C) A B;
	USEFUL_ITEMS(X)
	#undef X
};

std::ostream& operator<<(std::ostream&,Useful_data const&);

std::vector<Useful_data> parse_csv_inner(std::string const&,bool);
Robot_capabilities to_robot_capabilities(std::vector<Useful_data> const& data,double discount_rate=0,std::vector<Useful_data> const& extra_data={});
std::map<Team,Robot_capabilities> capabilities_by_team(std::vector<Useful_data> const&,double discount_rate=0);

double std_dev(std::vector<double> const&);
Endgame_px count_endgames(std::vector<std::pair<Endgame,std::vector<Endgame>>> const&,double);

#endif
