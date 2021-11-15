#pragma once
#include "optional.h"

namespace qsb { namespace traits {
// -----------------------------------------------------------------------------
//  from_string_trait
// -----------------------------------------------------------------------------
	/*
		qsb::util::from_string provides a unified function to convert a string into a value.
	 
		traits::from_string_trait is a customization point for util::from_string.
		Default implementation is the followings:
			1. direct convert from const char*
			2. convert to number such as int, long and double
			3. static from_string method (T::from_string(const char*))
			4. static from_str method (T::from_str(const char*))
		
		So even when we do not provide a specialization of this trait, 
		util::from_string works well if either of the above is defined. 
	
		ex)
			namespace test {
				struct from_str_test_class {
					int i;
					static from_str_test_class from_string(const std::string& str)
					{
						return {std::stoi(str)};
					}
				};
			}
			
			int main() {
				const auto x = qsb::util::from_string<test::from_str_test_class>("42");
				std::cout << x.i << std::endl;	// 42
			}
	*/
	template <typename T, typename = void>
	struct from_string_trait;

}} // namespace qsb::traits

#include "detail/from_str.h"

namespace qsb { namespace util {
// -----------------------------------------------------------------------------
//  from_string	: const char* -> T
// 	from_str	: const char* -> T
// -----------------------------------------------------------------------------
	template <typename T, std::enable_if_t<
		from_str_detail::trait_appliable<traits::from_string_trait<T>, const char*>::value,
	std::nullptr_t> = nullptr>
	T from_string(const char* str)
	{
		using from_string = traits::from_string_trait<T>;
		return from_string::apply(str);
	}

	template <typename T, std::enable_if_t<
		from_str_detail::trait_appliable<traits::from_string_trait<T>, const char*>::value,
	std::nullptr_t> = nullptr>
	T from_str(const char* str)
	{
		return util::from_string<T>(str);
	}

// -----------------------------------------------------------------------------
//  from_string	: (const char*, T&) -> void
// 	from_str	: (const char*, T&) -> void
// -----------------------------------------------------------------------------
	template <typename T, std::enable_if_t<
		from_str_detail::trait_appliable<traits::from_string_trait<T>, const char*>::value,
	std::nullptr_t> = nullptr>
	void from_string(const char* str, T& ref)
	{
		ref = util::from_string<T>(str);
	}

	template <typename T, std::enable_if_t<
		from_str_detail::trait_appliable<traits::from_string_trait<T>, const char*>::value,
	std::nullptr_t> = nullptr>
	void from_str(const char* str, T& ref)
	{
		ref = util::from_string<T>(str);
	}

// -----------------------------------------------------------------------------
//  safe_from_string : const char* -> optional<T>
// 	safe_from_str    : const char* -> optional<T>
// -----------------------------------------------------------------------------
	template <typename T, std::enable_if_t<
		from_str_detail::trait_appliable<traits::from_string_trait<T>, const char*>::value,
	std::nullptr_t> = nullptr>
	optional<T> safe_from_string(const char* str)
	{
		return from_string<optional<T>>(str);
	}

	template <typename T, std::enable_if_t<
		from_str_detail::trait_appliable<traits::from_string_trait<T>, const char*>::value,
	std::nullptr_t> = nullptr>
	optional<T> safe_from_str(const char* str)
	{
		return util::safe_from_string<T>(str);
	}

// -----------------------------------------------------------------------------
//  try_from_string : (const char*, T&) -> bool
// 	try_from_str	: (const char*, T&) -> bool
// -----------------------------------------------------------------------------
	template <typename T, std::enable_if_t<
		from_str_detail::trait_appliable<traits::from_string_trait<T>, const char*>::value,
	std::nullptr_t> = nullptr>
	bool try_from_string(const char* str, T& ref)
	{
		const auto opt = safe_from_string<T>(str);
		if (opt) {
			ref = opt.value();
			return true;
		}
		return false;
	}

	template <typename T, std::enable_if_t<
		from_str_detail::trait_appliable<traits::from_string_trait<T>, const char*>::value,
	std::nullptr_t> = nullptr>
	bool try_from_str(const char* str, T& ref)
	{
		return util::try_from_string(str, ref);
	}

// -----------------------------------------------------------------------------
//	convertible_string
// -----------------------------------------------------------------------------
	/*
		This class has two roles.
			1. facade to util::from_string, util::safe_from_string and util::try_from_string
			2. convert string into values depending on externally specified types.

		ex)
			auto get_str_number() {
				return qsb::util::lazy_from_str("42");
			}

			int main() {
				int i = get_str_number();
				const double d = get_str_number();
				std::string s = get_str_number();
				qsb::optional<int> maybe_i = get_str_number();
			}
	*/
	template <typename T>
	class convertible_string {
	private:
		using this_type = convertible_string;
		using string_type = T;

	public:
	// -------------------------------------------------------------------------
	//  constructors
	//
		constexpr convertible_string() = delete;
		constexpr convertible_string(const this_type&) = default;
		constexpr convertible_string(this_type&&) noexcept(std::is_nothrow_move_constructible_v<T>) = default;

		template <typename AT>
		explicit constexpr convertible_string(AT&& str)
			: str_(std::forward<AT>(str))
		{
		}

	// -------------------------------------------------------------------------
	//  assignments
	//
		constexpr this_type& operator =(const this_type&) = default;
		constexpr this_type& operator =(this_type&&) noexcept(std::is_nothrow_move_assignable_v<T>) = default;

	// -------------------------------------------------------------------------
	//	get  
	//
		constexpr const string_type& str() const noexcept { return str_; }

	// -------------------------------------------------------------------------
	//  cast
	//
		template <typename T>
		T as() const { return util::from_str<T>(this->str()); }

		auto as_int() const -> int { return this->as<int>(); }
		auto as_long() const -> long { return this->as<long>(); }
		auto as_llong() const -> long long { return this->as<long long>(); }
		auto as_float() const -> float { return this->as<float>(); }
		auto as_double() const -> double { return this->as<double>(); }
		auto as_ldouble() const -> long double { return this->as<long double>(); }

		template <typename T>
		optional<T> maybe() const { return util::safe_from_str<T>(this->str()); }

		auto maybe_int() const -> optional<int> { return this->maybe<int>(); }
		auto maybe_long() const -> optional<long> { return this->maybe<long>(); }
		auto maybe_llong() const -> optional<long long> { return this->maybe<long long>(); }
		auto maybe_float() const -> optional<float> { return this->maybe<float>(); }
		auto maybe_double() const -> optional<double> { return this->maybe<double>(); }
		auto maybe_ldouble() const -> optional<long double> { return this->maybe<long double>(); }

		template <typename T>
		constexpr void set_into(T& ref) const { util::from_str(str_, ref); }

		template <typename T>
		constexpr bool try_set_into(T& ref) const { return util::try_from_string(str_, ref); }

	// -------------------------------------------------------------------------
	//  implicit cast
	//
		template <typename T, std::enable_if_t<
			!from_str_detail::is_optional<T>::value,
		std::nullptr_t> = nullptr>
		operator T() const
		{
			return this->as<T>();
		}

		template <typename T, std::enable_if_t<
			from_str_detail::is_optional<T>::value &&
			!std::is_reference_v<typename from_str_detail::remove_cvref_t<T>::value_type>,
		std::nullptr_t> = nullptr>
		operator T() const
		{
			return this->maybe<typename from_str_detail::remove_cvref_t<T>::value_type>();
		}

	private:
		string_type str_; 

	}; // class convertible_string

// -----------------------------------------------------------------------------
//  lazy_from_string : <string> -> convertible_string<string>
// 	lazy_from_str	 : <string> -> convertible_string<string>
// -----------------------------------------------------------------------------
	convertible_string<const char*> lazy_from_string(const char* str)
	{
		return convertible_string<const char*>(str);
	}
	convertible_string<std::string> lazy_from_string(const std::string& str)
	{
		return convertible_string<std::string>(str);
	}
	convertible_string<std::string> lazy_from_string(std::string&& str)
	{
		return convertible_string<std::string>(std::move(str));
	}
	convertible_string<const std::string&> lazy_from_string(std::reference_wrapper<std::string> str)
	{
		return convertible_string<const std::string&>(str);
	}
	convertible_string<const std::string&> lazy_from_string(std::reference_wrapper<const std::string> str)
	{
		return convertible_string<const std::string&>(str);
	}

	convertible_string<const char*> lazy_from_str(const char* str)
	{
		return lazy_from_string(str);
	}
	convertible_string<std::string> lazy_from_str(const std::string& str)
	{
		return lazy_from_string(str);
	}
	convertible_string<std::string> lazy_from_str(std::string&& str)
	{
		return lazy_from_string(std::move(str));
	}
	convertible_string<const std::string&> lazy_from_str(std::reference_wrapper<std::string> str)
	{
		return lazy_from_string(str);
	}
	convertible_string<const std::string&> lazy_from_str(std::reference_wrapper<const std::string> str)
	{
		return lazy_from_string(str);
	}

}} // namespace qsb::util
