#pragma once

#include <stdlib.h>
#include <type_traits>
#include <string>

namespace qsb { namespace traits {
// -----------------------------------------------------------------------------
//  to_string_trait
// -----------------------------------------------------------------------------
	/*
		qsb::util::to_string provides a unified function to convert a value into std::string.
	 
		traits::to_string_trait is a customization point for util::to_string.
		Default implementation is the followings:
			1. direct convert from T to string
			2. ADL call to_string
			3. ADL call to_str
			4. std::to_string
			5. member function .to_string
			6. member function .to_str
		
		So even when we do not provide a specialization of this trait, 
		util::to_string works well if either of the above is defined. 
		ex)
			namespace test {
				struct to_str_test_class1 {};
				std::string to_string(to_str_test_class) { return "test42"; }
	
				struct to_str_test_class2 {
					std::string to_string(to_str_test_class) { return "42test"; }
				};
			}
			
			int main() {
				const test::to_str_test_class1 x;
				std::cout << qsb::util::to_string(x) << std::endl;	// test42
	 
				const test::to_str_test_class2 x;
				std::cout << qsb::util::to_string(x) << std::endl;	// 42test
			}
	*/
	template <typename T, typename = void>
	struct from_string_trait;

}} // namespace qsb::traits

#include "detail/to_str.h"

namespace qsb { namespace util {
// -----------------------------------------------------------------------------
//  to_string : T&& -> std::string
// 	to_str	  : T&& -> std::string
// -----------------------------------------------------------------------------
	template <typename T, std::enable_if_t<
		to_str_detail::trait_appliable<traits::to_string_trait<to_str_detail::remove_cvref_t<T>>, T&&>::value, std::nullptr_t
	> = nullptr>
	std::string to_string(T&& value)
	{
		using to_stringer = traits::to_string_trait<to_str_detail::remove_cvref_t<T>>;
		return to_stringer::apply(std::forward<T>(value));
	}

	template <typename T, std::enable_if_t<
		to_str_detail::trait_appliable<traits::to_string_trait<to_str_detail::remove_cvref_t<T>>, T&&>::value, std::nullptr_t
	> = nullptr>
	std::string to_str(T&& value)
	{
		return util::to_string(std::forward<T>(value));
	}

// -----------------------------------------------------------------------------
//  to_string : (double, int, int) -> std::string
// 	to_str	  : (double, int, int) -> std::string
// -----------------------------------------------------------------------------
	std::string to_string(double d, int prec, int digits)
	{
		if (d > 1.0e32 || d < -1.0e32)
		{
			return std::to_string(d);
		}
		char s[64]{};
#ifdef _MSC_VER
		sprintf_s(s, "%*.*f", digits > 64 ? 64 : digits, prec > 64 ? 64 : prec, d);
#else
		sprintf(s, "%*.*f", digits > 64 ? 64 : digits, prec > 64 ? 64 : prec, d);
#endif
		return std::string(s);
	}

	std::string to_str(double d, int prec, int digits)
	{
		return util::to_string(d, prec, digits);
	}

}} // namespace qsb::util
