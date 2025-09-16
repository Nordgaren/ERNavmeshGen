#pragma once
#include <mutex>

//Thread safe singleton base class
template<class T>
class Singleton {
public:
	static T* GetInstance();
	static void ClearInstance();

protected:
	Singleton() = default;

private:
	struct TIns : T {};
	static std::unique_ptr<TIns> s_instance;
};

#include "Singleton.inl"
