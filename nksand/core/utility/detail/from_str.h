#pragma once
#include <cerrno>
#include <type_traits>
#include <cstdint>
#include <stdexcept>
#include "../optional.h"

namespace qsb { namespace from_str_detail {
// -----------------------------------------------------------------------------
//	tags for dispatch
// -----------------------------------------------------------------------------
	struct not_defined_tag {};
	struct convertible_tag : not_defined_tag {};
	struct short_member_function_tag : convertible_tag {};
	struct member_function_tag : short_member_function_tag {};

// -----------------------------------------------------------------------------
//  remove_cvref_t
// -----------------------------------------------------------------------------
	template <typename T>
	using remove_cvref_t = std::remove_cv_t<std::remove_reference_t<T>>;

// -----------------------------------------------------------------------------
//  get_tag
// -----------------------------------------------------------------------------
	constexpr not_defined_tag get_tag_impl(not_defined_tag, ...);

	template <typename T>
	constexpr auto get_tag_impl(convertible_tag, T&&) -> std::enable_if_t<
		std::is_constructible_v<T, const char*>, 
		convertible_tag
	>;

	template <typename T>
	constexpr auto get_tag_impl(short_member_function_tag, T&&) -> std::enable_if_t<
		std::is_same_v<remove_cvref_t<decltype(T::from_str(std::declval<const char*>()))>, T>,
		short_member_function_tag
	>;

	template <typename T>
	constexpr auto get_tag_impl(member_function_tag, T&&) -> std::enable_if_t<
		std::is_same_v<remove_cvref_t<decltype(T::from_string(std::declval<const char*>()))>, T>,
		member_function_tag
	>;

	template <typename T>
	using get_tag = decltype(get_tag_impl(member_function_tag{}, std::declval<T>()));

// -----------------------------------------------------------------------------
//  strtox
// -----------------------------------------------------------------------------
	template <typename T>
	T strtox(const char* str, char** e, int base = 10);

	template <>
	short strtox<short>(const char* str, char** e, int base)
	{
		return static_cast<short>(std::strtol(str, e, base));
	}

	template <>
	unsigned short strtox<unsigned short>(const char* str, char** e, int base)
	{
		return static_cast<unsigned short>(std::strtoul(str, e, base));
	}

	template <>
	int strtox<int>(const char* str, char** e, int base)
	{
		return static_cast<int>(std::strtol(str, e, base));
	}

	template <>
	unsigned strtox<unsigned>(const char* str, char** e, int base)
	{
		return static_cast<unsigned>(std::strtoul(str, e, base));
	}

	template <>
	long strtox<long>(const char* str, char** e, int base)
	{
		return std::strtol(str, e, base);
	}

	template <>
	unsigned long strtox<unsigned long>(const char* str, char** e, int base)
	{
		return std::strtoul(str, e, base);
	}

	template <>
	long long strtox<long long>(const char* str, char** e, int base)
	{
		return std::strtoll(str, e, base);
	}

	template <>
	unsigned long long strtox<unsigned long long>(const char* str, char** e, int base)
	{
		return std::strtoull(str, e, base);
	}

	template <>
	float strtox<float>(const char* str, char** e, int)
	{
		return std::strtof(str, e);
	}

	template <>
	double strtox<double>(const char* str, char** e, int)
	{
		return std::strtod(str, e);
	}

	template <>
	long double strtox<long double>(const char* str, char** e, int)
	{
		return std::strtold(str, e);
	}

// -----------------------------------------------------------------------------
//  trait_appliable
// -----------------------------------------------------------------------------
	auto trait_appliable_impl(...) -> std::false_type;

	template <typename T, typename ...Args>
	auto trait_appliable_impl(T, Args&&...) -> decltype(T::apply(std::declval<Args>()...), std::true_type{});

	template <typename T, typename ...Args>
	using trait_appliable = decltype(trait_appliable_impl(std::declval<T>(), std::declval<Args>()...));

// -----------------------------------------------------------------------------
//  from_string_to_number_impl
// -----------------------------------------------------------------------------
	template <typename T>
	struct from_string_to_number_impl {
		static T apply(const char* str)
		{
		    int& errno_ref = errno;
			char* eptr {};
			errno_ref = 0;
			const T result = strtox<T>(str, &eptr);

			if (str == eptr) {
				throw std::invalid_argument("invalid strtol argument");
			}

			if (errno_ref == ERANGE) {
				throw std::invalid_argument("strtol argument out of range");
			}
			return result;
		}
	};

// -----------------------------------------------------------------------------
//  is_optional
// -----------------------------------------------------------------------------
	template <typename T>
	struct is_optional_impl : std::false_type {};

	template <typename T>
	struct is_optional_impl<optional<T>> : std::true_type {};

	template <typename T>
	using is_optional = is_optional_impl<std::remove_cv_t<std::remove_reference_t<T>>>;

}} // namespace qsb::from_str_detail

namespace qsb { namespace traits {
// -----------------------------------------------------------------------------
//  from_string_trait
// -----------------------------------------------------------------------------
	template <typename T, typename>
	struct from_string_trait {
	private:
		using this_type = from_string_trait;

		static T apply(const char* str, from_str_detail::convertible_tag)
		{
			return static_cast<T>(str);
		}

		static T apply(const char* str, from_str_detail::short_member_function_tag)
		{
			return T::from_str(str);
		}

		static T apply(const char* str, from_str_detail::member_function_tag)
		{
			return T::from_string(str);
		}

	public:
		template <typename Dummy = void>
		static auto apply(const char* str) -> std::enable_if_t<
			!std::is_same_v<from_str_detail::get_tag<T>, from_str_detail::not_defined_tag>, T>
		{
			return this_type::apply(str, from_str_detail::get_tag<T>{});
		}
	};

	template <typename T>
	struct from_string_trait<const T> : from_string_trait<T> {};

	template <typename T>
	struct from_string_trait<volatile T> : from_string_trait<T> {};

	template <typename T>
	struct from_string_trait<const volatile T> : from_string_trait<T> {};

// -----------------------------------------------------------------------------
//  from_string_trait<number>
// -----------------------------------------------------------------------------
	template <> struct from_string_trait<short> : from_str_detail::from_string_to_number_impl<short> {};
	template <> struct from_string_trait<unsigned short> : from_str_detail::from_string_to_number_impl<unsigned short> {};
	template <> struct from_string_trait<int> : from_str_detail::from_string_to_number_impl<int> {};
	template <> struct from_string_trait<unsigned> : from_str_detail::from_string_to_number_impl<unsigned> {};
	template <> struct from_string_trait<long> : from_str_detail::from_string_to_number_impl<long> {};
	template <> struct from_string_trait<unsigned long> : from_str_detail::from_string_to_number_impl<unsigned long> {};
	template <> struct from_string_trait<long long> : from_str_detail::from_string_to_number_impl<long long> {};
	template <> struct from_string_trait<unsigned long long> : from_str_detail::from_string_to_number_impl<unsigned long long> {};
	template <> struct from_string_trait<float> : from_str_detail::from_string_to_number_impl<float> {};
	template <> struct from_string_trait<double> : from_str_detail::from_string_to_number_impl<double> {};
	template <> struct from_string_trait<long double> : from_str_detail::from_string_to_number_impl<long double> {};

	template <>
	struct from_string_trait<std::int8_t, std::enable_if_t<
		std::is_same_v<std::int8_t, unsigned char> ||
		std::is_same_v<std::int8_t, signed char>>
	> {
		static std::int8_t apply(const char* str)
		{
			return static_cast<std::int8_t>(
				from_str_detail::from_string_to_number_impl<long>::apply(str));
		}
	};

// -----------------------------------------------------------------------------
//  from_string_trait<optional>
// -----------------------------------------------------------------------------
	template <typename T>
	struct from_string_trait<optional<T>, std::enable_if_t<
		from_str_detail::trait_appliable<from_string_trait<T>, const char*>::value
	>> {
		static optional<T> apply(const char* str)
		{
			try {
				return optional<T>(from_string_trait<T>::apply(str));
			}
			catch (...) {
				return optional<T>(nullopt);
			}
		}
	};

}} // namespace qsb::traits
