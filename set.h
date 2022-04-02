#ifndef SET_H
#define SET_H

#include<set>
#include<vector>
#include<iterator>
#include<iostream>
#include<array>
#include<cassert>

size_t sum(std::multiset<bool> const&);
float mean(std::multiset<bool> const&);

template<typename Func,typename T>
std::multiset<T> filter(Func f,std::multiset<T> const& t){
	std::multiset<T> r;
	std::copy_if(begin(t),end(t),inserter(r,r.end()),f);
	return r;
}

template<typename Func,typename T>
T take_first(Func f,std::multiset<T> m){
	for(auto elem:m){
		if(f(elem)){
			return elem;
		}
	}
	assert(0);
}

#define RM_CONST(X) typename std::remove_const<X>::type

#define ELEM(X) RM_CONST(RM_REF(decltype(*std::begin(X))))

template<typename Func,typename T>
auto mapf(Func f,std::multiset<T> const& a)->
	std::vector<decltype(f(*begin(a)))>
{
	std::vector<decltype(f(*begin(a)))> r;
	for(auto elem:a){
		r|=f(elem);
	}
	return r;
}

template<typename T>
T mode(std::vector<T> v){
	assert(v.size());
	auto m=std::multiset<T>(begin(v),end(v));
	auto max_count=max(mapf([&](auto x){ return m.count(x); },m));
	return take_first([&](auto x){ return m.count(x)==max_count; },m);
}

template<typename Func,typename T>
auto mapf(Func f,std::set<T> const& a)->
	std::vector<decltype(f(*begin(a)))>
{
	std::vector<decltype(f(*begin(a)))> r;
	for(auto elem:a){
		r|=f(elem);
	}
	return r;
}

template<typename T>
std::multiset<T>& operator|=(std::multiset<T>& a,T t){
	a.insert(t);
	return a;
}

template<typename T>
std::ostream& operator<<(std::ostream& o,std::multiset<T> const& a){
	o<<"{ ";
	for(auto elem:a){
		o<<elem<<" ";
	}
	return o<<"}";
}

template<typename T>
std::multiset<T>& operator|=(std::multiset<T>& a,std::multiset<T> b){
	a.insert(b.begin(),b.end());
	return a;
}

template<typename T>
std::multiset<T> operator+(std::multiset<T> a,std::multiset<T> b){
	a|=b;
	return a;
}

template<typename T>
std::multiset<T> flatten(std::vector<std::multiset<T>> a){
	std::multiset<T> r;
	for(auto elem:a){
		r|=elem;
	}
	return r;
}

template<typename T>
std::multiset<T> to_multiset(std::vector<T> const& a){
	return std::multiset<T>(begin(a),end(a));
}

template<typename T>
T max(std::multiset<T> const& a){
	assert(a.size());
	T r=*begin(a);
	for(auto elem:a){
		r=std::max(r,elem);
	}
	return r;
}

template<typename T>
std::set<T> to_set(std::multiset<T> const& a){
	return std::set<T>(begin(a),end(a));
}

template<typename T>
std::vector<T> to_vec(std::multiset<T> a){
	return std::vector<T>(a.begin(),a.end());
}

double mean(std::multiset<unsigned> const&);

template<typename T>
std::set<T>& operator|=(std::set<T>& a,std::vector<T> const& b){
	a.insert(b.begin(),b.end());
	return a;
}

template<typename T>
std::set<T>& operator|=(std::set<T>& a,T t){
	a.insert(t);
	return a;
}

template<typename T>
std::vector<T> to_vec(std::set<T> a){
	return std::vector<T>{begin(a),end(a)};
}

template<typename T>
std::set<T> operator|(std::set<T> a,std::set<T> const& b){
	a.insert(begin(b),end(b));
	return a;
}

template<typename T>
bool operator&(T t,std::set<T> const& s){
	return s.count(t)!=0;
}

template<typename Func,typename T>
std::set<T> filter(Func f,std::set<T> const& t){
	std::set<T> r;
	std::copy_if(begin(t),end(t),inserter(r,r.end()),f);
	return r;
}

template<typename T>
std::set<T> to_set(std::vector<T> const& v){
	return std::set<T>(begin(v),end(v));
}

template<typename T>
std::set<T> choose(size_t num,std::set<T> a){
	if(num==0){
		return {};
	}
	assert(a.size());
	auto other=choose(num-1,a);
	auto left=filter([other](auto x){ return other.count(x)==0; },a);
	return other|std::set<T>{to_vec(left)[rand()%left.size()]};
}

template<typename T>
std::set<T>& operator-=(std::set<T> &a,T const& t){
	auto it=a.find(t);
	if(it!=a.end()){
		a.erase(it);
	}
	return a;
}

template<typename T,typename T2>
std::set<T>& operator-=(std::set<T> &a,T2 b){
	auto it=a.find(T{b});
	if(it!=a.end()){
		a.erase(it);
	}
	return a;
}

template<typename T>
std::set<T>& operator-=(std::set<T> &a,std::set<T> const& b){
	for(auto elem:b){
		a-=elem;
	}
	return a;
}

template<typename T,size_t N>
std::set<T> to_set(std::array<T,N> const& a){
	return std::set<T>(a.begin(),a.end());
}

template<typename T>
std::set<T>& operator|=(std::set<T>& a,std::set<T> const& b){
	a.insert(b.begin(),b.end());
	return a;
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

template<typename T>
std::ostream& operator<<(std::ostream& o,std::set<T> const& a){
	o<<"{ ";
	std::ostream_iterator<T> out_it(o," ");
	std::copy(begin(a),end(a),out_it);
	return o<<"}";
}

template<typename T>
T max(std::set<T> const& a){
	assert(a.size());
	T r=*begin(a);
	for(auto elem:a){
		r=std::max(r,elem);
	}
	return r;
}

#endif
