#pragma once
#include<utility>
#include<iterator>
namespace Iter{

namespace detail{
template<typename iter_t>
struct enum_data {
    enum_data(std::size_t _i, iter_t _iter) :
       i(_i), iter(_iter), iter_obj(*iter) {}

    enum_data(const enum_data &_data) :
       i(_data.i), iter_obj(*_data.iter), copy(true) {}

    template<size_t I>
    inline decltype(auto) get() {
        if constexpr (I == 0) return i;
        else if constexpr (I == 1) return copy ? iter_obj : *iter;
    }

    template<size_t I>
    inline decltype(auto) get() const { return const_cast<enum_data *>(this)->template get<I>(); }

    std::size_t i;
    iter_t iter;
    typename std::iterator_traits<iter_t>::value_type iter_obj;
    bool copy = false;
};
}

template<typename container_t>
struct enumerate {
	
private:
    container_t &container_;
    using iter_t = decltype(std::begin(container_));
    using value_type = typename container_t::value_type;
private:
    detail::enum_data<iter_t> result{0, std::begin(container_)};

    struct iter_wrapper {
        inline bool operator!=(const iter_t &other) const { return data_.iter != other; }
        inline void operator++() {++data_.i;++data_.iter;}
        inline std::pair<int,value_type&> operator*() const { return {data_.i,*data_.iter}; }
        detail::enum_data<iter_t> &data_;
    } begin_it{result};

public:
    explicit enumerate(container_t &_container) : container_(_container) {}
    explicit enumerate(container_t &&_container) : container_(_container) {}
    auto &begin() { return begin_it; }
    iter_t end() { return std::end(container_); }
};



} // namespace Iter

