#ifndef VECTOR_H
#define VECTOR_H
#include <initializer_list>

template <typename T>
class Vector {
    public:
        // Implementation des constructeurs
        ~Vector();
        Vector() noexcept;
        Vector(size_t Count, const T& Value);
        explicit Vector(size_t Count);
        Vector(const Vector &Other);
        Vector(Vector &&Other) noexcept;
        Vector(std::initializer_list<T> init);
        Vector &operator=(const Vector &Other);
        Vector &operator=(Vector &&Other) noexcept;
        Vector &operator=(std::InitializerList<T> init);

    private:
        size_t m_size;
        T *m_data;
        size_t m_capacity;


};


#endif
