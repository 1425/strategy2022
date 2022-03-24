#include "tba.h"
#include "../tba/data.h"
#include "../tba/db.h"
#include "../tba/tba.h"
#include "opr.h"

using namespace std;

using Event_key=tba::Event_key;

map<Team,double> solve1(vector<pair<vector<Team>,int>> const& a){
	map<Team,vector<double>> m;
	for(auto [teams,value]:a){
		for(auto team:teams){
			m[team]|=double(value)/teams.size();
		}
	}
	return map_values([](auto x){ return mean(x); },m);
}

template<typename F>
map<Team,Robot_capabilities> process_data(F& fetcher,Event_key key){
	//initially, just going to look at a single event
	//Event_key key{"2022orwil"};//TODO: Make this be passed in.
	map<Team,vector<bool>> taxi_by_team;

	using To_solve=vector<pair<vector<Team>,int>>;
	To_solve auto_cargo_upper,auto_cargo_lower;
	To_solve tele_cargo_upper,tele_cargo_lower;
	
	map<Team,multiset<Endgame>> endgame;
	
	auto process_alliance=[&](tba::Match_Alliance alliance,tba::Match_Score_Breakdown_2022_Alliance scores){
		auto teams=mapf(to_team,alliance.team_keys|alliance.surrogate_team_keys);
		vector<bool> taxi;
		auto t=[&](auto x){
			taxi|=(x==tba::Yes_no::Yes);
		};
		t(scores.taxiRobot1);
		t(scores.taxiRobot2);
		t(scores.taxiRobot3);
		for(auto [team,taxi1]:zip(teams,taxi)){
			//obviously it would be better to do partial OPR to get balls up.
			//auto_by_team|=taxi+scores.autoCargoPoints/3;
			taxi_by_team[team]|=taxi1;
			
		}

		auto_cargo_upper|=make_pair(teams,scores.autoCargoUpperNear+scores.autoCargoUpperFar+scores.autoCargoUpperRed+scores.autoCargoUpperBlue);
		auto_cargo_lower|=make_pair(teams,scores.autoCargoLowerNear+scores.autoCargoLowerFar+scores.autoCargoLowerRed+scores.autoCargoLowerBlue);

		tele_cargo_upper|=make_pair(
			teams,
			scores.teleopCargoUpperNear+scores.teleopCargoUpperFar+scores.teleopCargoUpperRed+scores.teleopCargoUpperBlue
		);

		tele_cargo_lower|=make_pair(
			teams,
			scores.teleopCargoUpperNear+scores.teleopCargoUpperFar+scores.teleopCargoUpperRed+scores.teleopCargoUpperBlue
		);

		vector<Endgame> endgames;
		auto e=[&](auto x){ endgames|=x; };
		e(scores.endgameRobot1);
		e(scores.endgameRobot2);
		e(scores.endgameRobot3);
		for(auto [t,e]:zip(teams,endgames)){
			endgame[t]|=e;
		}
	};

	for(auto match:event_matches(fetcher,key)){
		//PRINT(match)
		auto m=match.score_breakdown;
		assert(m);
		auto s=*m;
		if(!std::holds_alternative<tba::Match_Score_Breakdown_2022>(s)){
			PRINT(match.key);
			PRINT(s);
			nyi
		} else{
			auto scores=std::get<tba::Match_Score_Breakdown_2022>(s);
			process_alliance(match.alliances.red,scores.red);
			process_alliance(match.alliances.blue,scores.blue);
		
		}
		//assert(std::holds_alternative<tba::Match_Score_Breakdown_2022>(s));
	}

	auto avg_auto_cargo_upper=solve1(auto_cargo_upper);
	auto avg_auto_cargo_lower=solve1(auto_cargo_lower);
	auto avg_tele_cargo_upper=solve1(tele_cargo_upper);
	auto avg_tele_cargo_lower=solve1(tele_cargo_lower);

	return to_map(mapf(
		[&](auto team){
			return make_pair(team,Robot_capabilities{
				[&](){
					auto v=mean(taxi_by_team[team])*2+avg_auto_cargo_upper[team]*4+avg_auto_cargo_lower[team]*2;
					Auto r;
					//No data about starting location, therefore just write it as the same for each.
					//obviously, would be better if did something like assume high scores are from locations 3/4.
					for(auto x:options((Starting_location*)0)){
						r[x]=v;
					}
					return r;
				}(),
				avg_tele_cargo_upper[team]+avg_tele_cargo_lower[team],
				[&](){
					map<Endgame,double> r;
					auto e=endgame[team];
					for(auto x:e){
						r[x]=(0.0+e.count(x))/e.size();
					}
					return r;
				}(),
				30,
				1
			});
		},
		keys(taxi_by_team)
	));
}

variant<map<Team,Robot_capabilities>,string> from_tba(
	string const& tba_auth_key_path,
	string const& tba_cache_path,
	tba::Event_key const& event_key
){
	ifstream file{tba_auth_key_path};
	string tba_key;
	getline(file,tba_key);
	tba::Cached_fetcher f{tba::Fetcher{tba::Nonempty_string{tba_key}},tba::Cache{tba_cache_path.c_str()}};

	try{
		return process_data(f,event_key);
	}catch(tba::Decode_error const& a){
		return as_string(a);
	}
}

