
#pragma once

template<typename T0, typename... TX>
struct Tag { using value_type = T0; };

template<typename F, typename T>
void ForEachTag(Tag<T> t, F f)
{
	f(t);
}

template<typename F, typename T1, typename... TX>
void ForEachTag(Tag<T1, TX...>, F f)
{
	f(Tag<T1>{});
	ForEachTag(Tag<TX...>{}, f);
}

//template<typename T0, typename... TX>
//constexpr auto PrependTag(Tag<T0>, Tag<TX...>)
//{
//	return Tag<T0, TX...>{};
//}


template <int IntValue0, int... IntValuesX>
struct IntegerConstants
{
	static constexpr int value() noexcept { return IntValue0; }
};

template<typename F, int IntValue>
void ForEachIntegerConstant(IntegerConstants<IntValue> ic, F f)
{
	f(ic);
}

template<typename F, int IntValue0, int... IntValueX>
void ForEachIntegerConstant(IntegerConstants<IntValue0, IntValueX...>, F f)
{
	f(IntegerConstants<IntValue0>{});
	ForEachIntegerConstant(IntegerConstants<IntValueX...>{}, f);
}

template<typename F> constexpr void ConstexprIf(std::false_type, F) { }
template<typename F> constexpr void ConstexprIf(std::true_type, F f) { f(); }
template<typename F1, typename F2> constexpr void ConstexprIfElse(std::false_type, F1, F2 f_false) { f_false(); }
template<typename F1, typename F2> constexpr void ConstexprIfElse(std::true_type, F1 f_true, F2) { f_true(); }
template<bool> struct BoolType : public std::false_type { };
template<> struct BoolType<true> : public std::true_type { };

/*
	The other way around (kudos https://github.com/mpusz):

	template<bool B>
	struct ConstexprIf_helper {
		template<typename F> constexpr void operator()(F) {}
	};

	template<>
	struct ConstexprIf_helper<true> {
		template<typename F> constexpr auto operator()(F f) { return f(); }
	};

	template<bool B, typename F> 
	constexpr auto ConstexprIf(F f) { return ConstexprIf_helper<B>{}(f); }
*/

template<typename IntType>
constexpr double GetRatioOf(IntType dividend, IntType divisor)
{
	auto quotient = dividend / divisor;
	auto remainder = dividend - quotient * divisor;
	return static_cast<double>(quotient) + static_cast<double>(remainder) / static_cast<double>(divisor);
}

template<typename Type>
constexpr Type clamp(Type value, Type min, Type max)
{
	return std::min(std::max(value, min), max);
}

struct int128_t
{
	int64_t hi;
	int64_t lo;

	constexpr operator bool() const noexcept { return hi && lo; }

	template<typename T, typename = std::enable_if<std::is_integral<T>::value>, typename = std::enable_if<sizeof(T) <= sizeof(lo)>>
	constexpr int128_t& operator^=(T v) { lo ^= v; return *this; }
};

template<int bits>
struct bigint_t
{
	using value_type = int64_t;
	enum { vv_size = bits / 8 / sizeof(value_type) };
	value_type vv[vv_size];

	constexpr bigint_t() = default;
	template<typename T> constexpr bigint_t(T v) : vv{ v } { }
};

//template<typename IntType> constexpr int gcd(IntType a, IntType b) { return b == 0 ? a : gcd(b, a % b); }
//template<typename IntType> std::string FormatRatio(IntType dividend, IntType divisor) { const auto x = gcd(dividend, divisor); return std::to_string(dividend / x) + '/' + std::to_string(divisor / x); }
