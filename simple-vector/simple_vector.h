#pragma once

#include <cassert>
#include <stdexcept>
#include <utility>
#include <algorithm>
#include "array_ptr.h"

using namespace std;

// Класс-обертка для различия версий конструкторов с параметрами size и reserve (оба типа size_t)
class ReserveProxyObj {
    size_t capacity = 0;
public:

    ReserveProxyObj() = delete;

    explicit ReserveProxyObj(size_t new_capacity) : capacity(new_capacity) {}

    size_t GetCapacity() { return capacity; }
};

// Функция, создающая прокси-объект для передачи его в конструктор SimpleVector
ReserveProxyObj Reserve(size_t capacity_to_reserve) {
    return ReserveProxyObj(capacity_to_reserve);
}

template <typename Type>
class SimpleVector
{
    ArrayPtr<Type> arr_;

    size_t size_ = 0;         // Количество элементов в векторе
    size_t capacity_ = 0;     // Выделено памяти в векторе

    void ChangeCapacity(const size_t new_size) {
        SimpleVector new_arr(new_size);
        size_t prev_size = size_;
        std::move(begin(), end(), new_arr.begin());
        swap(std::move(new_arr));
        size_ = prev_size;
    }

public:
    using Iterator = Type*;
    using ConstIterator = const Type*;

    SimpleVector() noexcept = default;

    // Создаёт вектор из size элементов, инициализированных значением по умолчанию
    explicit SimpleVector(size_t size) :
        SimpleVector(size, std::move(Type{}))
    {
    }

    // Создаёт вектор из size элементов, инициализированных значением value
    SimpleVector(size_t size, const Type& value) :
        arr_(size),
        size_(size),
        capacity_(size)
    {
        std::fill(begin(), end(), value);
    }

    SimpleVector(size_t size, Type&& value) :
        arr_(size),
        size_(size),
        capacity_(size)
    {
        for (auto it = arr_.Get(); it != arr_.Get() + size; ++it) {
            *it = std::move(value);
            value = Type{}; // Обновляем значение переменной на значение по умолчанию (перемещение)
        }
    }

    // Создаёт вектор из std::initializer_list
    SimpleVector(std::initializer_list<Type> init) :
        arr_(init.size()),
        size_(init.size()),
        capacity_(init.size())
    {
        std::move(init.begin(), init.end(), arr_.Get());
    }

    SimpleVector(const SimpleVector& other)
    {
        SimpleVector tmp(other.GetSize());
        std::copy(other.begin(), other.end(), tmp.begin());
        tmp.capacity_ = other.GetCapacity();
        swap(tmp);
    }

    // move-constructor
    SimpleVector(SimpleVector&& other) noexcept :
        arr_(other.size_)
    {
        if (this != &other)
        {
            arr_.swap(other.arr_);
            size_ = std::move(other.size_);
            capacity_ = std::move(other.capacity_);
            other.Clear(); // Очистка вектора после перемещения
        }

    }

    SimpleVector(ReserveProxyObj proxy) :
        arr_(proxy.GetCapacity()),
        capacity_(proxy.GetCapacity())
    {
    }

    SimpleVector& operator=(const SimpleVector& rhs) {
        if (this != &rhs) {
            if (rhs.IsEmpty())
                Clear();
            else {
                SimpleVector copy(rhs);
                swap(copy);
            }
        }
        return *this;
    }

    SimpleVector& operator=(SimpleVector&& rhs) noexcept {
        if (this != &rhs) {
            if (rhs.IsEmpty())
                Clear();
            else {
                swap(std::move(rhs));
                rhs.Clear(); // Очистка вектора после перемещения
            }
        }
        return *this;
    }

    // Возвращает ссылку на элемент с индексом index
    Type& operator[](size_t index) noexcept {
        assert(index < size_);
        return *(begin() + index);
    }

    // Возвращает константную ссылку на элемент с индексом index
    const Type& operator[](size_t index) const noexcept {
        assert(index < size_);
        return *(begin() + index);
    }

    // Возвращает количество элементов в массиве
    size_t GetSize() const noexcept { return size_; }

    // Возвращает вместимость массива
    size_t GetCapacity() const noexcept { return capacity_; }

    // Сообщает, пустой ли массив
    bool IsEmpty() const noexcept { return (size_ == 0); }

    // Возвращает константную ссылку на элемент с индексом index
    // Выбрасывает исключение std::out_of_range, если index >= size
    Type& At(size_t index) {
        if (index >= size_)
            throw std::out_of_range("Invalid index");
        return *(begin() + index);
    }

    // Возвращает константную ссылку на элемент с индексом index
    // Выбрасывает исключение std::out_of_range, если index >= size
    const Type& At(size_t index) const {
        if (index >= size_)
            throw std::out_of_range("Invalid index");
        return *(begin() + index);
    }

    // Обнуляет размер массива, не изменяя его вместимость
    void Clear() noexcept { size_ = 0; }

    // Изменяет размер массива.
    // При увеличении размера новые элементы получают значение по умолчанию для типа Type
    void Resize(size_t new_size) {
        if (new_size <= size_) {
            size_ = new_size;
            return;
        }
        if (new_size <= capacity_) {
            // Забиваем стандартным значением добавленные после size_ элементы
            for (auto it = begin() + size_; it != begin() + new_size; ++it) {
                *it = std::move(Type{});
            }
        }
        else
            ChangeCapacity(new_size);
        size_ = new_size;
    }

    // Добавляет элемент в конец вектора
    // При нехватке места увеличивает вдвое вместимость вектора
    void PushBack(const Type& item) {
        if (size_ == capacity_) {
            size_ != 0 ? ChangeCapacity(2 * size_) : ChangeCapacity(1); // Если вектор пустой, то изменяем вместимость на 1
        }
        arr_[size_] = item;
        size_++;
    }

    void PushBack(Type&& item) {
        if (size_ == capacity_) {
            size_ != 0 ? ChangeCapacity(2 * size_) : ChangeCapacity(1); // Если вектор пустой, то изменяем вместимость на 1
        }
        arr_[size_] = std::move(item);
        item = std::move(Type{});
        size_++;
    }

    // Вставляет значение value в позицию pos.
    // Возвращает итератор на вставленное значение
    // Если перед вставкой значения вектор был заполнен полностью,
    // вместимость вектора должна увеличиться вдвое, а для вектора вместимостью 0 стать равной 1
    Iterator Insert(ConstIterator pos, const Type& value) {
        assert(pos >= begin() && pos <= end());
        auto index = std::distance(cbegin(), pos);
        if (size_ == capacity_)
            size_ != 0 ? ChangeCapacity(2 * size_) : ChangeCapacity(1); // Если вектор пустой, то изменяем вместимость на 1
        auto it = begin() + index;
        std::copy_backward(it, end(), end() + 1);   // Сдвигаем элменеты на один вправо
        arr_[index] = std::move(value);
        ++size_;
        return Iterator(begin() + index);
    }

    Iterator Insert(ConstIterator pos, Type&& value) {
        assert(pos >= begin() && pos <= end());
        auto index = std::distance(cbegin(), pos);
        if (size_ == capacity_)
            size_ != 0 ? ChangeCapacity(2 * size_) : ChangeCapacity(1); // Если вектор пустой, то изменяем вместимость на 1
        auto it = begin() + index;
        std::move_backward(it, end(), end() + 1);
        arr_[index] = std::move(value);
        ++size_;
        value = std::move(Type{});
        return Iterator(begin() + index);
    }

    // "Удаляет" последний элемент вектора. Вектор не должен быть пустым
    void PopBack() noexcept {
        if (IsEmpty()) return;
        --size_;
    }

    // Удаляет элемент вектора в указанной позиции
    Iterator Erase(ConstIterator pos) {
        assert(pos >= begin() && pos < end());
        auto index = std::distance(cbegin(), pos);
        auto it = begin() + index;
        std::move((it + 1), end(), it);
        --size_;
        return Iterator(pos);
    }

    // Обменивает значение с другим вектором
    void swap(SimpleVector& other) noexcept {
        arr_.swap(other.arr_);
        std::swap(size_, other.size_);
        std::swap(capacity_, other.capacity_);
    }

    void swap(SimpleVector&& other) noexcept {
        arr_.swap(other.arr_);
        std::swap(size_, other.size_);
        std::swap(capacity_, other.capacity_);
        other.Clear();
    }

    // Увеличивает емкость вектора
    void Reserve(size_t new_capacity) {
        if (capacity_ < new_capacity)
            ChangeCapacity(new_capacity);
    }

    // Возвращает итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    Iterator begin() noexcept { return arr_.Get(); }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    Iterator end() noexcept { return arr_.Get() + size_; }

    // Возвращает константный итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator begin() const noexcept { return ConstIterator(arr_.Get()); }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator end() const noexcept { return ConstIterator(arr_.Get() + size_); }

    // Возвращает константный итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator cbegin() const noexcept { return begin(); }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator cend() const noexcept { return end(); }
};

template <typename Type>
bool operator==(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs)
{
    return lhs.GetSize() == rhs.GetSize() && std::equal(lhs.cbegin(), lhs.cend(), rhs.cbegin(), rhs.cend());
}

template <typename Type>
bool operator!=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs)
{
    return !(lhs == rhs);
}

template <typename Type>
bool operator<(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs)
{
    return std::lexicographical_compare(lhs.cbegin(), lhs.cend(), rhs.cbegin(), rhs.cend());
}

template <typename Type>
bool operator<=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs)
{
    return !(rhs < lhs);
}

template <typename Type>
bool operator>(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs)
{
    return rhs < lhs;
}

template <typename Type>
bool operator>=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs)
{
    return !(rhs > lhs);
}