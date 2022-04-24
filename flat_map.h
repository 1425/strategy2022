#ifndef FLAT_MAP_H
#define FLAT_MAP_H

template<typename K,typename V>
class Flat_map{
	public:
	using P=std::pair<K,V>;
	using Data=std::vector<P>;
	using iterator=typename Data::iterator;
	using const_iterator=typename Data::const_iterator;

	private:
	Data data; //this is not kept in any order.
	
	public:
	Flat_map(){}
	Flat_map(std::map<K,V> const& a):data(a.begin(),a.end()){}

	V& operator[](K k){
		for(auto at=begin();at!=end();++at){
			if(at->first==k){
				return at->second;
			}
		}
		data|=P{k,V{}};
		return data[data.size()-1].second;
	}

	iterator begin(){ return data.begin(); }
	iterator end(){ return data.end(); }
	const_iterator begin()const{ return data.begin(); }
	const_iterator end()const{ return data.end(); }

	iterator find(K k){
		for(auto it=begin();it!=end();++it){
			if(it->first==k){
				return it;
			}
		}
		return end();
	}

	auto operator<=>(Flat_map const&)const=default;

	Flat_map operator+(Flat_map const& a)const{
		auto r=*this;
		for(auto [k,v]:a){
			r[k]+=v;
		}
		return r;
	}

	void clear(){
		data.clear();
	}
};

template<typename K,typename V>
std::ostream& operator<<(std::ostream& o,Flat_map<K,V> const& a){
	o<<"{ ";
	for(auto elem:a){
		o<<elem<<" ";
	}
	return o<<"}";
}

template<typename K,typename V>
std::vector<V> values(Flat_map<K,V> const& v){
	std::vector<V> r;
	for(auto p:v) r|=p.second;
	return r;
}

#endif
