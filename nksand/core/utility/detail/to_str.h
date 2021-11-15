#pragma once

#include <type_traits>
#include <string>
#include "../lazy_false.h"

namespace qsb { namespace to_str_detail {
// -----------------------------------------------------------------------------
//	tags for dispatch
// -----------------------------------------------------------------------------
	struct not_defined_tag {};
	struct convertible_tag : not_defined_tag {};
	struct short_adl_function_tag : convertible_tag {};
	struct adl_function_tag : short_adl_function_tag {};
	struct std_function_tag : adl_function_tag {};
	struct short_member_function_tag : std_function_tag {};
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
		std::is_constructible_v<std::string, T&&>, 
		convertible_tag
	>;

	template <typename T>
	constexpr auto get_tag_impl(short_adl_function_tag, T&&) -> std::enable_if_t<
		std::is_same_v<remove_cvref_t<decltype(to_str(std::declval<T>()))>, std::string>, 
		short_adl_function_tag
	>;

	template <typename T>
	constexpr auto get_tag_impl(adl_function_tag, T&&) -> std::enable_if_t<
		std::is_same_v<remove_cvref_t<decltype(to_string(std::declval<T>()))>, std::string>, 
		adl_function_tag
	>;

	template <typename T>
	constexpr auto get_tag_impl(std_function_tag, T&&) -> std::enable_if_t<
		std::is_same_v<remove_cvref_t<decltype(std::to_string(std::declval<T>()))>, std::string>,
		std_function_tag
	>;

	template <typename T>
	constexpr auto get_tag_impl(short_member_function_tag, T&&) -> std::enable_if_t<
		std::is_same_v<remove_cvref_t<decltype(std::declval<T>().to_str())>, std::string>,
		short_member_function_tag
	>;

	template <typename T>
	constexpr auto get_tag_impl(member_function_tag, T&&) -> std::enable_if_t<
		std::is_same_v<remove_cvref_t<decltype(std::declval<T>().to_string())>, std::string>,
		member_function_tag
	>;

	template <typename T>
	using get_tag = decltype(get_tag_impl(member_function_tag{}, std::declval<T>()));

// -----------------------------------------------------------------------------
//  trait_appliable
// -----------------------------------------------------------------------------
	auto trait_appliable_impl(...) -> std::false_type;

	template <typename T, typename ...Args>
	auto trait_appliable_impl(T, Args&&...) -> decltype(T::apply(std::declval<Args>()...), std::true_type{});

	template <typename T, typename ...Args>
	using trait_appliable = decltype(trait_appliable_impl(std::declval<T>(), std::declval<Args>()...));

}} // namespace qsb::to_str_detail

namespace qsb { namespace traits {
// -----------------------------------------------------------------------------
//  to_string_trait
// -----------------------------------------------------------------------------
	template <typename T, typename>
	struct to_string_trait {
	private:
		using this_type = to_string_trait;

		template <typename AT>
		static std::string apply(AT&& value, to_str_detail::convertible_tag)
		{
			return std::string(std::forward<AT>(value));
		}

		template <typename AT>
		static decltype(auto) apply(AT&& value, to_str_detail::short_adl_function_tag)
		{
			return to_str(std::forward<AT>(value));
		}

		template <typename AT>
		static decltype(auto) apply(AT&& value, to_str_detail::adl_function_tag)
		{
			return to_string(std::forward<AT>(value));
		}

		template <typename AT>
		static std::string apply(AT&& value, to_str_detail::std_function_tag)
		{
			return std::to_string(std::forward<AT>(value));
		}

		template <typename AT>
		static decltype(auto) apply(AT&& value, to_str_detail::short_member_function_tag)
		{
			return std::forward<AT>(value).to_str();
		}

		template <typename AT>
		static decltype(auto) apply(AT&& value, to_str_detail::member_function_tag)
		{
			return std::forward<AT>(value).to_string();
		}

	public:
		template <typename AT, std::enable_if_t<
			!std::is_same_v<to_str_detail::not_defined_tag, to_str_detail::get_tag<AT>>,
			std::nullptr_t
		> = nullptr>
		static constexpr std::string apply(AT&& value)
		{
			static_assert(std::is_same_v<T, to_str_detail::remove_cvref_t<AT>>, "impl.error. type mismatch.");
			return this_type::apply(std::forward<AT>(value), to_str_detail::get_tag<AT>{});
		}
	};

// -----------------------------------------------------------------------------
//  to_string_trait<std::string>
// -----------------------------------------------------------------------------
	template <>
	struct to_string_trait<std::string> {
		static std::string apply(std::string&& str)
		{
			return std::move(str);
		}

		static std::string apply(const std::string& str)
		{
			return str;
		}
	};

}} // namespace qsb::traits