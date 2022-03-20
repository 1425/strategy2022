#ifndef MAP_FIXED_H
#define MAP_FIXED_H

#include<array>
#include<map>
#include<iostream>
#include<vector>

template<typename K,typename V,size_t N>
class Map_fixed{
	using P=std::pair<K,V>;
	using Data=std::array<P,N>;

	char buf[sizeof(Data)];
	size_t size_=0;

	public:
	Map_fixed(){}

	Map_fixed(Map_fixed const& a):size_(a.size_){
		for(auto i:range(size_)){
			new(begin()+i) P(*(a.begin()+i));
		}
	}

	Map_fixed& operator=(Map_fixed const& a){
		size_t i=0;
		for(;i<size_ && i<a.size_;i++){
			*(begin()+i)=*(a.begin()+i);
		}
		if(i<=a.size_){
			for(;i<a.size_;i++){
				new(begin()+i) P(*(a.begin()+i));
			}
			size_=a.size_;
		}else{
			for(;i<size_;i++){
				delete(begin()+i);
			}
			size_=a.size_;
		}
		return *this;
	}

	Map_fixed(std::map<K,V> const& a){
		for(auto p:a){
			assert(size_<N);
			new(begin()) P(p);
			size_++;
		}
	}

	V& operator[](K k){
		auto f=find(k);
		if(f!=end()){
			return f->second;
		}
		if(size_>=N){
			std::cout<<"size_"<<size_<<"\n";
			std::cout<<*this<<"\n";
		}
		assert(size_<N);
		new(begin()+size_) P(k,V{});
		size_++;
		return (begin()+size_-1)->second;
	}

	using iterator=P*;

	iterator find(K k){
		for(auto at=begin();at!=end();++at){
			if(at->first==k){
				return at;
			}
		}
		return end();
	}

	iterator begin(){ return (P*)buf; }
	iterator end(){ return begin()+size_; }

	using const_iterator=const P*;

	const_iterator begin()const{
		return (P*)buf;
	}

	const_iterator end()const{
		return begin()+size_;
	}

	auto operator<=>(Map_fixed const&)const=default;

	Map_fixed operator+(Map_fixed const&)const{
		
		nyi
	}
};


template<typename K,typename V,size_t N>
std::ostream& operator<<(std::ostream& o,Map_fixed<K,V,N> const& m){
	o<<"{ ";
	for(auto x:m){
		o<<x<<" ";
	}
	return o<<"}";
}

template<typename K,typename V,size_t N>
std::vector<V> values(Map_fixed<K,V,N> const& a){
	std::vector<V> r;
	for(auto x:a){
		r|=x.second;
	}
	return r;
}

#endif
