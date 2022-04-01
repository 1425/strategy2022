#include<set>
#include<iomanip>
#include<functional>
#include "valor.h"
#include "tba.h"
#include "scout.h"
#include "map.h"
#include "array.h"
#include "strategy.h"

//start generic code


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
	std::optional<std::string> pit_scouting_data_path;
	std::optional<std::array<Team, 3>> team_list; 
	std::optional<std::array<Team,6>> match6;
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
			"--pit",{"PATH"},
			"Path to pit scouting data CSV",
			[&](vector<string> v){
				r.pit_scouting_data_path=v[0];
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
		},
		{
			"--match_strategy",
			{"TEAM1", "TEAM2", "TEAM3"},
			"Determines strategy for a match given teams on alliance",
			[&](vector<string> v){
				r.team_list=std::array<Team,3>{
					Team{stoi(v[0])},
					Team{stoi(v[1])},
					Team{stoi(v[2])}
					
				};
			}
		},
		{
			"--match6",
			{"RED1","RED2","RED3","BLUE1","BLUE2","BLUE3"},
			"Look at in-match strategy for a whole match",
			[&](vector<string> v){
				r.match6=mapf(
					[=](auto i){
						try{
							return Team{stoi(v[i])};
						}catch(...){
							cerr<<"Error: Could not parse team number.\n";
							exit(1);
						}
					},
					range_st<6>()
				);
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

int show_html(std::string const& path){
	#ifdef __linux__
	return system( (string()+"firefox "+path).c_str() );
	#else
	cout<<"See results in "<< path <<"\n";
	return system( (string()+"\"/cygdrive/C/Program Files (x86)/Microsoft/Edge/Application/msedge.exe\" file://C:/cygwin64/home/ahgos/strategy2022/"+path).c_str() );
	return 0;
	#endif
}

int match_strategy(std::array<Team,3> const& teams,map<Team,Robot_capabilities> const& m){
	assert(teams.size()==3);
	auto cap=mapf(
		[=](auto x){
			auto f=m.find(x);
			if(f==m.end()){
				cerr<<"Could not find team:"<<x<<"\n";
				exit(1);
			}
			return f->second;
		},
		teams
	);
	auto x=expected_score(cap);

	auto climb_priority=seconds(reversed(sorted(mapf(
		[](auto x){
			auto [team,cap1]=x;
			return make_pair(points(cap1.endgame),team);
		},
		zip(teams,cap)
	))));

	cout<<"Expected net score:"<<x.first<<"\n";
	for(auto [team,strat,cap1]:zip(teams,x.second,cap)){
		cout<<team<<"\t"<<strat<<"\t"<<cap1<<"\n";
	}

	PRINT(climb_priority);

	auto title1="Match strategy for "+join("-",MAP(as_string,teams));
	auto h=html(
		head(title(title1))+
		body(
			h1(title1)+
			p("Expected net score:"+as_string(x.first))+
			tag(
				"table border",
				tr(
					th("Team")+
					th("Starting location")+
					th("Defend")+
					th("Climb")+
					th("Capabilities")
				)+
				join(mapf(
					[](auto x){
						auto [team,strat1,cap1]=x;
						return tr(
							td(team)+
							td(strat1.starting_location)+
							td(strat1.defend)+
							td(strat1.climb)+
							td(cap1)
						);
					},
					zip(teams,x.second,cap)
				))
			)
		)
	);
	auto filename="match_strategy.html";
	write_file(filename,h);
	return show_html(filename);
}

int match6(std::array<Team,6> const& teams,map<Team,Robot_capabilities> const& m){
	if(to_set(teams).size()!=6){
		cout<<"Warning: Duplicate teams.\n";
	}
	auto red=take<3>(teams);
	auto blue=skip<3>(teams);

	auto caps=[=](auto alliance){
		return mapf(
			[=](auto team){
				auto f=m.find(team);
				if(f==m.end()){
					cerr<<"Error: Could not find "<<team<<"\n";
					exit(1);
				}
				return f->second;
			},
			alliance
		);
	};
	auto red_cap=caps(red);
	auto blue_cap=caps(blue);

	auto red_exp=expected_score(red_cap);
	auto blue_exp=expected_score(blue_cap);

	auto show_alliance=[&](
		std::string color,
		std::array<Team,3> const& teams,
		Alliance_capabilities const& cap,
		pair<double,Alliance_strategy> const& exp
	){
		cout<<color<<" alliance:\n";
		auto expected_points=exp.first;
		PRINT(expected_points);
		for(auto z:zip(teams,cap,exp.second)){
			auto [team,cap1,strat]=z;
			cout<<team<<"\t"<<strat<<"\t"<<cap1<<"\n";
		}
	};

	show_alliance("Red",red,red_cap,red_exp);
	show_alliance("Blue",blue,blue_cap,blue_exp);

	auto title1="Match strategy "+join("-",red)+" vs "+join("-",blue);

	auto half=[&](auto f,auto alliance_data){
		return join(mapf(
			[=](auto x){ return td(f(x)); },
			alliance_data
		));
	};

	auto whole=[&](string name,auto f){
		return tr(half(f,red_exp.second)+th(name)+half(f,blue_exp.second));
	};

	auto wcap=[&](string name,auto f){
		return tr(half(f,red_cap)+th(name)+half(f,blue_cap));
	};

	auto auto_item=[&](Starting_location s){
		return wcap(
			"Auto start "+as_string(s),
			[=](auto x)->string{
				auto m=x.auto_pts;
				auto f=m.find(s);
				if(f==m.end()){
					return "-";
				}
				return round2(f->second);
			}
		);
	};

	auto h=html(
		head(title(title1))+
		body(
			h1(title1)+
			tag(
				"table border",
				tr(
					tag("th colspan=3","Red")+
					td("")+
					tag("th colspan=3","Blue")
				)+
				tr(
					tag("td colspan=3 align=center",round2(red_exp.first))+
					th("Expected net score")+
					tag("td colspan=3 align=center",round2(blue_exp.first))
				)+
				tr(
					join(MAP(th,red))+th("Team")+join(MAP(th,blue))
				)+
				tr(
					tag("th colspan=7","Recommended strategy")
				)+
				whole("Starting location",[](auto x){ return x.starting_location; })+
				whole("Defend",[](auto x){ return x.defend; })+
				whole("Climb",[](auto x){ return x.climb; })+
				tr(tag("th colspan=7","Capabilities"))+
				//wcap("Auto",[](auto x){ return x.auto_pts; })+
				join(mapf(auto_item,options((Starting_location*)0)))+
				wcap("Tele ball pts",[](auto x){ return round2(x.tele_ball_pts); })+
				//wcap("Endgame",[](auto x){ return x.endgame; })
				wcap("Endgame Traversal",[](auto x){ return round2(x.endgame[Endgame::Traversal]); })+
				wcap("Endgame High",[](auto x){ return round2(x.endgame[Endgame::High]); })+
				wcap("Endgame Mid",[](auto x){ return round2(x.endgame[Endgame::Mid]); })+
				wcap("Endgame Low",[](auto x){ return round2(x.endgame[Endgame::Low]); })+
				wcap("Endgame None",[](auto x){ return round2(x.endgame[Endgame::None]); })+
				wcap("Climb time",[](auto x){ return round2(x.climb_time); })
			)
		)
	);
	auto filename="match6.html";
	write_file(filename,h);
	return show_html(filename);
}
		
using Caps=map<Team,Robot_capabilities>;

void compare_inner(vector<pair<string,Caps>> const& v){
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

void compare(vector<pair<string,Caps>> v){
	compare_inner(v);

	cout<<"Climb-only comparison\n";
	for(auto &x:v){
		auto& [source,caps]=x;
		for(auto &elem:caps){
			auto& [k,v]=elem;
			v.auto_pts.clear();
			v.tele_ball_pts=0;
		}
	}
	compare_inner(v);
}

int main(int argc,char **argv){
	auto args=parse_args(argc,argv);

	if(args.pit_scouting_data_path){
		return parse_pit_scouting(*args.pit_scouting_data_path);
	}

	vector<pair<string,Caps>> v;

	v|=make_pair("scouted",parse_csv(args.scouting_data_path,args.verbose));
	vector<Team> no_defense {Team{3812}, Team{1432}};
	for (auto team: no_defense){
		v[0].second[team].defense_okay = 0;
	}

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

	if(args.team_list){
		return match_strategy(*args.team_list,v[0].second);
	}

	if(args.match6){
		return match6(*args.match6,v[0].second);
	}

	if(args.compare){
		compare(v);
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

