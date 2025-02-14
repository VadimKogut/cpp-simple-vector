#pragma once
#include <cassert>
#include <initializer_list>
#include <stdexcept>
#include <algorithm>
#include <utility> // Для std::move
#include "array_ptr.h" // Подключаем ArrayPtr

// Класс-объект для резервирования
class ReserveProxyObj {
public:
    explicit ReserveProxyObj(size_t capacity)
        : capacity_(capacity) {}

    size_t GetCapacity() const noexcept {
        return capacity_;
    }

private:
    size_t capacity_;
};

// Вспомогательная функция для создания ReserveProxyObj
ReserveProxyObj Reserve(size_t capacity) {
    return ReserveProxyObj(capacity);
}

template <typename Type>
class SimpleVector {
public:
    using Iterator = Type*;
    using ConstIterator = const Type*;

    // Конструктор по умолчанию
    SimpleVector() noexcept = default;

    // Конструктор с заданным размером
    explicit SimpleVector(size_t size)
        : size_(size), capacity_(size), data_(size == 0 ? nullptr : new Type[size]) {
        std::fill_n(data_.Get(), size_, Type());
    }

    // Конструктор с резервированием ёмкости
    explicit SimpleVector(ReserveProxyObj obj)
        : capacity_(obj.GetCapacity()), data_(obj.GetCapacity() == 0 ? nullptr : new Type[obj.GetCapacity()]) {}

    // Конструктор с заданным размером и значением
    SimpleVector(size_t size, const Type& value)
        : size_(size), capacity_(size), data_(size == 0 ? nullptr : new Type[size]) {
        std::fill_n(data_.Get(), size_, value);
    }

    // Конструктор с initializer_list
    SimpleVector(std::initializer_list<Type> init)
        : size_(init.size()), capacity_(init.size()), data_(init.size() == 0 ? nullptr : new Type[init.size()]) {
        std::copy(init.begin(), init.end(), data_.Get());
    }

    // Деструктор (не нужен, так как ArrayPtr сам освобождает память)
    ~SimpleVector() = default;

    // Конструктор копирования
    SimpleVector(const SimpleVector& other)
        : size_(other.size_), capacity_(other.size_), data_(other.size_ == 0 ? nullptr : new Type[other.size_]) {
        std::copy(other.data_.Get(), other.data_.Get() + other.size_, data_.Get());
    }

    // Конструктор перемещения
    SimpleVector(SimpleVector&& other) noexcept
        : size_(std::exchange(other.size_, 0)),
          capacity_(std::exchange(other.capacity_, 0)),
          data_(std::move(other.data_)) {}

    // Оператор присваивания
    SimpleVector& operator=(const SimpleVector& other) {
        if (this == &other) {
            return *this; // Защита от самоприсваивания
        }

        SimpleVector tmp(other); // Используем конструктор копирования
        swap(tmp); // Обмениваем содержимое
        return *this;
    }

    // Оператор перемещающего присваивания
    SimpleVector& operator=(SimpleVector&& other) noexcept {
        if (this != &other) {
            size_ = std::exchange(other.size_, 0);
            capacity_ = std::exchange(other.capacity_, 0);
            data_ = std::move(other.data_);
        }
        return *this;
    }

    // Оператор индексирования
    Type& operator[](size_t index) noexcept {
        assert(index < size_ && "Index out of range");
        return data_[index];
    }

    const Type& operator[](size_t index) const noexcept {
        assert(index < size_ && "Index out of range");
        return data_[index];
    }

    // Метод At с проверкой границ
    Type& At(size_t index) {
        if (index >= size_) {
            throw std::out_of_range("Index out of range");
        }
        return data_[index];
    }

    const Type& At(size_t index) const {
        if (index >= size_) {
            throw std::out_of_range("Index out of range");
        }
        return data_[index];
    }

    // Изменение размера вектора
    void Resize(size_t new_size) {
        if (new_size == size_) {
            return;
        }

        if (new_size > capacity_) {
            size_t new_capacity = new_size;
            ArrayPtr<Type> new_data(new_capacity);

            std::move(data_.Get(), data_.Get() + size_, new_data.Get());

            for (size_t i = size_; i < new_size; ++i) {
                new_data[i] = Type();
            }

            data_ = std::move(new_data);
            capacity_ = new_capacity;
        }
        else {
            for (size_t i = new_size; i < size_; ++i) {
                data_[i] = Type();
            }
        }

        size_ = new_size;
    }

    // Итераторы
    Iterator begin() noexcept {
        return data_.Get();
    }

    Iterator end() noexcept {
        return data_.Get() + size_;
    }

    ConstIterator begin() const noexcept {
        return data_.Get();
    }

    ConstIterator end() const noexcept {
        return data_.Get() + size_;
    }

    ConstIterator cbegin() const noexcept {
        return data_.Get();
    }

    ConstIterator cend() const noexcept {
        return data_.Get() + size_;
    }

    // Получение размера
    size_t GetSize() const noexcept {
        return size_;
    }

    // Получение емкости
    size_t GetCapacity() const noexcept {
        return capacity_;
    }

    // Проверка на пустоту
    bool IsEmpty() const noexcept {
        return size_ == 0;
    }

    // Очистка вектора
    void Clear() noexcept {
        size_ = 0;
    }

     // Добавление элемента в конец
    void PushBack(const Type& item) {
        if (size_ == capacity_) {
            size_t new_capacity = capacity_ == 0 ? 1 : capacity_ * 2;
            Type* new_data = new Type[new_capacity];
            std::copy(data_, data_ + size_, new_data);
            delete[] data_;
            data_ = new_data;
            capacity_ = new_capacity;
        }
        data_[size_] = item;
        ++size_;
    }

    // Вставка элемента
    Iterator Insert(ConstIterator pos, const Type& value) {
        if (pos < begin() || pos > end()) {
            throw std::out_of_range("Insert position out of range");
        }

        // Вычисляем индекс для вставки
        size_t insert_index = pos - begin();

        // Если нужно увеличить ёмкость
        if (size_ == capacity_) {
            size_t new_capacity = capacity_ == 0 ? 1 : capacity_ * 2;
            Type* new_data = new Type[new_capacity];

            // Копируем элементы до позиции вставки
            std::copy(data_, data_ + insert_index, new_data);

            // Вставляем новый элемент
            new_data[insert_index] = value;

            // Копируем оставшиеся элементы
            std::copy(data_ + insert_index, data_ + size_, new_data + insert_index + 1);

            // Освобождаем старую память
            delete[] data_;
            data_ = new_data;
            capacity_ = new_capacity;
        }
        else {
            // Сдвигаем элементы вправо
            std::copy_backward(data_ + insert_index, data_ + size_, data_ + size_ + 1);

            // Вставляем новый элемент
            data_[insert_index] = value;
        }

        // Увеличиваем размер
        ++size_;

        // Возвращаем итератор на вставленный элемент
        return begin() + insert_index;
    }
    
    // Добавление элемента в конец
    void PushBack(Type&& item) {
        if (size_ == capacity_) {
            size_t new_capacity = capacity_ == 0 ? 1 : capacity_ * 2;
            ArrayPtr<Type> new_data(new_capacity);
            std::move(begin(), end() + size_, new_data.Get());
            data_ = std::move(new_data);
            capacity_ = new_capacity;
        }
        data_[size_] = std::move(item);
        ++size_;
    }

    // Вставка элемента
    Iterator Insert(ConstIterator pos, Type&& value) {
    if (pos < begin() || pos > end()) {
        throw std::out_of_range("Insert position out of range");
    }
    size_t insert_index = pos - begin();

    if (size_ == capacity_) {
        // Увеличиваем ёмкость
        size_t new_capacity = capacity_ == 0 ? 1 : capacity_ * 2;
        ArrayPtr<Type> new_data(new_capacity);

        // Перемещаем элементы до позиции вставки
        std::move(data_.Get(), data_.Get() + insert_index, new_data.Get());

        // Вставляем новый элемент
        new_data[insert_index] = std::move(value);

        // Перемещаем оставшиеся элементы
        std::move(data_.Get() + insert_index, data_.Get() + size_, new_data.Get() + insert_index + 1);

        data_ = std::move(new_data);
        capacity_ = new_capacity;
    } else {
        // Сдвигаем элементы вправо, начиная с позиции вставки
        std::move_backward(data_.Get() + insert_index, data_.Get() + size_, data_.Get() + size_ + 1);

        // Вставляем новый элемент
        data_[insert_index] = std::move(value);
    }

    ++size_;
    return begin() + insert_index;
}

    // Удаление последнего элемента
    void PopBack() noexcept {
        assert(size_ > 0 && "PopBack called on an empty container");
        --size_;
        data_[size_] = Type(); // Очищаем последний элемент
    }

    // Удаление элемента
    Iterator Erase(ConstIterator pos) {
        if (size_ == 0) {
            throw std::out_of_range("Cannot erase from an empty container");
        }
        if (pos < begin() || pos >= end()) {
            throw std::out_of_range("Erase position out of range");
        }

        Iterator non_const_pos = const_cast<Iterator>(pos);
        std::move(non_const_pos + 1, end(), non_const_pos);
        --size_;
        data_[size_] = Type(); // Очищаем последний элемент

        return non_const_pos;
    }

    // Обмен с другим вектором
    void swap(SimpleVector& other) noexcept {
        std::swap(data_, other.data_);
        std::swap(size_, other.size_);
        std::swap(capacity_, other.capacity_);
    }

    // Метод Reserve
    void Reserve(size_t new_capacity) {
        if (new_capacity > capacity_) {
            ArrayPtr<Type> new_data(new_capacity);
            std::move(data_.Get(), data_.Get() + size_, new_data.Get());
            data_ = std::move(new_data);
            capacity_ = new_capacity;
        }
    }

private:
    size_t size_ = 0;
    size_t capacity_ = 0;
    ArrayPtr<Type> data_;
};

// Операторы сравнения

template <typename Type>
inline bool operator<(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return std::lexicographical_compare(
        lhs.begin(), lhs.end(), // Начало и конец первого диапазона
        rhs.begin(), rhs.end()  // Начало и конец второго диапазона
    );
}

template <typename Type>
inline bool operator==(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return lhs.GetSize() == rhs.GetSize() &&
        std::equal(lhs.begin(), lhs.end(), rhs.begin());
}

template <typename Type>
inline bool operator!=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(lhs == rhs);
}

template <typename Type>
inline bool operator>(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return rhs < lhs;
}

template <typename Type>
inline bool operator<=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(lhs > rhs);
}

template <typename Type>
inline bool operator>=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(lhs < rhs);
}
