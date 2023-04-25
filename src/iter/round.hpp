#pragma once
namespace Iter{
class round{
	int value;
	const int init;
	bool exceeded = false;
	const int maximal;
	public:
	round(int current,int total):value(current),maximal(total),init(current){}
	auto& begin(){return *this;}
	auto& end()const{return *this;}
	int operator*()const{return value;}
	bool operator==(const round&){return exceeded && value == init;}
	void operator++(){
		++value;
		if (value >= maximal){value = 0;exceeded = true;} 
	}
};


}
