#pragma once

template <class T>
std::unique_ptr<typename Singleton<T>::TIns> Singleton<T>::s_instance;

template <class T>
T* Singleton<T>::GetInstance()
{
	static std::once_flag flag;
	std::call_once(flag, [] { s_instance = std::make_unique<TIns>(TIns()); });
	return s_instance.get();
}

template <class T>
void Singleton<T>::ClearInstance()
{
	static std::once_flag flag;
	std::call_once(flag, [] { s_instance.reset(); });
}
