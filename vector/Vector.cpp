#include <new>
#include "Vector.h"

template<typename T>
Vector<T>::Vector() noexcept {
    m_data = new(std::nothrow) T[m_capacity];
    m_size = 0;
}

template<typename T>
Vector<T>::Vector(size_t Count, const T &Value) {
    m_size = Count;
    m_capacity = Count * 2; // Pour l'allocation dynamique
    m_data = new(std::nothrow) T[m_capacity];
    for (size_t i = 0; i < m_capacity; i++)
        m_data[i] = Value;
}

template<typename T>
Vector<T>::Vector(size_t Count) {
    m_size = Count;
    m_capacity = Count;
    m_data = new(std::nothrow) T[m_capacity];
    memset(m_data, T{}, m_capacity * sizeof(T));
    //memset car remplissage par valeur par defaut
}

template<typename T>
Vector<T>::Vector(const Vector &Other) {
    m_size = Other.m_size;
    m_capacity = Other.m_capacity;
    m_data = new(std::nothrow) T[m_capacity];
    if (Other.m_data != nullptr) {
        for (size_t i = 0; i < m_capacity; i++) {
            m_data[i] = Other.m_data[i];
        }
    }
    //Choix de la copie case par case car memcpy ne gere pas correctement les types complexes
}

template<typename T>
Vector<T>::Vector(Vector &&Other) noexcept {
    m_size = Other.m_size;
    m_capacity = Other.m_capacity;
    m_data = Other.m_data;
    Other.m_data = nullptr;
}

template<typename T>
Vector<T>::Vector(std::initializer_list<T> init) {
    m_size = init.size();
    m_capacity = init.size() * 2;
    m_data = new(std::nothrow) T[m_capacity];
    if (init.empty()) {
        delete[] m_data;
        return;
    }
    for (size_t i = 0; i < init.size(); i++) {
        m_data[i] = init[i];
    }
}

template<typename T>
Vector<T> &Vector<T>::operator=(const Vector &Other)
{
    this.m_size = Other.m_size;
    this.m_capacity = Other.m_size * 2;
    this.m_data = new(std::nothrow) T[this.m_capacity];
    for (size_t i = 0; i < this.m_size; i++)
    {
        this.m_data[i] = Other.m_data[i];
    }
    
    return *this;
}

template <typename T>
Vector<T> &Vector<T>::operator=(Vector &&Other)
{
    this.m_size = Other.m_size;
    this.m_capacity = Other.m_capacity;
    this.m_data = Other.m_data;
    Other.m_data = nullptr;

    return *this;
}

template <typename T>
Vector<T> &Vector<T>::operator=(std::initializer_list<T> init)
{
    this.m_size = init.size();
    this.m_capacity = init.size() * 2;
    this.m_data = new (std::nothrow) T[this.m_capacity];

    for (size_t i = 0; i < init.size(); i++)
    {
        this.m_data[i] = init[i];
    }
    return *this;
    
}