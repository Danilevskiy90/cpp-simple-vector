#pragma once

#include <cassert>
#include <algorithm>
#include <initializer_list>
#include <stdexcept>
#include <iterator>
#include <utility>
#include <iostream>

#include "array_ptr.h"

class ReserveProxyObj 
{
public:
    ReserveProxyObj(const size_t capacity_to_reserve) 
    :reverse_assistant_(capacity_to_reserve)
    {}
 
    size_t GetSize() 
    { 
        return reverse_assistant_; 
    }
    
private:
    size_t reverse_assistant_;
    
};

ReserveProxyObj Reserve(const size_t capacity_to_reserve) 
{
    return ReserveProxyObj(capacity_to_reserve);
}

template <typename Type>
class SimpleVector 
{
public:
    using Iterator = Type*;
    using ConstIterator = const Type*;

    SimpleVector() noexcept = default;

    // Создаёт вектор из size элементов, инициализированных значением по умолчанию
    SimpleVector(size_t size) : SimpleVector(size, Type()) 
    { }

    SimpleVector(ReserveProxyObj obj) 
    {
        SimpleVector<Type> vec;
        vec.Reserve(obj.GetSize());
        swap(vec);
    }

    // Создаёт вектор из size элементов, инициализированных значением value
    SimpleVector(size_t size, const Type& value)
        : ptr_(size), size_(size), capacity_(size) 
    {
        std::fill(ptr_.Get(), ptr_.Get() + size, value);
    }


    void Reserve(size_t capacity_to_reserve) 
    {
        if (capacity_to_reserve > capacity_) 
        {
            SimpleVector<Type> tmp_items(capacity_to_reserve);
            std::copy(cbegin(), cend(), tmp_items.begin());
            tmp_items.size_ = size_;
            swap(tmp_items);
        }
    }

    // Создаёт вектор из std::initializer_list
    SimpleVector(std::initializer_list<Type> init) 
        : size_(init.size()), capacity_(init.size())
    {
        ArrayPtr<Type> tmp_ptr(init.size());
        std::copy(std::make_move_iterator(init.begin()), 
                  std::make_move_iterator(init.end()), 
                  tmp_ptr.Get());
                  
        ptr_.swap(tmp_ptr);
    }

    // Конструктор копирования                          /** Такой вопрос: а как можно использовать перемещающие конструктор и оператор присваивания 
                                                        /** умного указателя в конструкторе вектора? */
    SimpleVector(const SimpleVector& other) 
        :  size_(other.size_), capacity_(other.size_)
    {
        ArrayPtr<Type> tmp_ptr(other.GetCapacity());
        std::copy(std::make_move_iterator(other.cbegin()), 
                  std::make_move_iterator(other.cend()), 
                  tmp_ptr.Get());

        ptr_.swap(tmp_ptr);
    }

    /* move-конструктор копирования */
    SimpleVector(SimpleVector&& other) 
    {
        ptr_.swap(other.ptr_);
        size_ = std::exchange(other.size_, 0);
        capacity_ = std::exchange(other.capacity_, 0);
    }
    
    SimpleVector& operator=(const SimpleVector& rhs) 
    {
        if(this != &rhs)
        {
            SimpleVector tmp(rhs);
            swap(tmp);
        }
        return *this;
    }

    SimpleVector& operator=(SimpleVector&& other)
    {
        ptr_.swap(other.ptr_);

        size_     = std::exchange(other.size_, 0);
        capacity_ = std::exchange(other.capacity_, 0);
        return *this;
    }
    
    // Обменивает значение с другим вектором
    void swap(SimpleVector& other) noexcept 
    {
        std::swap(size_, other.size_);
        std::swap(capacity_, other.capacity_);
        ptr_.swap(other.ptr_);
    }

    // Добавляет элемент в конец вектора
    // При нехватке места увеличивает вдвое вместимость вектора
    void PushBack(const Type& item) 
    {
        if ( size_ == capacity_ )
        {
            Resize(size_+1);
        }
        else
        {
            size_++;
        }

        ptr_[size_-1] = item;
    }

    void PushBack( Type&& item )
    {
        if ( size_ == capacity_ )
        {
            Resize(size_+1);
        }
        else
        {
            size_++;
        }

        ptr_[size_-1] = std::exchange(item, Type());
    }

    // "Удаляет" последний элемент вектора. Вектор не должен быть пустым
    void PopBack() noexcept 
    {
        assert(size_ > 0);
        size_--;
    }

    // Вставляет значение value в позицию pos.
    // Возвращает итератор на вставленное значение
    // Если перед вставкой значения вектор был заполнен полностью,
    // вместимость вектора должна увеличиться вдвое, а для вектора вместимостью 0 стать равной 1
    Iterator Insert(ConstIterator pos, const Type& value) 
    {
        assert(pos >= begin() && pos <= end());
        auto step_ = pos - begin();

        if (capacity_ == size_)  /** не совсем понял, что тут стоит проверить на assert: размер ведь может быть равен вместимости, тогда
                                  *  тогда мы просто вызываем ReCapacity, а проверять  size_ на ноль бессмысленно на мой взгляд, ведь мы можем вставить элемент в 
                                  *  пустой вектор. Поясните пожалуйста) */
        {
            ReCapacity(size_+1);
        }

        std::move_backward(begin() + step_, end(), end() + 1);

        auto* it = begin() + step_;
        *it = std::move(value);
        size_++;
        return it;
    }

    Iterator Insert(ConstIterator pos, Type&& value) 
    {
        assert(pos >= begin() && pos <= end());
        auto step_ = pos - begin();

        if (capacity_ == size_) 
        {
            ReCapacity(size_+1);
        }

        std::move_backward(begin() + step_, end(), end() + 1);

        auto* it = begin() + step_;
        *it = std::move(value);
        size_++;
        return it;
    }

    // Удаляет элемент вектора в указанной позиции
    Iterator Erase(ConstIterator pos) 
    {
        assert(size_ > 0);

        auto step_ = pos - begin();
        auto* it = begin() + step_;
        std::move((it + 1), end(), it);
        size_--;
        
        return (begin() + step_);
    }

    // Возвращает количество элементов в массиве
    size_t GetSize() const noexcept 
    {
        return size_;
    }

    // Возвращает вместимость массива
    size_t GetCapacity() const noexcept 
    {
        return capacity_;
    }

    // Сообщает, пустой ли массив
    bool IsEmpty() const noexcept 
    {
        return size_ == 0;
    }

    // Возвращает ссылку на элемент с индексом index
    Type& operator[](size_t index) noexcept 
    {
        return *(ptr_.Get() + index);
    }

    // Возвращает константную ссылку на элемент с индексом index
    const Type& operator[](size_t index) const noexcept 
    {
        return *(ptr_.Get() + index);
    }

    // Возвращает константную ссылку на элемент с индексом index
    // Выбрасывает исключение std::out_of_range, если index >= size
    Type& At(size_t index) const
    {
        if ( index < size_ )
        {
            return *(ptr_.Get() + index);
        }
        else
        {
            throw std::out_of_range("Segmentation fault: .At");
        }
    }

    // Обнуляет размер массива, не изменяя его вместимость
    void Clear() noexcept
    {
        size_ = 0;
    }

    // Изменяет размер массива.
    // При увеличении размера новые элементы получают значение по умолчанию для типа Type
    void Resize(size_t new_size) 
    {
        if (size_ < new_size) 
        {
            if (new_size <= capacity_)
            {
                auto beg_it = ptr_.Get() + size_, end_it = ptr_.Get() + new_size;

                while( beg_it != end_it )
                {
                    std::exchange(*beg_it, Type());
                    beg_it++;
                }
            }
            else
            {
                ReCapacity(new_size * 2);
            }
        }
        size_ = new_size;
    }
    // Возвращает итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    Iterator begin() noexcept 
    {
        return ptr_.Get();
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    Iterator end() noexcept 
    {
        return ptr_.Get() + size_;
    }

    // Возвращает константный итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator begin() const noexcept 
    {
        return ptr_.Get();
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator end() const noexcept 
    {
        return ptr_.Get() + size_;
    }

    // Возвращает константный итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator cbegin() const noexcept 
    {
        return begin();
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator cend() const noexcept 
    {
        return end();
    }


private:
    ArrayPtr<Type> ptr_;
    size_t size_ = 0;
    size_t capacity_ = 0;

    /* Изменить емкость вектора */
    void ReCapacity(size_t new_capacity) 
    {
        ArrayPtr<Type> new_cont_(new_capacity);
        std::move(ptr_.Get(), ptr_.Get() + size_, new_cont_.Get());
        ptr_.swap(new_cont_);
        capacity_ = new_capacity;
    }
};
template <typename Type>
inline bool operator==(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) 
{
    if ( lhs.GetSize() == rhs.GetSize() )
    {
        return std::equal(lhs.begin(), lhs.end(), rhs.begin());
    }
    else    
    {
        return false;
    }
}

template <typename Type>
inline bool operator!=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) 
{
    return !(lhs == rhs);
}

template <typename Type>
inline bool operator<(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) 
{
    return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end()); 
}

template <typename Type>
inline bool operator<=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) 
{
    return !(rhs < lhs); 
}

template <typename Type>
inline bool operator>(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) 
{
    return (rhs < lhs);
}

template <typename Type>
inline bool operator>=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) 
{
    return !(lhs < rhs);
} 