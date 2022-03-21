#include "input_data.h"

using namespace std;

template<typename K,typename V>
map<K,V> operator+(map<K,V> a,map<K,V> const& b){
	for(auto [k,v]:b){
		a[k]+=v;
	}
	return a;
}

Auto& operator/=(Auto& a,int i){
	for(auto &p:a){
		p.second/=i;
	}
	return a;
}

Robot_capabilities operator+(Robot_capabilities a,Robot_capabilities b){
	Robot_capabilities r;
	r.auto_pts=a.auto_pts+b.auto_pts;
	r.tele_ball_pts=a.tele_ball_pts+b.tele_ball_pts;
	for(auto k:keys(a.endgame)|keys(b.endgame)){
		r.endgame[k]=a.endgame[k]+b.endgame[k];
	}
	return r;
}

Robot_capabilities operator/(Robot_capabilities a,int i){
	a.auto_pts/=i;
	a.tele_ball_pts/=i;
	for(auto &p:a.endgame){
		p.second/=i;
	}
	return a;
}

#ifdef __CYGWIN__
std::ostream& operator<<(std::ostream& o, Endgame const &a){
	switch(a){
		case Endgame::None:
			return o<<"None";
		case Endgame::Low:
			return o<<"Low";
		case Endgame::Mid:
			return o<<"Mid";
		case Endgame::High:
			return o<<"High";
		case Endgame::Traversal:
			return o<<"Traversal";
		default:
			assert(0);
	}
}
#endif

std::ostream& operator<<(std::ostream& o,Robot_capabilities const& a){
	o<<"Robot_capabilities( ";
	#define X(T,A) o<<""#A<<":"<<a.A<<" ";
	ROBOT_CAPABILITIES_ITEMS(X)
	#undef X
	return o<<")";
}

std::vector<Endgame> endgames(){
	std::vector<Endgame> r;
	#define X(A) r|=Endgame::A;
	TBA_ENDGAME_2022_OPTIONS(X)
	#undef X
	return r;
}

double value(Endgame a){
	switch(a){
		case Endgame::None:
			return 0;
		case Endgame::Low:
			return 4;
		case Endgame::Mid:
			return 6;
		case Endgame::High:
			return 10;
		case Endgame::Traversal:
			return 15;
		default:
			assert(0);
	}
}
