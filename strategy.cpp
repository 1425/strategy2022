#include "strategy.h"
#include<array>
#include "input_data.h"
#include "map.h"
#include "array.h"

using namespace std;

template<typename T>
T max_else(vector<T> const& v,T const& t){
	if(v.empty()){
		return t;
	}
	return max(v);
}

std::ostream& operator<<(std::ostream& o,Robot_strategy const& a){
	o<<"Robot_strategy(";
	#define X(A,B) o<<" "#B<<":"<<a.B;
	ROBOT_STRATEGY_ITEMS(X)
	#undef X
	return o<<" )";
}

#define ROBOT_TELEOP_STRATEGY_ITEMS(X)\
	X(bool,defend)\
	X(bool,climb)

struct Robot_teleop_strategy{
	ROBOT_TELEOP_STRATEGY_ITEMS(INST)

	auto operator<=>(Robot_teleop_strategy const&)const=default;
};

std::ostream& operator<<(std::ostream& o,Robot_teleop_strategy const& a){
	o<<"Robot_teleop_strategy(";
	#define X(A,B) o<<" "#B<<":"<<a.B;
	ROBOT_TELEOP_STRATEGY_ITEMS(X)
	#undef X
	return o<<")";
}

vector<Robot_teleop_strategy> robot_teleop_strategies(){
	vector<Robot_teleop_strategy> r;
	for(auto a:range(2)){
		for(auto b:range(2)){
			r|=Robot_teleop_strategy{bool(a),bool(b)};
		}
	}
	return r;
}

using Alliance_teleop_strategy=std::array<Robot_teleop_strategy,3>;

vector<Alliance_teleop_strategy> alliance_teleop_strategies(){
	vector<Alliance_teleop_strategy> r;
	for(auto a:robot_teleop_strategies()){
		for(auto b:robot_teleop_strategies()){
			for(auto c:robot_teleop_strategies()){
				r|=Alliance_teleop_strategy{a,b,c};
			}
		}
	}
	return r;
}

double points(Endgame_px const& e){
	auto r=0;
	for(auto [k,v]:e){
		r+=value(k)*v;
	}
	return r;
}

double climb_score(Alliance_capabilities const& a,bool c0,bool c1,bool c2){
	vector<Endgame_px> v;
	if(c0) v|=a[0].endgame;
	if(c1) v|=a[1].endgame;
	if(c2) v|=a[2].endgame;
	if(v.empty()) return 0;
	v=reversed(sort_by(v,[](auto x){ return points(x); }));
	map<Endgame,int> m;
	auto available=[&](Endgame g){
		return m[g]<2;
	};
	auto mark=[&](Endgame g){
		switch(g){
			case Endgame::Traversal:
				m[Endgame::Traversal]++;
				m[Endgame::High]++;
				break;
			case Endgame::High:
				m[Endgame::Traversal]++;
				m[Endgame::High]++;
				m[Endgame::Mid]++;
				break;
			case Endgame::Mid:
				m[Endgame::High]++;
				m[Endgame::Mid]++;
				m[Endgame::Low]++;
				break;
			case Endgame::Low:
				m[Endgame::Low]++;
				break;
			case Endgame::None:
				//plenty of room on the floor
				break;
			default:
				assert(0);
		}
	};

	double total=0;
	for(auto endgame_px:v){
		auto f=filter_keys(available,endgame_px);
		if(f.empty()) continue;
		auto target=argmax(value,keys(f));
		mark(target);
		auto left=sum(values(f));
		assert(left>0);
		//cout<<"end "<<endgame_px<<"\t"<<left<<"\n";
		//PRINT(endgame_px/left);
		total+=points(endgame_px);
	}
	//cout<<"climb:"<<total<<"\n";
	return total;
}

double teleop_score(Alliance_capabilities const& cap,Alliance_teleop_strategy const& strat){
	std::array<double,3> defense_values{10,5,2}; //these could be revised at some point.
	double defenders=0;
	double r=0;
	for(auto [cap1,strat1]:zip(cap,strat)){
		if(strat1.defend){
			if(cap1.defense_okay){
				r+=defense_values[defenders++];
				if(cap1.climb_time<20 || !strat1.climb){
					//assume can do 0 extra pt of defense if fast climb
					//or not going to climb.
					r++;
				}
				
			}
		}else{
			auto time_multiplier=[=](){
				static const double TELEOP_TIME=60*2+15;
				static const auto NOT_END=TELEOP_TIME-30;
				if(strat1.climb){
					return (TELEOP_TIME-cap1.climb_time)/NOT_END;
				}
				return TELEOP_TIME/NOT_END;
			}();
			r+=cap1.tele_ball_pts*time_multiplier;
		}
	}
	//PRINT(r);
	return 0.9*r+climb_score(cap,strat[0].climb,strat[1].climb,strat[2].climb);
}

pair<double,Alliance_teleop_strategy> teleop_score(Alliance_capabilities const& a){
	return max(mapf(
		[=](auto x){ return make_pair(teleop_score(a,x),x); },
		alliance_teleop_strategies()
	));
}

vector<Robot_strategy> robot_strategies(){
	vector<Robot_strategy> r;
	for(auto starting_location:options((Starting_location*)0)){
		for(auto x:range(2)){
			for(auto y:range(2)){
				r|=Robot_strategy{starting_location,bool(x),bool(y)};
			}
		}
	}
	return r;
}

using Auto_strategy=std::array<Starting_location,3>;

vector<Auto_strategy> auto_strategies(){
	//going to make it so that no more than 1 robot is in locations 3&4.
	//because those are likely to conflict
	vector<Auto_strategy> r;
	for(auto a:options((Starting_location*)0)){
		auto available=to_set(options((Starting_location*)0))-a;
		if(a==3) available-=4;
		if(a==4) available-=3;
		for(auto b:available){
			auto av2=available-b;
			if(b==3) av2-=4;
			if(b==4) av2-=3;
			for(auto c:av2){
				r|=Auto_strategy{a,b,c};
			}
		}
	}
	return r;
}

double points(Alliance_capabilities const& cap,Auto_strategy const& strat){
	return sum(mapf(
		[](auto p){
			auto [cap1,strat1]=p;
			return cap1.auto_pts[strat1];
		},
		zip(cap,strat)
	));
}

pair<double,Auto_strategy> auto_points(Alliance_capabilities const& cap){
	return max(mapf(
		[=](auto x){ return make_pair(points(cap,x),x); },
		auto_strategies()
	));
}

vector<Alliance_strategy> alliance_strategies(){
	vector<Alliance_strategy> r;
	for(auto a:robot_strategies()){
		for(auto b:robot_strategies()){
			for(auto c:robot_strategies()){
				r|=Alliance_strategy{a,b,c};
			}
		}
	}
	return r;
}

double points(Robot_capabilities const& a){
	double r=max_else(values(a.auto_pts),0.0)+a.tele_ball_pts;
	for(auto [type,p]:a.endgame){
		r+=value(type)*p;
	}
	return r;
}

double expected_score(Alliance_capabilities a,Alliance_strategy){
	//obviously, this should become more sophisticated, and actually look at what strategy was passed in.
	return points(sum(a));
}

pair<double,Alliance_strategy> expected_score(Alliance_capabilities const& a){
	//return auto_points(a)+teleop_score(a);
	auto ap=auto_points(a);
	//PRINT(ap);
	auto b=teleop_score(a);
	//PRINT(b);
	return make_pair(
		ap.first+b.first,
		mapf(
			[=](auto x){
				auto [auto1,tele1]=x;
				return Robot_strategy{
					auto1,
					tele1.defend,
					tele1.climb
				};
			},
			zip(ap.second,b.second)
		)
	);
}

variant<Picklist,string> make_picklist(Team const& base_team,map<Team,Robot_capabilities> cap){
	//first, figure out who's a good pick with no third robot
	auto f=cap.find(base_team);
	if(f==cap.end()){
		return "Error: Team not found";
	}
	auto base_cap=cap[base_team];
	auto other_teams=keys(cap)-base_team;

	auto first_only=reversed(sorted(mapf(
		[&](auto team){
			return make_pair(
				expected_score(Alliance_capabilities{base_cap,cap[team],Robot_capabilities{}}).first,
				team
			);
		},
		other_teams
	)));

	/*cout<<"First pass:\n";
	print_lines(first_only);*/

	//then, estimate what a third robot would look like

	auto est_third_robots=[&](){
		if(first_only.size()>5){
			return take(
				5,
				skip(
					min(20,cap.size()-5),
					first_only
				)
			);
		}
		return first_only;
	}();

	auto generic_third=[&](){
		if(est_third_robots.empty()){
			//this should only happen if there is only one robot worth of data available.
			return Robot_capabilities{};
		}
		return mean(mapf([&](auto x){ return cap[x.second]; },est_third_robots));
	}();
	PRINT(generic_third);

	//then redo the estimate of who's a good first pick
	auto first_picks=reversed(sorted(mapf(
		[&](auto team){
			return make_pair(
				expected_score(Alliance_capabilities{base_cap,cap[team],generic_third}).first,
				team
			);
		},
		other_teams
	)));

	//and then figure our all the second picks from that first pick list.
	return Picklist(
		base_team,
		mapf(
			[&](auto x){
				auto [value,team]=x;
				auto team_cap=cap[team];
				auto remaining=other_teams-team;
				auto m=reversed(sorted(mapf(
					[&](auto x){
						return make_pair(
							expected_score(Alliance_capabilities{base_cap,team_cap,cap[x]}).first,
							x
						);
					},
					remaining
				)));
				return make_tuple(team,value,swap_pairs(m));
			},
			take(25,first_picks) //note: if rank badly, this needs to go higher than 15.
		)
	);
}

