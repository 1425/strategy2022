#ifndef ARRAY_H
#define ARRAY_H

template<typename T,size_t LEN>
std::ostream& operator<<(std::ostream& o,std::array<T,LEN> const& a){
	o<<"[ ";
	std::ostream_iterator<T> out_it(o," ");
	std::copy(begin(a),end(a),out_it);
	return o<<"]";
}

template<size_t LEN>
std::array<size_t,LEN> range_st(){
	std::array<size_t,LEN> r;
	std::iota(begin(r),end(r),0);
	return r;
}

template<typename T,size_t LEN>
T sum(std::array<T,LEN> const& a){
	return std::accumulate(begin(a),end(a),T{});
}

template<typename Func,typename T,size_t LEN>
std::vector<T> filter(Func f,std::array<T,LEN> const& t){
	std::vector<T> r;
	std::copy_if(begin(t),end(t),back_inserter(r),f);
	return r;
}

template<typename Func,typename T,size_t LEN>
auto mapf(Func f,std::array<T,LEN> const& v) -> std::array< decltype(f(v[0])) , LEN> {
	std::array<decltype(f(v[0])),LEN> r;
	std::transform(begin(v),end(v),begin(r),f);
	return r;
}

template<typename A,typename B,size_t LEN>
std::array<std::pair<A,B>,LEN> zip(std::array<A,LEN> const& a,std::array<B,LEN> const& b){
	std::array<std::pair<A,B>,LEN> r;
	std::transform(
		begin(a),end(a),begin(b),begin(r),
		[](auto a1,auto b1){ return std::make_pair(a1,b1); }
	);
	return r;
}

template<typename A,typename B,size_t LEN>
std::vector<std::pair<A,B>> zip(std::vector<A> const& a,std::array<B,LEN> const& b){
	std::vector<std::pair<A,B>> r;
	std::transform(
		begin(a),begin(a)+std::min(a.size(),LEN),begin(b),back_inserter(r),
		[](auto a1,auto b1){ return std::make_pair(a1,b1); }
	);
	return r;
}

template<typename T,size_t LEN>
std::array<T,LEN> sorted(std::array<T,LEN> a){
	std::sort(begin(a),end(a));
	return a;
}

template<typename T>
std::vector<std::tuple<T,T,T>> cross3(std::array<std::vector<T>,3> in){
	std::vector<std::tuple<T,T,T>> r;
	for(auto a:in[0]){
		for(auto b:in[1]){
			for(auto c:in[2]){
				r|=make_tuple(a,b,c);
			}
		}
	}
	return r;
}

template<typename T,size_t N>
T max(std::array<T,N> const& a){
	static_assert(N);
	T r=a[0];
	for(auto i:range(size_t(1),N)){
		r=std::max(r,a[i]);
	}
	return r;
}

template<size_t X,typename T,size_t N>
std::array<T,X> take(std::array<T,N> const& a){
	assert(X<=N);
	std::array<T,X> r;
	for(auto i:range_st<X>()){
		r[i]=a[i];
	}
	return r;
}

template<size_t X,typename T,size_t N>
std::array<T,N-X> skip(std::array<T,N> const& a){
	std::array<T,N-X> r;
	for(auto i:range_st<N-X>()){
		r[i]=a[i+X];
	}
	return r;
}

template<typename T,size_t N>
std::vector<T> to_vec(std::array<T,N> const& a){
	return std::vector<T>(a.begin(),a.end());
}

template<size_t N>
auto join(std::array<std::string,N> const& a){
	return join(to_vec(a));
}

template<typename T,size_t N>
std::string join(std::string const& delim,std::array<T,N> const& a){
	if(N==0) return "";

	std::stringstream ss;
	ss<<a[0];
	for(auto i:range(size_t(1),N)){
		ss<<delim<<a[i];
	}
	return ss.str();
}

template<typename T,size_t N>
std::array<T,N> reversed(std::array<T,N> a){
	std::reverse(begin(a),end(a));
	return a;
}

template<typename A,typename B,size_t N>
std::array<B,N> seconds(std::array<std::pair<A,B>,N> const& a){
	return mapf([](auto x){ return x.second; },a);
}

template<typename A,typename B,typename C,size_t N>
std::array<std::tuple<A,B,C>,N> zip(std::array<A,N> const& a,std::array<B,N> const& b,std::array<C,N> const& c){
	std::array<std::tuple<A,B,C>,N> r;
	for(auto i:range(N)){
		r[i]=std::make_tuple(a[i],b[i],c[i]);
	}
	return r;
}

#endif
