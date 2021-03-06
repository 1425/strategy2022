#include<set>
#include<iomanip>
#include<functional>
#include<fstream>
#include<cmath>
#include<filesystem>
#include "../tba/db.h"
#include "../tba/tba.h"
#include "valor.h"
#include "tba.h"
#include "scout.h"
#include "map.h"
#include "array.h"
#include "strategy.h"
#include "util.h"
#include "opr.h"

//start generic code

template<typename Func,typename T>
auto filter_unique(Func f,T t){
	auto f1=filter(f,t);
	assert(f1.size()==1);
	return f1[0];
}

template<size_t N,typename T>
std::array<T,N> to_array(std::vector<T> const& v){
	assert(v.size()==N);
	using Data=std::array<T,N>;

	//This funny dance with placement new exists so that items that are not default-constructable can still use this function.
	char s[sizeof(Data)];
	Data &r=*(Data*)s;
	for(auto i:range_st<N>()){
		new(&r[i]) T(v[i]);
	}
	return r;
}

template<typename T,size_t A,size_t B>
std::array<T,A+B> operator|(std::array<T,A> const& a,std::array<T,B> const& b){
	std::array<T,A+B> r;
	int i=0;
	for(auto elem:a) r[i++]=elem;
	for(auto elem:b) r[i++]=elem;
	return r;
}

std::vector<std::filesystem::directory_entry> to_vec(std::filesystem::directory_iterator a){
	std::vector<std::filesystem::directory_entry> r;
	for(auto elem:a) r|=elem;
	return r;
}

std::string round2(double d){
	std::stringstream ss;
	ss<<std::setprecision(2);
	ss<<d;
	return ss.str();
}

std::string round3(double d){
	std::stringstream ss;
	ss<<std::setprecision(3);
	ss<<d;
	return ss.str();
}

template<typename T>
T median(std::vector<T> v){
	assert(v.size());
	std::sort(v.begin(),v.end());
	//this is done the simplest way; could do something "more correct" when there is an even number of items.
	return v[v.size()/2];
}

//start program-specific code

using namespace std;

vector<Team> expected_2022pncmp(){
	vector<int> v{
		2046,7034,4089,2910,4488,2930,7461,2990,4911,1540,2976,955,
		3636,2147,360,5827,8532,488,2471,3218,6831,2557,3663,1294,
		3786,1899,492,568,2522,1425,3711,1595,1432,4131,3674,2521,
		5920,4513,1359,997,4125,4512,6350,8248,5970,957,2811,2412,
		2635,4915,2928
	};
	return MAP(Team,v);
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
					tag("th colspan=5","Endgame Probability")+
					tag("th rowspan=2","Climb time")+
					tag("th rowspan=2","Defense OK")
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
							join(mapf([&](auto x){ return td(round2(data.endgame[x])); },endgames()))+
							td(round2(data.climb_time))+
							td(data.defense_okay)
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
	TBA_setup tba;
	tba::Event_key event_key{"2022pncmp"};
	bool verbose=0;
	bool compare=0;
	std::optional<Team> team_details;
	std::optional<std::string> pit_scouting_data_path;
	std::optional<std::array<Team, 3>> team_list; 
	std::optional<std::array<Team,6>> match6;
	bool trend_check=0;
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
				r.tba.auth_key_path=v[0];
			}
		},
		{
			"--cache",
			{"PATH"},
			"Path to use for cache of data from The Blue Alliance",
			[&](vector<string> v){
				r.tba.cache_path=v[0];
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
		},
		{
			"--trend_check",{},
			"Look at the trends of how teams do throughout an event",
			[&](vector<string>){
				r.trend_check=1;
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

	auto show_team=[&](Team team){
		auto fail=h2(team)+p("No image");
		try{
			auto files=to_vec(std::filesystem::directory_iterator("photos/frc"+as_string(team)));
			if(files.empty()) return fail;

			//Choose the file with the largest size to show.
			auto to_show=ARGMAX(std::filesystem::file_size,files);

			return h2(team)+"<img src="+as_string(to_show)+" width=\"100%\">";//p(to_vec(files));
		}catch(std::filesystem::filesystem_error const&){
			return fail;
		}
	};

	auto photos=[&]()->string{
		return table(
			tr(join(mapf([=](auto x){ return tag("td width=30% valign=top",show_team(x)); },red)))+
			tr(join(mapf([=](auto x){ return tag("td valign=top",show_team(x)); },blue)))
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
					tag("td colspan=3 align=center",round3(red_exp.first))+
					th("Expected net score")+
					tag("td colspan=3 align=center",round3(blue_exp.first))
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
			)+
			photos()
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

pair<double,double> predictor(std::vector<tba::Match_Simple> const& match_results,std::vector<Useful_data> const& v,double discount){
	vector<double> error,squared_error;

	for(
		auto match:match_results
	){
		if(match.comp_level!=tba::Competition_level::qm){
			continue;
		}
		//PRINT(match.match_number);
		auto data=filter(
			[=](auto x){ return x.match<match.match_number; },
			v
		);
		//PRINT(data.size());

		//could speed this up by filtering to only the teams that we care about.
		auto all_caps=capabilities_by_team(data,discount);
		//PRINT(all_caps.size());

		auto f=[&](tba::Match_Alliance const& a){
			auto cap=mapf(
				[&](auto team){
					auto f=all_caps.find(team_number(team));
					if(f==all_caps.end()){
						//if no data, assume robot does nothing.
						return Robot_capabilities{};
					}
					return f->second;
				},
				a.team_keys
			);
			auto expected=expected_score(to_array<3>(cap)).first;
			if(a.score.valid()){
				auto actual=a.score.value();
				//PRINT(expected);
				//PRINT(actual);
				error|=fabs(expected-actual);
				squared_error|=pow(expected-actual,2);
			}
		};
		f(match.alliances.red);
		f(match.alliances.blue);
	}
	return make_pair(
		mean(error),
		sqrt(mean(squared_error))
	);
}

void picklist_change(std::string const& scouting_csv_path){
	auto v=parse_csv_inner(scouting_csv_path,0);
	map<Team,vector<pair<double,int>>> m;
	auto xs=range(0.0,1.0,.01);
	for(auto x:xs){
		auto caps=capabilities_by_team(v,x);
		auto pl=make_picklist(Team{1425},caps);
		assert(holds_alternative<Picklist>(pl));
		auto [main_team,list]=get<0>(pl);
		for(auto [i,y]:enumerate_from(1,list)){
			auto [team,v,second]=y;
			m[team]|=make_pair(x,i);
		}
	}

	//label
	for(auto x:xs) cout<<"\t"<<x;
	cout<<"\n";	

	for(auto [team,ranks]:m){
		cout<<team;
		for(auto [discount,rank]:ranks){
			cout<<"\t"<<rank;
		}
		cout<<"\n";
	}
}

void sample_window(std::string const& scouting_csv_path,TBA_setup const& tba_setup){
	auto f=tba_fetcher(tba_setup);
	tba::Event_key event{"2022orsal"};//TODO: Have this get passed in
	auto match_results=sort_by(
		event_matches_simple(f,event),
		[](auto x){ return x.match_number; }
	);
	match_results=filter(
		[](auto x){
			return x.comp_level==tba::Competition_level::qm && x.match_number>5;
		},
		match_results
	);
	auto v=parse_csv_inner(scouting_csv_path,0);
	cout<<"Hard window results\n";
	for(auto n:range(0,13)){
		vector<double> l1,l2;
		for(auto match:match_results){
			auto present_data=filter([=](auto x){ return x.match<match.match_number; },v);
			//use only the last n matches of the teams in question
			auto run=[&](tba::Match_Alliance const& a){
				auto caps=mapf(
					[=](auto team_key){
						Team team=team_number(team_key);
						auto scouted_data=take(n,reversed(filter(
							[=](auto x){
								return x.team==team;
							},
							present_data
						)));
						return to_robot_capabilities(scouted_data);
					},
					to_array<3>(a.team_keys)
				);
				auto expected=expected_score(caps).first;
				if(a.score.valid()){
					auto observed=a.score.value();
					auto l1_v=fabs(expected-observed);
					auto l2_v=pow(expected-observed,2);
					l1|=l1_v;
					l2|=l2_v;
				}
			};
			run(match.alliances.red);
			run(match.alliances.blue);
		}
		//PRINT(l1.size());
		cout<<n<<"\t"<<mean(l1)<<"\t"<<sqrt(mean(l2))<<"\n";
	}
}

void predictor(std::string const& scouting_csv_path,TBA_setup const& tba_setup){
	sample_window(scouting_csv_path,tba_setup);
	picklist_change(scouting_csv_path);

	auto f=tba_fetcher(tba_setup);
	tba::Event_key event{"2022orsal"};//TODO: Have this get passed in
	auto match_results=sort_by(
		event_matches_simple(f,event),
		[](auto x){ return x.match_number; }
	);

	//ignore matches where most teams probably haven't played a match yet, so no data.
	match_results=filter(
		[](auto x){ return x.match_number>5; },
		match_results
	);

	auto v=parse_csv_inner(scouting_csv_path,0);
	for(auto x:range<double>(0.0,.25,.001)){
		//PRINT(x);
		auto p=predictor(match_results,v,x);
		cout<<x<<"\t"<<p.first<<"\t"<<p.second<<"\n";
	}
}

void trend_check(std::string const& path,TBA_setup const& tba_setup){
	predictor(path,tba_setup);

	//vector<Useful_data> parse_csv_inner(std::string const& filename,bool verbose){
	auto v=parse_csv_inner(path,0);
	auto g=group([](auto x){ return x.team; },v);

	map<int,vector<double>> by_round;

	for(auto [team,data]:g){
		//PRINT(team);
		//print_lines(data);
		auto m=::mapf(
			[](auto x){
				return points(to_robot_capabilities(vector<Useful_data>{x}));
			},
			data
		);
		//cout<<team<<"\t"<<min(m)<<"\t"<<median(m)<<"\t"<<max(m)<<"\t"<<mean(m)<<"\t"<<std_dev(m)<<"\n";
		cout<<team;
		for(auto x:m) cout<<"\t"<<x;
		cout<<"\n";

		for(auto [i,x]:enumerate(m)){
			by_round[i]|=x;
		}
	}

	cout<<"By round\n";
	for(auto [i,v1]:by_round){
		cout<<i<<"\t"<<mean(v1)<<"\t"<<std_dev(v1)<<"\n";
	}
}

void compare_matches(TBA_setup const& tba_setup,tba::Event_key const& event,std::string const& csv_path){
	auto tba=tba_by_match(tba_setup,event);
	auto scout=parse_csv_inner(csv_path,0);
	PRINT(tba.size())
	PRINT(scout.size())

	auto tag=[](auto x){ return make_tuple(x.team,x.match); };
	auto tags=[=](auto a){ return to_set(mapf(tag,a)); };
	auto tba_played=tags(tba);//mapf([](auto x){ return make_tuple(x.team,x.match,x.alliance); },tba);
	auto scout_played=tags(scout);//mapf([](auto x){ return make_tuple(x.team,x.match,x.alliance); },scout);

	{
		auto m1=tba_played-scout_played;
		if(!m1.empty()){
			cout<<"Missing scouting data:"<<m1<<"\n";
		}
	}

	{
		auto m1=scout_played-tba_played;
		if(!m1.empty()){
			cout<<"Missing tba data:"<<m1<<"\n";
		}
	}

	using Match=int;
	vector<tuple<Team,Match,string,string,string>> diffs;

	auto both=tba_played&scout_played;
	for(auto tag1:both){
		auto f=[tag,tag1](auto source){ return filter_unique([=](auto x){ return tag(x)==tag1; },source); };
		auto t=f(tba);
		auto s=f(scout);

		#define COMPARE(X) if(t.X!=s.X) diffs|=make_tuple(t.team,t.match,""#X,as_string(t.X),as_string(s.X));
		COMPARE(alliance) //this should never happen
		COMPARE(taxi)
		COMPARE(endgame)
		#undef COMPARE
	}

	print_lines(diffs);
	cout<<"Diff:"<<diffs.size()<<" "<<both.size()<<"\n";
	cout<<count(MAP(get<2>,diffs))<<"\n";
	
	map<string,multiset<pair<string,string>>> changes;
	for(auto [team,match,element,tba,scouted]:diffs){
		changes[element]|=make_pair(tba,scouted);
	}
	for(auto [element,ms]:changes){
		cout<<element<<"\n";
		//print_r(1,count(ms));
		for(auto [a,i]:count(ms)){
			cout<<"\t"<<i<<"\t"<<a<<"\n";
		}
	}
}

using Path=std::string;

map<Team,Robot_capabilities> combined_data(Path scouting_data_path,TBA_setup const& setup,tba::Event_key const& event){
	auto p=parse_csv_inner(scouting_data_path,0);
	auto t=tba_by_match(setup,event);
	for(auto &match_data:p){
		auto f=filter([=](auto x){ return match_data.team==x.team && match_data.match==x.match; },t);
		assert(f.size()<2);
		if(!f.empty()){
			auto x=f[0];
			match_data.taxi=x.taxi;
			match_data.endgame=x.endgame;
		}
	}
	return capabilities_by_team(p,0.07);
}

tba::Event_key most_recent(tba::Cached_fetcher &f,vector<tba::Event_key> const& v){
	return argmax(
		[&](auto x){
			auto d=event_simple(f,x).start_date;
			assert(d);
			return *d;
		},
		v
	);
}

auto hist_test(TBA_setup const& tba_setup,tba::Event_key const& event){
	auto f=tba_fetcher(tba_setup);
	auto teams=[&](){
		if(event=="2022pncmp"){
			return expected_2022pncmp();
		}
		auto e=event_teams_keys(f,event);
		return mapf(to_team,e);
	}();
	tba::Year year{2022}; //TODO: Make this get set based on the event.

	//for each of the teams, look at their most recent event
	map<Team,Robot_capabilities> r;
	for(auto team:teams){
		PRINT(team);
		auto events=team_events_year_keys(f,tba::Team_key{"frc"+as_string(team)},year);

		//look only at previous events.
		events=filter([=](auto x){ return x!=event; },events);

		assert(events.size());
		//auto m=team_event_matches(f,team,last(events));
		auto mr=most_recent(f,events);
		PRINT(mr);
		auto p=process_data(f,mr);//could cache this.
		auto f=p.find(team);
		if(f==p.end()){
			PRINT(keys(p));
			nyi
		}
		r[team]=f->second;
	}
	return r;
}

int main(int argc,char **argv){
	auto args=parse_args(argc,argv);

	if(args.trend_check){
		trend_check(args.scouting_data_path,args.tba);
		return 0;
	}

	if(args.pit_scouting_data_path){
		return parse_pit_scouting(*args.pit_scouting_data_path);
	}

	vector<pair<string,Caps>> v;

	try{
		v|=make_pair("combined",combined_data(args.scouting_data_path,args.tba,args.event_key));
	}catch(...){
		//ignore, just don't add as an option
	}

	v|=make_pair("scouted",parse_csv(args.scouting_data_path,args.verbose));
	vector<Team> no_defense {Team{3812}, Team{1432}};
	for (auto team: no_defense){
		v[v.size()-1].second[team].defense_okay = 0;
	}

	try{
		auto h=hist_test(args.tba,args.event_key);
		for(auto [team,info]:h){
			auto& base=v[0].second;
			auto f=base.find(team);
			if(f==base.end()){
				base[team]=info;
			}
		}
	}catch(...){
		cout<<"History test failed\n";
	}

	{
		auto p1=from_tba(args.tba,args.event_key);
		if(holds_alternative<Caps>(p1)){
			v|=make_pair("tba",std::get<Caps>(p1));
		}else{
			cout<<"The Blue Alliance data could not be used.\n";
			cout<<std::get<string>(p1)<<"\n";
		}
	}

	v|=make_pair("valor",valor_data(args.valor_data_path));

	if(v.empty()){
		cout<<"Error: No data available.\n";
		return 1;
	}

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

	auto data_to_use=v[0].second;

	if(args.team_list){
		return match_strategy(*args.team_list,data_to_use);
	}

	if(args.match6){
		return match6(*args.match6,data_to_use);
	}

	if(args.compare){
		compare_matches(args.tba,args.event_key,args.scouting_data_path);
		compare(v);
	}

	/*{
		vector<tuple<double,Team,Robot_capabilities>> v;
		for(auto [team,cap]:p){
			v|=make_tuple(points(cap),team,cap);
		}
		print_lines(sorted(v));
	}*/

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

