#ifndef MAP_H
#define MAP_H

template<typename Func,typename T>
auto group(Func f,T v)->std::map<decltype(f(*begin(v))),T>{
	std::map<decltype(f(*begin(v))),T> r;
	for(auto elem:v){
		r[f(elem)]|=elem;
	}
	return r;
}
template<typename Func,typename K,typename V>
auto mapf(Func f,std::map<K,V> const& v) -> std::vector< decltype(f(*begin(v))) > {
	std::vector<decltype(f(*begin(v)))> r(v.size());
	std::transform(begin(v),end(v),begin(r),f);
	return r;
}

template<typename K,typename V>
std::map<K,V> without_key(std::map<K,V> a,K key){
	a.erase(a.find(key));
	return a;
}

template<typename K,typename V>
std::ostream& operator<<(std::ostream& o,std::map<K,V> const& a){
	o<<"{ ";
	for(auto elem:a){
		o<<elem<<" ";
	}
	return o<<"}";
}

template<typename Func,typename K,typename V>
auto map_values(Func f,std::map<K,V> const& in){
	std::map<K,decltype(f(in.begin()->second))> r;
	for(auto [k,v]:in){
		r[k]=f(v);
	}
	return r;
}

#define MAP_VALUES(f,v) map_values([&](auto x){ return f(x); },v)

template<typename K,typename V>
std::vector<V> values(std::map<K,V> const& a){
	return mapf([](auto x){ return x.second; },a);
}

template<typename Func,typename K,typename V>
std::map<K,V> filter(Func f,std::map<K,V> a){
	std::map<K,V> r;
	for(auto p:a){
		if(f(p)){
			r[p.first]=p.second;
		}
	}
	return r;
}

template<typename K,typename V>
std::vector<std::pair<K,V>> to_vec(std::map<K,V> a){
	std::vector<std::pair<K,V>> r;
	for(auto p:a){
		r|=p;
	}
	return r;
}

template<typename K,typename V>
std::map<K,V> to_map(std::vector<std::pair<K,V>> const& a){
	std::map<K,V> r;
	for(auto [k,v]:a) r[k]=v;
	return r;
}

template<typename T>
auto dist(std::vector<T> const& a){
	auto m=to_multiset(a);
	std::map<T,size_t> r;
	for(auto elem:a){
		r[elem]=m.count(elem);
	}
	return reversed(sorted(mapf(
		[](auto a){
			return std::make_pair(a.second,a.first);
		},
		r
	)));
}

template<typename Func,typename K,typename V>
std::map<K,V> filter_keys(Func f,std::map<K,V> a){
	std::map<K,V> r;
	for(auto [k,v]:a){
		if(f(k)){
			r[k]=v;
		}
	}
	return r;
}

#define FILTER_KEYS(A,B) filter_keys([&](auto x){ return (A)(x); },(B))

template<typename K,typename V>
std::map<K,V> without_key(K const& k,std::map<K,V> a){
	a.erase(k);
	return a;
}

template<typename K,typename V>
std::set<K> keys(std::map<K,V> a){
	return to_set(mapf(
		[](auto p){ return p.first; },
		a
	));
}

template<typename T>
std::map<T,unsigned> count(std::vector<T> const& v){
	std::map<T,unsigned> r;
	for(auto x:v){
		r[x]++;
	}
	return r;
}

template<typename K,typename V>
std::map<K,V> operator/(std::map<K,V> m,double d){
	for(auto &a:m){
		a.second/=d;
	}
	return m;
}

#endif
