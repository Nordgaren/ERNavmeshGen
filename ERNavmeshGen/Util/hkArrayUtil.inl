#pragma once

template <typename T>
bool hkArrayManager::Register(hkArray<T>* array)
{
	return hkArrayManagerT<T>::GetInstance()->Register(array);
}

template <typename T>
bool hkArrayManager::Deregister(hkArray<T>* array)
{
	return hkArrayManagerT<T>::GetInstance()->Deregister(array);
}

template <typename T, typename... TArgs>
void hkArrayManager::CreateManaged(hkArray<T>* array, TArgs&&... args)
{
	hkArrayManagerT<T>::GetInstance()->CreateManaged(array, std::forward<TArgs>(args)...);
}

template <typename T>
void hkArrayManager::PushBack(hkArray<T>* array, T value)
{
	return hkArrayManagerT<T>::GetInstance()->PushBack(array, value);
}

template <typename T>
bool hkArrayManager::hkArrayManagerT<T>::Register(hkArray<T>* array)
{
	std::vector<T> vector { array->data, array->data + array->size };
	if (!m_arrays.emplace(array, vector).second) return false;
	Synchronize(vector, array);
	return true;
}

template <typename T>
bool hkArrayManager::hkArrayManagerT<T>::Deregister(hkArray<T>* array)
{
	return m_arrays.erase(array);
}

template <typename T>
template <typename... TArgs>
void hkArrayManager::hkArrayManagerT<T>::CreateManaged(hkArray<T>* array, TArgs&&... args)
{
	m_arrays.try_emplace(array, std::forward<TArgs>(args)...);
	std::vector<T>& vector = m_arrays.at(array);
	Synchronize(vector, array);
}

template <typename T>
void hkArrayManager::hkArrayManagerT<T>::PushBack(hkArray<T>* array, T value)
{
	if (!m_arrays.contains(array)) Register(array);

	Synchronize(array);
	std::vector<T>& vector = m_arrays.at(array);
	vector.push_back(value);
	Synchronize(vector, array);
}

template <typename T>
void hkArrayManager::hkArrayManagerT<T>::Synchronize(hkArray<T>* array)
{
	std::vector<T>& vector = m_arrays.at(array);
	if (vector.begin()._Ptr != array->data || vector.size() != array->size)
	{
		vector.assign(array->data, array->data + array->size);
	}
}

template <typename T>
void hkArrayManager::hkArrayManagerT<T>::Synchronize(std::vector<T>& vector, hkArray<T>* array)
{
	array->data = vector.begin()._Ptr;
	array->size = (uint32_t)vector.size();
	array->capacityAndFlags = (uint32_t)vector.capacity() | DONT_DEALLOCATE;
}
