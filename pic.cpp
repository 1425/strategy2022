#include<fstream>
#include<unistd.h>
#include<fcntl.h>
#include "../tba/tba.h"
#include "../tba/db.h"
#include "../tba/curl.h"
#include "util.h"
#include "map.h"

using namespace std;
using namespace tba;

void print_r(int indent,tba::Media_details const& a){
	indent_by(indent++);
	cout<<"Media_details\n";
	#define X(A,B) indent_by(indent); cout<<""#B<<"\n"; print_r(indent+1,a.B);
	TBA_MEDIA_DETAILS(X)
	#undef X
}

void print_r(int indent,Media const& m){
	indent_by(indent++);
	cout<<"Media\n";
	#define X(A,B) indent_by(indent); cout<<""#B<<"\n"; print_r(indent+1,m.B);
	TBA_MEDIA(X)
	#undef X
}

void print_r(Media a){ return print_r(0,a); }

void team_media(Cached_fetcher &f,Team_key team){
	Year year{2022};
	vector<Media> v=team_media_year(f,team,year);
	v|=team_social_media(f,team);
	//print_r(v);	

	auto team_dir="photos/"+as_string(team)+"/";
	auto prepare_dir=[&](){
		auto r=system( ("mkdir -p "+team_dir).c_str() );
		assert(r==0);
	};

	for(auto x:v){
		if(x.type==Media_type::IMGUR){
			assert(x.foreign_key);

			auto path=team_dir+*x.foreign_key+".jpg";

			if(access(path.c_str(),F_OK)==0){
				continue;
			}

			//auto url="https://i.imgur.com/"+*x.foreign_key+".gifv";
			auto url="https://i.imgur.com/"+*x.foreign_key+".jpg";
			//auto url="https://imgur.com/download/"+*x.foreign_key;
			auto res=get_url(url);
			//PRINT(res.headers);
			//PRINT(res.data.size());
			//PRINT(res.data);

			prepare_dir();
			write_file(path,res.data);
		}else if(x.type==Media_type::FACEBOOK_PROFILE){
		}else if(x.type==Media_type::INSTAGRAM_PROFILE){
		}else if(x.type==Media_type::TWITTER_PROFILE){
		}else if(x.type==Media_type::YOUTUBE_CHANNEL){
		}else if(x.type==Media_type::AVATAR){
			//no idea where you would even go to download this.
		}else if(x.type==Media_type::GITHUB_PROFILE){
		}else if(x.type==Media_type::INSTAGRAM_IMAGE){
			auto path=team_dir+*x.foreign_key+".jpg";
			//Example: https://www.instagram.com/p/CQX8lKutaZU/media/?size=l
			auto url="https://www.instagram.com/p/"+*x.foreign_key+"/media/?size=l";
			//PRINT(url);
			auto res=get_url(url);
			//PRINT(res.headers);
			//PRINT(res.data.size());
			//PRINT(res.data);
			prepare_dir();
			write_file(path,res.data);

			//nyi
			//write_file("tmp.html",*x.details->html);
			//system("firefox tmp.html");
		}else if(x.type==Media_type::ONSHAPE){
			assert(x.foreign_key);
			//https://cad.onshape.com/api/thumbnails/d/cd0b0e474e581bdfec4f150d/w/373f57dad67277118d0ce57f/s/600x340
			auto url="https://cad.onshape.com/api/thumbnails/d/"+*x.foreign_key+"/s/600x340";
			auto path=team_dir+(*x.foreign_key).substr(0,10)+".png";
			if(access(path.c_str(),F_OK)==0){
				continue;
			}
			auto res=get_url(url);
			prepare_dir();
			write_file(path,res.data);
		}else if(x.type==Media_type::YOUTUBE){
			//this is a video ... ignore it.
		}else{
			PRINT(team);
			print_r(x);
		}
	}
}

int main1(){
	//TODO: make it so that can look at predictions and pull those teams up
	//or maybe just do all in the district?

	string tba_auth_key_path="../tba/auth_key";
	string tba_cache_path="../tba/cache.db";
	ifstream file{tba_auth_key_path};
	string tba_key;
	getline(file,tba_key);
	tba::Cached_fetcher f{tba::Fetcher{tba::Nonempty_string{tba_key}},tba::Cache{tba_cache_path.c_str()}};

	//Event_key event_key{"2022orsal"};
	for(auto event_key:district_events_keys(f,District_key{"2022pnw"})){
		for(auto x:event_teams_keys(f,event_key)){
			team_media(f,x);
		}
	}
	return 0;
}

int main(){
	try{
		return main1();
	}catch(Decode_error const& s){
		cerr<<s<<"\n";
		return 1;
	}
}
