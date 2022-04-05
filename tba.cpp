#include "tba.h"
#include<fstream>
#include "../tba/data.h"
#include "../tba/db.h"
#include "../tba/tba.h"
#include "opr.h"
#include "map.h"
#include "scout.h"

using namespace std;

std::ostream& operator<<(std::ostream& o,TBA_explicit const& a){
	o<<"TBA_explicit( ";
	#define X(A,B) o<<""#B<<":"<<a.B<<" ";
	TBA_EXPLICIT(X)
	#undef X
	return o<<")";
}

std::ostream& operator<<(std::ostream& o,tba::Yes_no a){
	if(a==tba::Yes_no::Yes){
		return o<<"Yes";
	}
	return o<<"No";
}

template<typename T>
multiset<T> operator-(multiset<T> a,T t){
	//if you just do a.erase(t) you will remove all instances, not just one.
	auto f=a.find(t);
	if(f!=a.end()){
		a.erase(f);
	}
	return a;
}

using Event_key=tba::Event_key;

Team team_number(tba::Team_key const& a){
	//return Team{stoi(a.str().substr(3,10))};
	return Team{atoi(a.str().c_str()+3)};
}

tba::Cached_fetcher tba_fetcher(TBA_setup const& tba){
	ifstream file{tba.auth_key_path};
	string tba_key;
	getline(file,tba_key);
	return tba::Cached_fetcher{tba::Fetcher{tba::Nonempty_string{tba_key}},tba::Cache{tba.cache_path.c_str()}};
}

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
	
	map<Team,vector<pair<Endgame,vector<Endgame>>>> endgame;
	
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
			endgame[t]|=make_pair(e,to_vec(to_multiset(endgames)-e));
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
					static const double discount_rate=0.07;
					return count_endgames(endgame[team],discount_rate);
					/*map<Endgame,double> r;
					auto e=endgame[team];
					for(auto x:e){
						r[x]=(0.0+e.count(x))/e.size();
					}
					return r;*/
				}(),
				30,
				1
			});
		},
		keys(taxi_by_team)
	));
}

variant<map<Team,Robot_capabilities>,string> from_tba(
	TBA_setup const& setup,
	tba::Event_key const& event_key
){
	auto f=tba_fetcher(setup);

	try{
		return process_data(f,event_key);
	}catch(tba::Decode_error const& a){
		return as_string(a);
	}
}

template<typename T>
std::array<T,3> dup3(T t){
	return {t,t,t};
}

template<typename A,typename B,typename C,typename D,typename E>
auto zip(A const& a,B const& b,C const& c,D const& d,E const& e){
	auto ai=begin(a),ae=end(a);
	auto bi=begin(b),be=end(b);
	auto ci=begin(c),ce=end(c);
	auto di=begin(d),de=end(d);
	auto ei=begin(e),ee=end(e);

	vector<tuple<
		ELEM(a),ELEM(b),ELEM(c),ELEM(d),ELEM(e)
	>> r;
	while(ai!=ae && bi!=be && ci!=ce && di!=de && ei!=ee){
		r|=make_tuple(*ai,*bi,*ci,*di,*ei);
		++ai;
		++bi;
		++ci;
		++di;
		++ei;
	}
	return r;
}

vector<TBA_explicit> process_match(tba::Match const& match){
	if(match.comp_level!=tba::Competition_level::qm){
		return {};
	}

	auto f=[=](tba::Alliance_color color,tba::Match_Alliance const& ma,tba::Match_Score_Breakdown_2022_Alliance const& b){
		auto z=zip(
			ma.team_keys,
			dup3(match.match_number),
			dup3(color),
			array{b.taxiRobot1,b.taxiRobot2,b.taxiRobot3},
			array{b.endgameRobot1,b.endgameRobot2,b.endgameRobot3}
		);
		return mapf(
			[](auto x){
				auto [a,b,c,d,e]=x;
				return TBA_explicit{team_number(a),b,c,d==tba::Yes_no::Yes,e}; 
			},
			z
		);
	};
	auto m=match.score_breakdown;
	assert(m);
	auto m1=*m;
	using BD=tba::Match_Score_Breakdown_2022;
	assert(std::holds_alternative<BD>(m1));
	auto bd=std::get<BD>(m1);
	//cout<<"m "<<match.match_number<<"\t"<<match.alliances.red.team_keys<<"\t"<<match.alliances.blue.team_keys<<"\n";
	return f(tba::Alliance_color::RED,match.alliances.red,bd.red)|
		f(tba::Alliance_color::BLUE,match.alliances.blue,bd.blue);
}

std::vector<TBA_explicit> tba_by_match(TBA_setup const& setup,tba::Event_key const& event){
	(void)setup;
	(void)event;
	auto f=tba_fetcher(setup);
	return flatten(mapf(process_match,event_matches(f,event)));
}
