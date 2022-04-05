#include "scout.h"
#include<fstream>
#include<cmath>
#include<boost/tokenizer.hpp>
#include<boost/algorithm/string.hpp>
#include "map.h"
#include "strategy.h"

template<typename Func,typename T>
void mapv(Func f,T t){
	for(auto elem:t) f(elem);
}

#define MAPV(A,B) mapv([&](auto x){ return (A)(x); },(B))

template<typename T>
std::vector<double> as_doubles(T const& t){
	return MAP(double,t);
}

double mean_discounted(std::vector<double> v,double discount_rate){
	//assuming that v appears in chronological order, with later more recent
	//discount rate should be in range 0-1
	assert(v.size());

	double numerator_total=0;
	double denominator_total=0;
	double weight=1;
	for(auto x:reversed(v)){
		numerator_total+=x*weight;
		denominator_total+=weight;
		weight*=(1-discount_rate);
	}
	return numerator_total/denominator_total;
}

template<typename T>
double mean_discounted(std::vector<T> const& v,double discount_rate){
	return mean_discounted(as_doubles(v),discount_rate);
}

template<typename A,typename B,typename C>
std::vector<C> thirds(std::vector<std::tuple<A,B,C>> const& v){
	return mapf([](auto x){ return get<2>(x); },v);
}

double std_dev(std::vector<double> const& v){
	auto u=mean(v);
	return sqrt(mean(mapf(
		[u](auto x){ return pow(x-u,2); },
		v
	)));
}

std::string upper(std::string const& s){
	return boost::to_upper_copy<std::string>(s);
}

using namespace std;

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
		/*if(u.team==1425 && u.match==13) u.start_position=Starting_location{3};
		if(u.team==2811 && u.match==25) u.start_position=Starting_location{3};
		if(u.team==753 && u.match==29) u.auto_high=1;
		if(u.team==4488 && u.match==29) u.auto_high=1;
		if(u.team==7034 && u.match==29) u.auto_high=2;*/
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

auto options(Endgame const*){
	return endgames();
}

auto points(Endgame a){
	return value(a);
}

Endgame_px count_endgames(vector<pair<Endgame,vector<Endgame>>> const& observed_endgames,double discount_rate){
	//Assuming that observed ingame is passed in oldest matches first.

	//First item is # of times it was available and something better was not taken
	//Second # is # of times that it was taken
	map<Endgame,pair<double,double>> slot_results;

	double weight=1;
	for(auto [this_team,others]:reversed(observed_endgames)){
		Climb_space space;
		MAPV(space.fill,others);
		slot_results[this_team].first+=weight;
		slot_results[this_team].second+=weight;
		for(auto a:space.open()){
			if(points(a)>points(this_team)){
				slot_results[a].first+=weight;
			}
		}
		weight*=(1-discount_rate);
	}
	Endgame_px r;
	for(auto endgame:endgames()){
		auto [could,did]=slot_results[endgame];
		if(could){
			auto value=double(did)/could;
			r[endgame]=value;
		}
	}
	r[Endgame::None]=1;
	return r;
}

Robot_capabilities to_robot_capabilities(vector<Useful_data> const& data,double discount_rate,vector<Useful_data> const& extra_data){
	//Discount rate = how much less to weight each older piece of data; should be something in the range 0-1, hopefully closer to 0.

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
				return 2*min(mapf(
					[=](auto data){ return mean_discounted(as_doubles(ITEMS(taxi)),discount_rate); },
					values(g)
				));
			}();

			Auto a{to_map(mapf(
				[=](auto loc){
					auto f=g.find(loc);
					if(f==g.end()){
						//if never tried a location, assume that they can at least do taxi from there how they do somewhere else
						return make_pair(loc,worst_taxi_pos_value);
					}
					auto data=f->second;
					return make_pair(
						loc,
						mean_discounted(as_doubles(ITEMS(taxi)),discount_rate)*2+
						mean_discounted(ITEMS(auto_high),discount_rate)*4+
						mean_discounted(ITEMS(auto_low),discount_rate)*2
					);
				},
				options((Starting_location*)0)
			))};
			auto best=max(values(a));
			for(auto &x:a){
				x.second=max(x.second,best/2);
			}
			return a;
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
			return mean_discounted(m,discount_rate)*(TELEOP_LEN-30);
			//old version: return mean_d(ITEMS(tele_high))*2+mean_d(ITEMS(tele_low));
		}(),
		//endgame
		[&](){
			/*auto x=to_multiset(ITEMS(endgame));
			map<Endgame,double> r;
			for(auto elem:x){
				r[elem]=(0.0+x.count(elem))/x.size();
			}
			return r;*/
			vector<pair<Endgame,vector<Endgame>>> endgames;
			for(auto team_match:data){
				auto f=filter(
					[=](auto x){ return x.match==team_match.match && x.alliance==team_match.alliance && x.team!=team_match.team; },
					extra_data
				);
				auto m=mapf([](auto x){ return x.endgame; },f);
				endgames|=make_pair(team_match.endgame,m);
			}
			return count_endgames(endgames,discount_rate);

			/*map<Endgame,double> r;
			double weight=1;
			double total_weight=0;
			for(auto endgame:reversed(ITEMS(endgame))){
				r[endgame]+=weight;
				total_weight+=weight;
				weight*=(1-discount_rate);
			}
			return r/total_weight;*/
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
			return mean_discounted(v,discount_rate);
		}(), 1
		#undef ITEMS
	};
}

using Match=int;

//tuple(type name,Team,Match,value,z-score)
void outliers(std::string const& name,vector<tuple<Team,Match,double>> v){
	if(v.empty()) return;

	if(name=="climbtime"){
		//0 means no data for climb time.
		v=filter([](auto x){ return get<2>(x); },v);
	}
	auto values=thirds(v);
	auto mu=mean(values);
	auto sigma=std_dev(values);

	bool shown=0;
	auto show=[&](){
		if(!shown){
			shown=1;
			PRINT(name);
			PRINT(mu);
			PRINT(sigma);
		}
	};
	//auto m=reversed(sorted(mapf([=](auto x){ return fabs(x-mu)/sigma; },values)));
	//print_lines(m);
	auto z=[=](auto value){
		return (value-mu)/sigma;
	};

	for(auto [team,match,value]:v){
		auto z1=z(value);
		if(fabs(z1)>3){
			show();
			cout<<name<<"\t"<<team<<"\t"<<match<<"\t"<<value<<"\t"<<z1<<"\n";
		}
	}

	for(auto [team,data]:group([](auto x){ return get<0>(x); },v)){
		auto values=thirds(data);
		auto mu=mean(values);
		auto sigma=std_dev(values);
		auto z=[&](auto value){
			return (value-mu)/sigma;
		};
		for(auto [team,match,value]:data){
			auto z1=z(value);
			if(fabs(z1)>3){
				cout<<"pt:"<<team<<values<<"\t"<<match<<"\t"<<value<<"\t"<<z1<<"\n";
			}
		}
	}
}

template<int A,int B>
void outliers(std::string const& name,std::vector<tuple<Team,Match,Int_limited<A,B>>> const& v){
	return outliers(name,mapf(
		[](auto x){ return make_tuple(get<0>(x),get<1>(x),double(get<2>(x))); },
		v
	));
}

void outliers(std::string const& name,std::vector<tuple<Team,Match,int>> const& v){
	return outliers(name,mapf(
		[](auto x){ return make_tuple(get<0>(x),get<1>(x),double(get<2>(x))); },
		v
	));
}

void outliers(std::string const& name,std::vector<tuple<Team,Match,unsigned>> const& v){
	return outliers(name,mapf(
		[](auto x){ return make_tuple(get<0>(x),get<1>(x),double(get<2>(x))); },
		v
	));
}

void outliers(std::string const& name,std::vector<tuple<Team,Match,tba::Endgame_2022>> const& v){
	return outliers(name,mapf(
		[](auto x){ return make_tuple(get<0>(x),get<1>(x),double(get<2>(x))); },
		v
	));
}

void outliers(std::string const& name,std::vector<tuple<Team,Match,tba::Alliance_color>> const& v){
	//this one is a little silly to exist.
	return outliers(name,mapf(
		[](auto x){ return make_tuple(get<0>(x),get<1>(x),double(get<2>(x))); },
		v
	));
}

void outliers(std::string const& name,std::vector<tuple<Team,Match,bool>> const& v){
	return outliers(name,mapf(
		[](auto x){ return make_tuple(get<0>(x),get<1>(x),double(get<2>(x))); },
		v
	));
}

template<typename T>
void outliers(std::string const& name,vector<tuple<Team,Match,T>> const& v){
	if(v.empty()) return;
	(void)name;
	cout<<typeid(T).name()<<"\n";
	//PRINT(v);
	nyi
}

bool combo_looks_ok(multiset<Endgame> const& a){
	Climb_space space;
	for(auto x:a){
		if(!space.open(x)){
			return 0;
		}
		space.fill(x);
	}
	return 1;
}

void alliance_combos_endgame(vector<Useful_data> const& v){
	auto g=group([](auto x){ return make_pair(x.match,x.alliance); },v);
	map<multiset<Endgame>,vector<pair<Match,tba::Alliance_color>>> seen;
	for(auto [id,data]:g){
		auto result=to_multiset(mapf([](auto x){ return x.endgame; },data));
		seen[result]|=id;
		//TODO: End results & try to measure if they're possible.
	}
	cout<<"Endgames\n";
	//print_lines(seen);
	for(auto [k,v]:seen){
		if(!combo_looks_ok(k)){
			cout<<mapf([](auto x){ return as_string(x)[0]; },k)<<"    \t"<<v.size()<<"\t"<<take(5,v)<<"\n";
		}
	}
}

bool combo_looks_ok(multiset<Starting_location> const& a){
	//looks ok if there are no duplicates.
	return to_set(a).size()==a.size();
}

void alliance_combos(vector<Useful_data> const& v){
	alliance_combos_endgame(v);

	auto g=group([](auto x){ return make_pair(x.match,x.alliance); },v);
	map<multiset<Starting_location>,vector<pair<Match,tba::Alliance_color>>> seen;
	for(auto [id,data]:g){
		auto result=to_multiset(mapf([](auto x){ return x.start_position; },data));
		seen[result]|=id;
		//TODO: End results & try to measure if they're possible.
	}
	cout<<"Start positions\n";
	//print_lines(seen);
	for(auto [k,v]:seen){
		if(!combo_looks_ok(k)){
			cout<<k<<"    \t"<<v.size()<<"\t"<<take(5,v)<<"\n";
		}
	}
}

void outliers(vector<Useful_data> const& v){
	alliance_combos(v);

	#define X(A,B,C) outliers(""#B,mapf([](auto x){ return make_tuple(x.team,x.match,x.B); },v));
	USEFUL_ITEMS(X)
	#undef X
}

map<Team,Robot_capabilities> capabilities_by_team(vector<Useful_data> const& vu,double discount_rate){
	return map_values(
		[=](auto x){ return to_robot_capabilities(x,discount_rate,vu); },
		group(
			[](auto x){ return x.team; },
			vu
		)
	);
}

map<Team,Robot_capabilities> parse_csv(string const& filename,bool verbose){
	/*auto x=parse_pit_scouting();
	PRINT(x);*/

	auto vu=parse_csv_inner(filename,verbose);
	if(verbose) outliers(vu);
	return capabilities_by_team(vu,.07);
}
