#include "scout.h"
#include<boost/tokenizer.hpp>
#include<boost/algorithm/string.hpp>

using namespace std;

std::string upper(std::string const& s){
	return boost::to_upper_copy<std::string>(s);
}

int parse_pit_scouting(std::string const& path="data/test_pit.csv"){
	ifstream f(path);

	vector<vector<string>> data;
	string line;
	while(getline(f,line)){
		using namespace boost;
		tokenizer<escaped_list_separator<char>> t(line);
		//data|=vector<string>{t.begin(),t.end()};
		vector<string> v{t.begin(),t.end()};
		data.push_back(v);
	}

	if(data.empty()){
		return 0;
	}
	
	auto headers=data[0];

	//Interesting cols:
	//NickName
	//TeamNumber

	auto z=mapf(
		[=](auto x){ return to_map(zip(headers,x)); },
		tail(data)
	);
	//print_lines(z);

	for(auto h:headers){
		auto m=mapf([&](auto x){ return x[h];},z);
		cout<<h<<":"<<count(m)<<"\n";
	}

	//db_type
	//db_wheels
	//db_cims
	//db_neos
	//db_falcons
	//db_others

	return 0;
}

tba::Alliance_color parse(tba::Alliance_color const*,string const& s){
	if(s=="red") return tba::Alliance_color::RED;
	if(s=="blue") return tba::Alliance_color::BLUE;
	throw invalid_argument{"Unknown color"};
}

Endgame parse(Endgame const*,string const& s){
	#define X(A) if(upper(s)==upper(""#A)) return Endgame::A;
	TBA_ENDGAME_2022_OPTIONS(X)
	#undef X
	if(s=="traverse") return Endgame::Traversal;

	if(s=="") throw invalid_argument("blank");
	throw invalid_argument{"Endgame unrecognized"};
}

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

ostream& operator<<(std::ostream& o,Useful_data const& a){
	o<<"Useful_data{";
	#define X(A,B,C) o<<" "#B<<":"<<a.B;
	USEFUL_ITEMS(X)
	#undef X
	return o<<" }";
}

vector<Useful_data> parse_csv_inner(std::string const& filename,bool verbose){
	ifstream f(filename);

	vector<vector<string>> data;
	string line;
	while(getline(f,line)){
		using namespace boost;
		tokenizer<escaped_list_separator<char>> t(line);
		//data|=vector<string>{t.begin(),t.end()};
		vector<string> v{t.begin(),t.end()};
		data.push_back(v);
	}

	if(data.empty()){
		return {};
	}

	/*if(data.size()){
		cout<<data[0]<<"\n";
		cout<<data[1]<<"\n";
	}
	cout<<"rows:"<<data.size()<<"\n";
	cout<<"cols:"<<count(MAP(size,data))<<"\n";*/

	if(verbose){
		for(auto i:range(data[0].size())){
			auto name=data[0][i];
			auto values=mapf([=](auto x){ return x[i]; },tail(data));
			cout<<name<<": "<<count(values)<<"\n";
		}
	}

	auto by_label=mapf(
		[&](auto x){
			return to_map(zip(data[0],x));
		},
		tail(data)
	);

	vector<Useful_data> vu;
	for(auto row:by_label){
		Useful_data u;
		vector<pair<string,string>> good,bad;
		#define X(A,B,C) try{\
			u.B=parse((A*)0,row[""#C]);\
			good|=make_pair(""#C,row[""#C]);\
		}catch(...){\
			bad|=make_pair(""#C,row[""#C]);\
		}
		USEFUL_ITEMS(X)
		#undef X
		if(bad.empty()){
			vu|=u;
		}
	}
	cout<<"Good rows:"<<vu.size()<<" ("<<vu.size()/6.0<<" matches)\n";

	return vu;
}

void team_details(string const& filename,Team const& team){
	auto p=parse_csv_inner(filename,1);
	auto f=filter([=](auto x){ return x.team==team; },p);
	print_lines(f);

	auto caps=parse_csv(filename,0);
	cout<<"Estimates:\n"<<caps[team]<<"\n";
}

Robot_capabilities to_robot_capabilities(vector<Useful_data> const& data){
	return Robot_capabilities{
		#define ITEMS(A) mapf([](auto x){ return x.A; },data)
		//auto_pts
		[=]()->Auto{
			auto g=group(
				[](auto x){ return x.start_position; },
				data
			);

			//how many taxi points get at the worst spot they've tried
			auto worst_taxi_pos_value=[&]()->double{
				if(g.empty()) return 0;
				return min(mapf(
					[](auto data){ return mean_d(ITEMS(taxi)); },
					values(g)
				));
			}();

			return Auto{to_map(mapf(
				[=](auto loc){
					auto f=g.find(loc);
					if(f==g.end()){
						//if never tried a location, assume that they can at least do taxi from there how they do somewhere else
						return make_pair(loc,worst_taxi_pos_value);
					}
					auto data=f->second;
					return make_pair(
						loc,
						mean_d(ITEMS(taxi))*2+mean_d(ITEMS(auto_high))*4+mean_d(ITEMS(auto_low))*2
					);
				},
				options((Starting_location*)0)
			))};
		}(),
		//tele_ball_pts
		[=](){
			static const int TELEOP_LEN=60*2+15;
			auto m=mapf(
				[](auto match){	
					//points scored during normal phase
					auto pts=match.tele_high*2+match.tele_low;

					//endgame did something?
					auto did_climb=match.endgame!=Endgame::None;

					double normal_time;
					if(did_climb){
						if(match.climbtime){
							normal_time=TELEOP_LEN-match.climbtime;
						}else{
							//if no data about length of length of climb, assume 30 seconds
							normal_time=TELEOP_LEN-30;
						}
					}else{
						normal_time=TELEOP_LEN;
					}
					return double(pts)/normal_time;
				},
				data
			);
			if(m.empty()) return double(0);
			return mean(m)*(TELEOP_LEN-30);
			//old version: return mean_d(ITEMS(tele_high))*2+mean_d(ITEMS(tele_low));
		}(),
		//endgame
		[&](){
			auto x=to_multiset(ITEMS(endgame));
			map<Endgame,double> r;
			for(auto elem:x){
				r[elem]=(0.0+x.count(elem))/x.size();
			}
			return r;
		}(),
		//climb time
		[&]()->double{
			vector<double> v;
			for(auto match:data){
				if(match.endgame==Endgame::None) continue;
				if(match.climbtime==0) continue;
				v|=match.climbtime;
			}
			if(v.empty()) return 30;
			return mean(v);
		}(), 1
		#undef ITEMS
	};
}

map<Team,Robot_capabilities> parse_csv(string const& filename,bool verbose){
	/*auto x=parse_pit_scouting();
	PRINT(x);*/

	auto vu=parse_csv_inner(filename,verbose);
	//things to look at:
	//how many matches for each team
	//set of teams is different in the same match
	//for each column:
		//min/max values
		//most common values

	//todo: endgame times put in.
	//endgame__climbtime



	return map_values(
		to_robot_capabilities,
		group(
			[](auto x){ return x.team; },
			vu
		)
	);
}

