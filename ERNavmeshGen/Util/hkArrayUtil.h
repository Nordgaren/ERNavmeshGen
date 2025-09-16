#pragma once
#include "Singleton.h"

#include <unordered_map>
#include "../GhidraStructs/GhidraStructs.h"

class hkArrayManager {
public:
	template <typename T>
	static bool Register(hkArray<T>* array);

	template <typename T>
	static bool Deregister(hkArray<T>* array);

	template <typename T, typename... TArgs>
	static void CreateManaged(hkArray<T>* array, TArgs&&...);

	template <typename T>
	static void PushBack(hkArray<T>* array, T value);

private:
	template <typename T>
	class hkArrayManagerT : public Singleton<hkArrayManagerT<T>> {
	public:
		bool Register(hkArray<T>* array);
		bool Deregister(hkArray<T>* array);

		template <typename... TArgs>
		void CreateManaged(hkArray<T>* array, TArgs&&...);

		void PushBack(hkArray<T>* array, T value);

	protected:
		hkArrayManagerT() = default;

	private:
		//synchronize the std::vector with the hkArray
		void Synchronize(hkArray<T>* array);

		//synchronize the hkArray with the std::vector
		void Synchronize(std::vector<T>& vector, hkArray<T>* array);

		std::unordered_map<hkArray<T>*, std::vector<T>> m_arrays;
	};
};


#include "hkArrayUtil.inl"
