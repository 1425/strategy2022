#ifndef UTIL_H
#define UTIL_H

#include "set.h"
#include<vector>
#include<iostream>
#include<numeric>
#include<cassert>
#include<map>
#include<sstream>
#include<bitset>
#include<variant>
#include<optional>

#define nyi { std::cout<<"nyi "<<__FILE__<<":"<<__LINE__<<"\n"; exit(44); }
#define PRINT(X) { std::cout<<""#X<<":"<<(X)<<"\n"; }

int sum(std::pair<int,int>);
size_t sum(std::vector<bool> const&);

template<typename T>
T sum(std::vector<T> const& v){
	return std::accumulate(begin(v),end(v),T{});
}

template<typename T>
T sum(std::tuple<T,T,T> t){
	return std::get<0>(t)+std::get<1>(t)+std::get<2>(t);
}

double product(std::vector<double> const&);
double geomean(std::vector<double> const&);

double mean(double,double);
float mean(std::vector<bool> const&);

template<typename T>
T mean(std::vector<T> const& v){
	assert(v.size());
	//The cast is here to avoid making the sum get converted to unsigned.
	return sum(v)/(int)v.size();
}

template<typename A,typename T>
auto mean_else(A a,T t)->decltype(mean(a)){
	if(a.empty()) return t;
	return mean(a);
}

std::string pop(std::vector<std::string>&);

template<typename T,typename T2>
std::vector<T>& operator|=(std::vector<T> &a,T2 t){
	a.push_back(std::move(t));
	return a;
}

template<typename T>
std::vector<T>& operator|=(std::vector<T> &a,std::vector<T> const& b){
	a.insert(a.end(),b.begin(),b.end());
	return a;
}

template<typename A,typename B>
std::ostream& operator<<(std::ostream& o,std::pair<A,B> const& a){
	return o<<"("<<a.first<<","<<a.second<<")";
}

template<typename T>
std::ostream& operator<<(std::ostream& o,std::vector<T> const& a){
	o<<"[ ";
	/*
	Not sure why this commented part doesn't work sometimes.
	std::ostream_iterator<T> out_it(o," ");
	std::copy(begin(a),end(a),out_it);*/
	for(auto elem:a){
		o<<elem<<" ";
	}
	return o<<"]";
}

template<typename T>
std::vector<T> range(T lim){
	std::vector<T> r(lim);
	std::iota(begin(r),end(r),0);
	return r;
}

template<typename T>
std::vector<T> range(T start,T lim){
	assert(lim>=start);
	std::vector<T> r(lim-start);
	std::iota(begin(r),end(r),start);
	return r;
}


template<typename T,typename STEP>
std::vector<T> range(T start,T lim,STEP step){
	assert(lim>=start);
	std::vector<T> r;
	for(auto i=start;i<lim;i+=step){
		r|=i;
	}
	return r;
}

void write_file(std::string filename,std::string data);

int min(int a,unsigned b);

template<typename T>
T min(std::vector<T> const& a){
	assert(a.size());
	T r=*begin(a);
	for(auto elem:a){
		r=std::min(r,elem);
	}
	return r;
}

template<typename T>
T min(T a,T b,T c){ return std::min(a,std::min(b,c)); }

template<typename T>
T max(std::vector<T> const& a){
	assert(a.size());
	return *std::max_element(begin(a),end(a));
}

char as_hex_digit(int);
std::string as_hex(int);
int rerange(int min_a,int max_a,int min_b,int max_b,int value);

template<typename A,typename B>
std::vector<std::pair<A,B>> cross(std::vector<A> const& a,std::vector<B> const& b){
	std::vector<std::pair<A,B>> r;
	for(auto const& a1:a){
		for(auto const& b1:b){
			r|=std::make_pair(a1,b1);
		}
	}
	return r;
}

#define FILTER(F,X) filter([&](auto x){ return F(x); },X)

template<typename Func,typename T>
T filter(Func f,T const& t){
	T r;
	std::copy_if(begin(t),end(t),back_inserter(r),f);
	return r;
}

template<typename T>
T id(T t){ return t; }

template<typename Func,typename T>
auto mapf(Func f,std::vector<T> const& v) -> std::vector< decltype(f(v[0])) > {
	std::vector<decltype(f(v[0]))> r;
	r.reserve(v.size());
	std::transform(begin(v),end(v),std::back_inserter(r),f);
	return r;
}

#define MAP(F,V) mapf([&](auto const& elem){ return F(elem); },V)

template<typename T>
class Nonempty_vector{
	std::vector<T> data;
};

template<typename T>
std::string as_string(T const& t){
	std::stringstream ss;
	ss<<t;
	return ss.str();
}

std::string first_word(std::string const&);

template<typename T>
std::string tag(std::string const& t,T const& body){
	std::stringstream ss;
	ss<<"<"<<t<<">";
	ss<<body;
	ss<<"</"<<first_word(t)<<">";
	return ss.str();
}

template<typename T>
std::string html(T a){ return tag("html",a); }

template<typename T>
std::string table(T a){ return tag("table",a); }

template<typename T>
std::string tr(T a){ return tag("tr",a); }

template<typename T>
std::string td(T a){ return tag("td",a); }

template<typename T>
std::string th(T a){ return tag("th",a); }

template<typename T>
std::string head(T a){ return tag("head",a); }

template<typename T>
auto body(T a){ return tag("body",a); }

template<typename T>
auto title(T a){ return tag("title",a); }

template<typename T>
auto h1(T a){ return tag("h1",a); }

template<typename T>
auto h2(T a){ return tag("h2",a); }

template<typename T>
auto small(T a){ return tag("small",a); }

template<typename T>
auto p(T a){ return tag("p",a); }

std::string join(std::vector<std::string> const&);
std::string join(std::string const&,std::vector<std::string> const&);
std::vector<bool> bools();

#define RM_REF(X) typename std::remove_reference<X>::type

template<typename T>
void print_lines(T const& a){
	/*std::ostream_iterator<RM_REF(decltype(*begin(a)))> out_it(std::cout,"\n");
	std::copy(begin(a),end(a),out_it);*/
	for(auto const& elem:a){
		std::cout<<elem<<"\n";
	}
}

template<typename T>
std::ostream& operator<<(std::ostream& o,std::optional<T> const& a){
	if(a) return o<<*a;
	return o<<"NULL";
}

template<typename T>
std::vector<T> sorted(std::vector<T> a){
	std::sort(begin(a),end(a));
	return a;
}

template<typename T>
std::vector<T> reversed(std::vector<T> a){
	std::reverse(begin(a),end(a));
	return a;
}

template<typename T>
std::vector<T> non_null(std::vector<std::optional<T>> a){
	std::vector<T> r;
	for(auto elem:a){
		if(elem){
			r|=*elem;
		}
	}
	return r;
}

template<typename A,typename B>
std::vector<std::pair<A,B>> zip(std::vector<A> const& a,std::vector<B> const& b){
	return mapf(
		[&](auto i){ return std::make_pair(a[i],b[i]); },
		range(min(a.size(),b.size()))
	);
}

template<typename A,typename B>
auto zip(A const& a,B const& b){
	std::vector<std::pair<ELEM(a),ELEM(b)>> r;
	auto ai=std::begin(a);
	auto bi=std::begin(b);
	for(;ai!=a.end() && bi!=b.end();++ai,++bi){
		r|=std::make_pair(*ai,*bi);
	}
	return r;
}

void indent_by(unsigned indent);

template<typename T>
void print_r(unsigned indent,T const& t){
	indent_by(indent);
	std::cout<<t<<"\n";
}

template<typename A,typename B>
void print_r(unsigned indent,std::pair<A,B> const& p){
	indent_by(indent);
	std::cout<<"pair\n";
	print_r(indent+1,p.first);
	print_r(indent+1,p.second);
}

template<typename K,typename V>
void print_r(unsigned indent,std::map<K,V> const& a){
	indent_by(indent);
	std::cout<<"map\n";
	for(auto elem:a){
		print_r(indent+1,elem);
	}
}

template<typename T>
void print_r(unsigned indent,std::vector<T> const& a){
	indent_by(indent);
	std::cout<<"vector\n";
	for(auto elem:a){
		print_r(indent+1,elem);
	}
}

template<typename T>
void print_r(T t){
	print_r(0,t);
}

template<typename T1,typename T2>
std::vector<T1>& operator|=(std::vector<T1>& a,std::vector<T2> const& b){
	for(auto elem:b){
		a|=elem;
	}
	return a;
}

std::vector<std::string> split(std::string const& s,char target);

template<typename A,typename B>
std::pair<A,B> operator+(std::pair<A,B> a,std::pair<A,B> b){
	return std::make_pair(a.first+b.first,a.second+b.second);
}

template<typename T>
std::vector<T> take(size_t lim,std::vector<T> in){
	if(in.size()<=lim) return in;
	return std::vector<T>(begin(in),begin(in)+lim);
}

template<typename T>
std::vector<T> skip(size_t i,std::vector<T> v){
	//note: this is implemented in a very slow way.
	for(auto _:range(i)){
		(void)_;
		if(v.size()){
			v.erase(v.begin());
		}
	}
	return v;
}

template<typename A,typename B>
std::vector<B> seconds(std::vector<std::pair<A,B>> a){
	return mapf([](auto x){ return x.second; },a);
}

int atoi(std::string const&);
std::vector<std::string> args(int argc,char **argv);

#define INST(A,B) A B;

//kind of nutty that this isn't built in.
template<size_t N>
bool operator<(std::bitset<N> a,std::bitset<N> b){
	return a.to_ullong()<b.to_ullong();
}

int rand(int const*);
bool rand(bool const*);
unsigned rand(unsigned const*);

template<typename A,typename B>
std::vector<std::pair<std::optional<A>,std::optional<B>>> zip_extend(std::vector<A> const& a,std::vector<B> const& b){
	auto a_at=begin(a);
	auto b_at=begin(b);
	auto a_end=end(a);
	auto b_end=end(b);
	std::vector<std::pair<std::optional<A>,std::optional<B>>> r;
	while(a_at!=a_end || b_at!=b_end){
		r|=make_pair(
			[=]()->std::optional<A>{
				if(a_at==a_end) return std::nullopt;
				return *a_at;
			}(),
			[=]()->std::optional<B>{
				if(b_at==b_end) return std::nullopt;
				return *b_at;
			}()
		);
		if(a_at!=a_end) a_at++;
		if(b_at!=b_end) b_at++;
	}
	return r;
}

template<typename T>
std::vector<std::pair<size_t,T>> enumerate(std::vector<T> const& a){
	std::vector<std::pair<size_t,T>> r;
	size_t i=0;
	for(auto const& elem:a){
		r|=std::make_pair(i++,elem);
	}
	return r;
}

template<typename T>
std::vector<std::pair<size_t,T>> enumerate_from(size_t i,std::vector<T> const& a){
	std::vector<std::pair<size_t,T>> r;
	for(auto elem:a){
		r|=make_pair(i++,elem);
	}
	return r;
}

int parse(int const*,std::string const&);
bool parse(bool const*,std::string const&);
unsigned parse(unsigned const*,std::string const&);

template<typename A,typename B>
std::vector<A> firsts(std::vector<std::pair<A,B>> const& a){
	return mapf([](auto x){ return x.first; },a);
}

template<typename T>
double mean_d(std::vector<T> v){
	assert(v.size());
	double x=sum(v);
	return x/v.size();
}

std::string take(size_t,std::string const&);

template<typename Func,typename T>
std::vector<T> sort_by(std::vector<T> a,Func f){
	std::sort(begin(a),end(a),[&](auto x,auto y){ return f(x)<f(y); });
	return a;
}

#define SORT_BY(A,B) sort_by((A),[&](auto x){ return (B)(x); })

template<typename T>
std::vector<T> rand(std::vector<T> const*){
	return mapf(
		[](auto _){
			(void)_;
			return rand((T*)0);
		},
		range(rand()%100)
	);
}

template<typename T>
std::vector<T> cdr(std::vector<T> a){
	if(a.size()) a.erase(begin(a));
	return a;
}

/*template<typename Func,typename T>
T argmax(Func f,std::vector<T> const& a){
	assert(a.size());
	auto x=f(a[0]);
	T r=a[0];
	for(auto elem:cdr(a)){
		auto x1=f(elem);
		if(x1>x){
			x=x1;
			r=elem;
		}
	}
	return r;
}*/

template<typename Func,typename T>
auto argmax(Func f,T const& a){
	auto at=a.begin();
	assert(at!=a.end());
	auto x=f(*at);
	auto r=*at;
	for(;at!=a.end();++at){
		auto x1=f(*at);
		if(x1>x){
			x=x1;
			r=*at;
		}
	}
	return r;
}

#define ARGMAX(A,B) argmax([&](auto x){ return (A)(x); },(B))

template<typename T>
std::vector<T> operator+(std::vector<T> a,std::vector<T> b){
	a|=b;
	return a;
}

template<typename T>
std::vector<std::pair<T,bool>> mark_end(std::vector<T> a){
	return mapf(
		[=](auto p){
			auto [i,v]=p;
			return make_pair(v,i==a.size()-1);
		},
		enumerate(a)
	);
}

template<typename A,typename B,typename C>
std::vector<std::tuple<A,B,C>> zip(std::vector<A> const& a,std::vector<B> const& b,std::vector<C> const& c){
	return mapf(
		[&](auto i){
			return make_tuple(a[i],b[i],c[i]);
		},
		range(min(a.size(),b.size(),c.size()))
	);
}

//template<typename T>
//auto mean_d(multiset<T> a){ return mean_d(to_vec(a)); }

template<typename... A>
std::ostream& operator<<(std::ostream& o,std::variant<A...> const& a){
        std::visit([&](auto &&elem){ o<<elem; },a);
        return o;
}

template<typename A,typename B>
std::vector<std::pair<B,A>> swap_pairs(std::vector<std::pair<A,B>> const& a){
	return mapf([](auto p){ return std::make_pair(p.second,p.first); },a);
}

template<typename T>
std::vector<T> operator|(std::vector<T> const& a,std::vector<T> const& b){
	std::vector<T> r;
	r|=a;
	r|=b;
	return r;
}

template<typename A,typename B,typename C>
std::ostream& operator<<(std::ostream& o,std::tuple<A,B,C> const& a){
	o<<"("<<get<0>(a)<<","<<get<1>(a)<<","<<get<2>(a)<<")";
	return o;
}

template<typename A,typename B,typename C,typename D>
std::ostream& operator<<(std::ostream& o,std::tuple<A,B,C,D> const& a){
	o<<"("<<get<0>(a)<<","<<get<1>(a)<<","<<get<2>(a)<<","<<get<3>(a)<<")";
	return o;
}

template<typename A,typename B,typename C,typename D,typename E>
std::ostream& operator<<(std::ostream& o,std::tuple<A,B,C,D,E> const& a){
	o<<"(";
	o<<get<0>(a)<<","<<get<1>(a)<<",";
	o<<get<2>(a)<<","<<get<3>(a)<<",";
	o<<get<4>(a);
	o<<")";
	return o;
}

template<typename T>
std::vector<T> tail(std::vector<T> a){
	if(a.size()){
		a.erase(a.begin());
	}
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

template<typename A,typename B,typename C>
bool operator<(std::tuple<A,B,C> const& a,std::tuple<A,B,C> const& b){
	if(std::get<0>(a)<std::get<0>(b)) return 1;
	if(std::get<0>(b)<std::get<0>(a)) return 0;

	if(std::get<1>(a)<std::get<1>(b)) return 1;
	if(std::get<1>(b)<std::get<1>(a)) return 0;
	
	return std::get<2>(a)<std::get<2>(b);
}

template<typename T>
std::vector<T> flatten(std::vector<std::vector<T>> const& a){
	std::vector<T> r;
	for(auto elem:a){
		for(auto x:elem){
			r|=x;
		}
	}
	return r;
}

template<typename A,typename B>
std::ostream& operator<<(std::ostream& o,std::tuple<A,B> const& a){
	return o<<"("<<get<0>(a)<<","<<get<1>(a)<<")";
}

template<typename T>
T last(std::vector<T> const& v){
	assert(v.size());
	return v[v.size()-1];
}

#endif
