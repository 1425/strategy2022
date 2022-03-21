#include<set>
#include<iomanip>
#include "valor.h"
#include "tba.h"
#include "scout.h"

//start generic code

template<typename K,typename V>
std::map<K,V> operator/(std::map<K,V> m,double d){
	for(auto &a:m){
		a.second/=d;
	}
	return m;
}

template<typename T>
std::set<T>& operator|=(std::set<T>& a,std::set<T> const& b){
	a.insert(b.begin(),b.end());
	return a;
}

template<typename T>
auto or_all(std::vector<T> const& v){
	T r{};
	for(auto elem:v){
		r|=elem;
	}
	return r;
}

template<typename T>
std::set<T> operator-(std::set<T> a,T t){
	a.erase(t);
	return a;
}

template<typename T>
std::set<T> operator-(std::set<T> a,std::set<T> const& b){
	a-=b;
	return a;
}

template<typename T>
std::set<T> operator&(std::set<T> const& a,std::set<T> const& b){
	std::set<T> r;
	set_intersection(
		a.begin(),a.end(),
		b.begin(),b.end(),
		std::inserter(r,r.begin())
	);
	return r;
}

template<typename A,typename B,typename C>
bool operator<(std::tuple<A,B,C> const& a,std::tuple<A,B,C> const& b){
	if(std::get<0>(a)<std::get<0>(b)) return 1;
	if(std::get<0>(b)<std::get<0>(a)) return 0;

	if(std::get<1>(a)<std::get<1>(b)) return 1;
	if(std::get<1>(b)<std::get<1>(a)) return 0;
	
	return std::get<2>(a)<std::get<2>(b);
}

//start program-specific code

using namespace std;

string round2(double d){
	stringstream ss;
	ss<<std::setprecision(2);
	ss<<d;
	return ss.str();
}

string show(map<Team,Robot_capabilities> const& a){
	auto title1="Estimated Robot capabilities";
	return html(
		head(title(title1))+
		body(
			h1(title1)+
			tag(
				"table border",
				tr(
					tag("th rowspan=2","Team")+
					tag("th colspan=4","Auto pts by location")+
					tag("th rowspan=2","Tele Ball pts")+
					tag("th colspan=5","Endgame Probability")
				)+
				tr(
					th(1)+th(2)+th(3)+th(4)+
					join(mapf(
						[](auto x){ return th(x); },
						endgames()
					))
				)+
				join(mapf(
					[](auto x){
						auto [team,data]=x;
						return tr(
							td(team)+
							join(mapf(
								[&](auto x){
									auto f=data.auto_pts.find(x);
									if(f==data.auto_pts.end()){
										return td("-");
									}
									return td(round2(f->second));
								},
								options((Starting_location*)0)
							))+
							td(round2(data.tele_ball_pts))+
							join(mapf([&](auto x){ return td(round2(data.endgame[x])); },endgames()))
						);
					},
					a
				))
			)
		)
	);
}

using Alliance_capabilities=std::array<Robot_capabilities,3>;

#define ROBOT_STRATEGY_ITEMS(X)\
	X(Starting_location,starting_location)\
	X(bool,defend)\
	X(bool,climb)

struct Robot_strategy{
	ROBOT_STRATEGY_ITEMS(INST)
};

#define ROBOT_TELEOP_STRATEGY_ITEMS(X)\
	X(bool,defend)\
	X(bool,climb)

struct Robot_teleop_strategy{
	ROBOT_TELEOP_STRATEGY_ITEMS(INST)
};

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

double points(Endgame_px e){
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
		total+=points(endgame_px/left);
	}
	return total;
}

double teleop_score(Alliance_capabilities const& cap,Alliance_teleop_strategy const& strat){
	std::array<double,3> defense_values{10,5,2}; //these could be revised at some point.
	double defenders=0;
	double r=0;
	for(auto [cap1,strat1]:zip(cap,strat)){
		if(strat1.defend){
			r+=defense_values[defenders++];
		}else{
			r+=cap1.tele_ball_pts;
		}
	}
	return r+climb_score(cap,strat[0].climb,strat[1].climb,strat[2].climb);
}

double teleop_score(Alliance_capabilities const& a){
	return max(mapf(
		[=](auto x){ return teleop_score(a,x); },
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

using Alliance_strategy=std::array<Robot_strategy,3>;

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

double auto_points(Alliance_capabilities const& cap){
	return max(mapf(
		[=](auto x){ return points(cap,x); },
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
	double r=max(values(a.auto_pts))+a.tele_ball_pts;
	for(auto [type,p]:a.endgame){
		r+=value(type)*p;
	}
	return r;
}

double expected_score(Alliance_capabilities a,Alliance_strategy){
	//obviously, this should become more sophisticated, and actually look at what strategy was passed in.
	return points(sum(a));
}

double expected_score(Alliance_capabilities a){
	return auto_points(a)+teleop_score(a);
}

using Picklist_row=std::tuple<Team,double,std::vector<pair<Team,double>>>;
using Picklist=std::pair<Team,std::vector<Picklist_row>>;

string show(Picklist const& p){
	string title1="Team "+as_string(p.first)+" picklist";

	auto [base_team,data]=p;
	auto p2_len=data.size()?std::get<2>(data[0]).size():22;

	return html(
		head(
			title(title1)
		)+
		body(
			h1(title1)+
			tag(
				"table border",
				tr(
					tag("th colspan=2 rowspan=2","First pick")+
					tag("th colspan="+as_string(p2_len),"Second pick")
				)+
				tr(
					join(mapf(
						[](auto x){ return th(x); },
						range(1,int(1+p2_len))
					))
				)+
				join(mapf(
					[&](auto p){
						auto [i,x]=p;
						auto [team,value,rest]=x;
						return tr(
							th(1+i)+
							th(as_string(team)+"<br>"+small(int(value)))+
							join(mapf(
								[](auto y){ return td(as_string(y.first)+"<br>"+small(int(y.second))); },
								take(p2_len,rest)
							))
						);
					},
					enumerate(data)
				))
			)
		)
	);
}

variant<Picklist,string> make_picklist(Team base_team,map<Team,Robot_capabilities> cap){
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
				expected_score(Alliance_capabilities{base_cap,cap[team],Robot_capabilities{}}),
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
				expected_score(Alliance_capabilities{base_cap,cap[team],generic_third}),
				team
			);
		},
		other_teams
	)));

	//and then figure our all the second picks from that first pick list.
	return make_pair(
		base_team,
		mapf(
			[&](auto x){
				auto [value,team]=x;
				auto team_cap=cap[team];
				auto remaining=other_teams-team;
				auto m=reversed(sorted(mapf(
					[&](auto x){
						return make_pair(
							expected_score(Alliance_capabilities{base_cap,team_cap,cap[x]}),
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

struct Args{
	Team team{1425};
	string scouting_data_path="data/orwil_match.csv";
	string valor_data_path="data/Valor Scout.html";
	string tba_auth_key_path="../tba/auth_key";
	string tba_cache_path="../tba/cache.db";
	tba::Event_key event_key{"2022orwil"};
	bool verbose=0;
	bool compare=0;
	std::optional<Team> team_details;
};

Args parse_args(int argc,char **argv){
	struct Flag{
		string name;
		vector<string> args;
		string help;
		std::function<void(std::vector<std::string>)> func;
	};

	auto x=args(argc,argv);
	Args r;

	vector<Flag> flags{
		{
			"--verbose",{},"Show extra detail about the data used",
			[&](std::vector<std::string>){
				r.verbose=1;
			}
		},
		{
			"--compare",{},"Show comparison of data from different sources",
			[&](std::vector<std::string>){
				r.compare=1;
			}
		},
		{
			"--team",
			{"NUM"},
			"Which team to make the list for",
			[&](std::vector<std::string> v){
				r.team=Team{stoi(v[0])};
			}
		},
		{
			"--team_details",
			{"TEAM"},
			"Show info about a team rather than making a picklist",
			[&](std::vector<std::string> v){
				r.team_details=Team{stoi(v[0])};
			}
		},
		{
			"--csv",
			{"PATH"}
			,"Path to scouting data csv",
			[&](vector<string> v){
				r.scouting_data_path=v[0];
			}
		},
		{
			"--valor",
			{"PATH"},
			"Path to webpage saved from Valor's scouting system",
			[&](vector<string> v){
				r.valor_data_path=v[0];
			}
		},
		{
			"--auth_key",
			{"PATH"},
			"Path to file containing authorization key to talk to The Blue Alliance",
			[&](vector<string> v){
				r.tba_auth_key_path=v[0];
			}
		},
		{
			"--cache",
			{"PATH"},
			"Path to use for cache of data from The Blue Alliance",
			[&](vector<string> v){
				r.tba_cache_path=v[0];
			}
		},
		{
			"--event",
			{"EVENT_KEY"},
			"Event key to use to look up data from The Blue Alliance",
			[&](vector<string> v){
				r.event_key=tba::Event_key{v[0]};
			}
		}
	};
	flags|=Flag{
		"--help",
		{},
		"Show this message",
		[&](vector<string>){
			cout<<x[0];
			for(auto f:flags){
				cout<<" ["<<f.name;
				for(auto arg:f.args){
					cout<<" "<<arg;
				}
				cout<<"]";
			}
			cout<<"\n\n";

			for(auto f:flags){
				cout<<f.name<<"\n";
				cout<<"\t"<<f.help<<"\n";
			}
			exit(0);
		}
	};

	for(auto at=x.begin()+1;at!=x.end();){
		auto f=filter([=](auto x){ return x.name==*at; },flags);
		if(f.size()!=1){
			cerr<<"Error: Unknown argument: "<<*at<<"\n";
			exit(1);
		}
		auto flag=f[0];
		vector<string> v;
		at++;
		for(auto arg:flag.args){
			if(at==x.end()){
				cerr<<"Error: Missing argument \""<<arg<<"\" to "<<flag.name<<"\n";
				exit(1);
			}
			v|=*at;
			at++;
		}
		flag.func(v);
	}
	return r;
}

int main(int argc,char **argv){
	auto args=parse_args(argc,argv);

	using Caps=map<Team,Robot_capabilities>;
	vector<pair<string,Caps>> v;

	v|=make_pair("scouted",parse_csv(args.scouting_data_path));

	{
		auto p1=from_tba(args.tba_auth_key_path,args.tba_cache_path,args.event_key);
		if(holds_alternative<Caps>(p1)){
			v|=make_pair("tba",std::get<Caps>(p1));
		}else{
			cout<<"The Blue Alliance data could not be used.\n";
			cout<<std::get<string>(p1)<<"\n";
		}
	}

	if(v.empty()){
		cout<<"Error: No data available.\n";
		return 1;
	}

	v|=make_pair("valor",valor_data(args.valor_data_path));

	map<Team,set<string>> available;
	for(auto [source,data]:v){
		for(auto [team,info]:data){
			available[team]|=source;
		}
	}

	auto m=mapf([](auto x){ return x.second.size(); },available);
	cout<<"Number of times that a team appears in that # of sources:"<<count(m)<<"\n";
	cout<<"Teams appearing in each dataset:"<<mapf([](auto x){ return make_pair(x.first,x.second.size()); },v)<<"\n";

	if(args.team_details){
		team_details(args.scouting_data_path,*args.team_details);
		return 0;
	}

	if(args.compare){
		using Source=string;
		vector<tuple<double,Team,map<Source,Robot_capabilities>>> diff;
		auto k=or_all(mapf([](auto x){ return keys(x.second); },v));
		
		for(auto team:k){
			map<string,Robot_capabilities> caps;
			for(auto [source,data]:v){
				auto f=data.find(team);
				if(f!=data.end()){
					caps[source]=f->second;
				}
			}
			auto pts=mapf([](auto x){ return points(x); },values(caps));
			diff|=make_tuple(max(pts)-min(pts),team,caps);
		}
		auto x=take(5,reversed(sorted(diff)));
		for(auto [a,b,c]:x){
			cout<<a<<"\t"<<b<<"\n";
			//cout<<"\t"<<c<<"\n";
			for(auto [source,cap]:c){
				cout<<"\t"<<source<<"\t"<<points(cap)<<"\t"<<cap<<"\n";
			}
		}
	}

	/*{
		vector<tuple<double,Team,Robot_capabilities>> v;
		for(auto [team,cap]:p){
			v|=make_tuple(points(cap),team,cap);
		}
		print_lines(sorted(v));
	}*/

	auto data_to_use=v[0].second;
	write_file("cap.html",show(data_to_use));

	auto pl=make_picklist(args.team,data_to_use);
	switch(pl.index()){
		case 0:{
			auto list=std::get<0>(pl);
			write_file("picklist.html",show(list));
			return 0;
		}
		case 1:
			cerr<<std::get<1>(pl)<<"\n";
			return 1;
		default:
			assert(0);
	}
}
