#ifndef INPUT_DATA_H
#define INPUT_DATA_H

#include "int_limited.h"
#include "../tba/data.h"
#include "map_fixed.h"
#include "flat_map.h"

using Team=Int_limited<1,10*1000>;

using Endgame=tba::Endgame_2022;

#ifdef __CYGWIN__
std::ostream& operator<<(std::ostream&,Endgame const&);
#endif

std::vector<Endgame> endgames();
double value(Endgame);

using Px=double;//probability so should be from 0-1

using Endgame_px=std::map<Endgame,Px>;

using Starting_location=Int_limited<1,4>;

using Auto=Flat_map<Starting_location,double>;
//using Auto=Map_fixed<Starting_location,double,4>;

#define ROBOT_CAPABILITIES_ITEMS(X)\
	X(Auto,auto_pts)\
	X(double,tele_ball_pts)\
	X(Endgame_px,endgame)

struct Robot_capabilities{
	//auto
		//average pts
		//then eventually have data about where they start the match...
		//initially no strategy in this section, eventually have it work out starting locations for each robot
	//tele balls
		//average high
		//average low
		//strategy is either go for balls or go for defense
	//endgame
		//going to assume that they don't interfere as long as don't have more than 2 at the same level
		//strategies are either try to climb or not for each robot and you get what you get
	#define X(A,B) A B;
	ROBOT_CAPABILITIES_ITEMS(X)
	#undef X

	auto operator<=>(Robot_capabilities const&)const=default;
};
std::ostream& operator<<(std::ostream&,Robot_capabilities const&);

Robot_capabilities operator+(Robot_capabilities,Robot_capabilities);
Robot_capabilities operator/(Robot_capabilities,int);

#endif
