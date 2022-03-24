#include "valor.h"
#include<unistd.h>
#include<cassert>
#include<cstdlib>
#include<iostream>
#include<sstream>
#include<boost/tokenizer.hpp>
#include "util.h"

using namespace std;

//TODO: Make it so that the data can be passed in.

string read_data(std::string const& path){
	int fds[2];
	int r=pipe(fds);
	assert(!r);

	auto pid=fork();
	if(pid==-1){
		assert(0);
	}
	if(pid==0){
		//child process
		close(STDOUT_FILENO);
		{
			int r=dup2(fds[1],STDOUT_FILENO);
			assert(r!=-1);
		}

		//int r=system("python3 valor.py");
		execl("/usr/bin/python3","python3","valor.py","--path",path.c_str(),NULL);
		perror("execl");
		exit(1);
	}

	//parent process

	//close the write end of the pipe so that we will find out when the subprocess dies.
	close(fds[1]);

	stringstream ss;
	while(1){
		static const size_t LEN=80;
		char buf[LEN];
		auto r=read(fds[0],buf,LEN);
		if(r==0){
			return ss.str();
		}
		assert(r!=-1);
		ss.write(buf,r);
	}
}

map<Team,Robot_capabilities> parse(string s){
	using namespace boost;
	vector<vector<string>> data;
	{
		stringstream ss;
		ss<<s;
		string line;
		while(getline(ss,line)){
			tokenizer<escaped_list_separator<char>> t(line);
			vector<string> v{t.begin(),t.end()};
			data.push_back(v);
		}
	}

	if(data.empty()){
		return {};
	}
	auto headers=data[0];
	auto labeled=mapf(
		[=](auto x){ return to_map(zip(headers,x)); },
		tail(data)
	);

	//PRINT(headers);
	//PRINT(take(5,labeled));

	auto cap=[&](map<string,string> m)->pair<Team,Robot_capabilities>{
		Team team{stoi(m["Team"])};
		return make_pair(
			team,
			Robot_capabilities{
				//auto_pts
				[&](){
					Auto r;
					auto v=stof(m["Auto Cargo"])+stof(m["Taxi"]);
					//since we have no data about where the starting locations are, just assume that it doesn't matter.
					for(auto s:options((Starting_location*)0)){
						r[s]=v;
					}
					return r;
				}(),
				//tele_ball_pts
				stof(m["Teleop Cargo"]),
				//endgame
				[&]{
					//going to be very generous and just assume that the points are from doing something consistently
					//this is obviously not true
					auto v=stof(m["Climb"]);
					if(v<0) return Endgame_px{{Endgame::None,1}};
					auto e1=sorted(mapf(
						[](auto x){ return make_pair(value(x),x); },
						endgames()
					));
					for(auto [value,endgame]:e1){
						if(value<v) continue;
						return Endgame_px{
							{Endgame::None,1-v/value},
							{endgame,v/value}
						};
					}
					return Endgame_px{{Endgame::Traversal,1}};
				}(),
				30,
				1
			}
		);
	};

	return to_map(mapf(cap,labeled));
}

std::map<Team,Robot_capabilities> valor_data(std::string const& path){
	auto s=read_data(path);
	return parse(s);
}

/*int main(){
	auto p=valor_data();
	print_lines(p);
}*/
