#include "scout.h"
#include<boost/tokenizer.hpp>
#include<boost/algorithm/string.hpp>

using namespace std;

std::string upper(std::string const& s){
	return boost::to_upper_copy<std::string>(s);
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
}

map<Team,Robot_capabilities> parse_csv(string const& filename,bool verbose){
	auto vu=parse_csv_inner(filename,verbose);
	//things to look at:
	//how many matches for each team
	//set of teams is different in the same match
	//for each column:
		//min/max values
		//most common values

	//todo: endgame times put in.
	//endgame__climbtime

	return to_map(mapf(
		[](auto p){
			auto [team,data]=p;
			//cout<<team<<" "<<data.size()<<"\n";
			Robot_capabilities r{
				#define ITEMS(A) mapf([](auto x){ return x.A; },data)
				//auto_pts
				[=]()->Auto{
					auto g=group(
						[](auto x){ return x.start_position; },
						data
					);
					return Auto{map_values(
						[](auto data){
							return mean_d(ITEMS(taxi))*2+mean_d(ITEMS(auto_high))*4+mean_d(ITEMS(auto_low))*2;
						},
						g
					)};
				}(),
				//tele_ball_pts
				mean_d(ITEMS(tele_high))*2+mean_d(ITEMS(tele_low)),
				//endgame
				[&](){
					auto x=to_multiset(ITEMS(endgame));
					map<Endgame,double> r;
					for(auto elem:x){
						r[elem]=(0.0+x.count(elem))/x.size();
					}
					return r;
				}()
				#undef ITEMS
			};
			return make_pair(team,r);
		},
		group(
			[](auto x){ return x.team; },
			vu
		)
	));
}
