#pragma once

#include <version>

#ifdef __cpp_lib_inplace_vector
#include <inplace_vector>
#else

#include <array>
#include <exception>

namespace unstd
{

/**
 * @brief like std::inplace_vector in c++26, but ugly.
 */
template<typename Ty, size_t N>
class inplace_vector
{
    static_assert(sizeof(Ty) * N < 1024, "error: the object is too big.");

private:
    std::array<Ty, N> m_data{};
    size_t m_size{};

public:
    Ty *begin() { return data(); }
    const Ty *begin() const { return data(); }
    Ty *end() { return data() + m_size; }
    const Ty *end() const { return data() + m_size; }

    Ty &operator[](size_t index) { return m_data[index]; }
    const Ty &operator[](size_t index) const { return m_data[index]; }

    bool empty() const { return m_size == 0; }
    size_t size() const { return m_size; }
    size_t capacity() const { return N; }

    Ty &front() { return m_data[0]; }
    const Ty &front() const { return m_data[0]; }
    Ty &back() { return m_data[m_size - 1]; }
    const Ty &back() const { return m_data[m_size - 1]; }
    Ty *data() { return m_data.data(); }
    const Ty *data() const { return m_data.data(); }

    Ty &push_back(const Ty &value)
    {
        if (m_size < N) {
            m_data[m_size] = value;
            m_size++;
        } else {
            throw std::bad_alloc{};
        }
        return back();
    }
    Ty &push_back(Ty &&value)
    {
        if (m_size < N) {
            m_data[m_size] = std::move(value);
            m_size++;
        } else {
            throw std::bad_alloc{};
        }
        return back();
    }
    Ty *try_push_back(const Ty &value)
    {
        if (m_size < N) {
            m_data[m_size] = value;
            m_size++;
            return &back();
        }
        return nullptr;
    }
    Ty *try_push_back(Ty &&value)
    {
        if (m_size < N) {
            m_data[m_size] = std::move(value);
            m_size++;
            return &back();
        }
        return nullptr;
    }
    Ty &unchecked_push_back(const Ty &value)
    {
        m_data[m_size] = value;
        m_size++;
        return back();
    }
    Ty &unchecked_push_back(Ty &&value)
    {
        m_data[m_size] = std::move(value);
        m_size++;
        return back();
    }
    void pop_back()
    {
        if (m_size > 0) {
            m_size--;
        }
    }
    void clear() { m_size = 0; }
};

} // namespace unstd

namespace std
{
template<typename Ty, size_t N>
using inplace_vector = unstd::inplace_vector<Ty, N>;
}

#endif
