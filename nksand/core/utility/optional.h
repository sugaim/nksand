#pragma once

#include <type_traits>
#include <utility>
#include "../../_external/TartanLlama_optional/optional.hpp"

namespace qsb { namespace opt_detail {
// -----------------------------------------------------------------------------
//  is_optional
// -----------------------------------------------------------------------------
	template <typename T>
	struct is_optional : std::false_type {};

// -----------------------------------------------------------------------------
//  pipe_dispatch
// -----------------------------------------------------------------------------
	template <typename T, typename F, typename = void, typename = void>
	struct pipe_dispatch {
		static constexpr auto apply(T&& opt, F&& f)
			-> decltype(std::declval<T>().transform(std::declval<F>()))
		{
			return std::forward<T>(opt).transform(std::forward<F>(f));
		}
	};

	template <typename T, typename F>
	struct pipe_dispatch<T, F, std::enable_if_t<
		is_optional<tl::detail::invoke_result_t<F, decltype(std::declval<T>().value())>>::value
	>> {
		static constexpr auto apply(T&& opt, F&& f)
			-> decltype(std::declval<T>().and_then(std::declval<F>()))
		{
			return std::forward<T>(opt).and_then(std::forward<F>(f));
		}
	};

	template <typename T, typename F>
	struct pipe_dispatch<T, F, tl::detail::enable_if_ret_void<F>> {
		static constexpr auto apply(T&& opt, F&& f)
			-> decltype(std::declval<T>().or_else(std::declval<F>()))
		{
			return std::forward<T>(opt).or_else(std::forward<F>(f));
		}
	};

	template <typename T, typename F>
	struct pipe_dispatch<T, F, tl::detail::disable_if_ret_void<F>> {
		static constexpr auto apply(T&& opt, F&& f)
			-> decltype(std::declval<T>().or_else(std::declval<F>()))
		{
			return std::forward<T>(opt).or_else(std::forward<F>(f));
		}
	};

}} // namespace qsb::opt_detail

namespace qsb {
// -----------------------------------------------------------------------------
//	nullopt_t
// -----------------------------------------------------------------------------
	struct nullopt_t {};
	constexpr nullopt_t nullopt;

// -----------------------------------------------------------------------------
//	in_place_t
// -----------------------------------------------------------------------------
	struct in_place_t {};
	constexpr in_place_t in_place;
	
// -----------------------------------------------------------------------------
//  optional
// -----------------------------------------------------------------------------
	/*
	* It is easy to implement optional for semiregular classes,
	* but implementing for non-semiregular classes is complicated.
	* 
	* This is a reason why we use tl::optional internally.
	* (Instead of using tl::optional directly, we wrap it to reduce dependency on external library)
	* https://github.com/TartanLlama/optional/tree/v1.0.0
	* 
	* With introducing C++17, we can replace tl::optional with std::optional.
	* (Some utilities of tl, such as tl::detail::invoke_result_t, are also used.
	*  These can be replaced with STL ones after introducing C++17, too.)
	* 
	* Doc: qsb::optional almost obeys the following
	* https://en.cppreference.com/w/cpp/utility/optional
	* 
	* In addition to C++17 behaviors of std::optional, C++23 monadic behaviors.
	* See the following about example for such behaviors.
	* http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2021/p0798r6.html
	* 
	* C++23 example used in the paper to get cute cat image from an image img.
	* ---------------------------------------------------------------------------
	*	const optional<image> maybe_cute_cat = crop_to_cat(img)
	*		.and_then(add_bow_tie)
	*		.and_then(make_eyes_sparkle)
	*		.transform(make_smaller)
	*		.transform(add_rainbow);
	* 
	* Pipe operator (opt | func1 | func2 ...), which is a wrapper of C++23 behavior,
	* is also implemented in qsb::optional.
	* 
	* equivalent expression with operator overloading
	* ---------------------------------------------------------------------------
	*	const optional<image> maybe_cute_cat = crop_to_cat(img)
	*		| add_bow_tie
	*		| make_eyes_sparkle
	*		| make_smaller
	*		| add_rainbow;
	* 
	* or if we have default cute cat image default_cute_cat_img,  
	* ---------------------------------------------------------------------------
	*	const image at_lease_cute_cat = crop_to_cat(img)
	*		| add_bow_tie
	*		| make_eyes_sparkle
	*		| make_smaller
	*		| add_rainbow
	*		/= default_cute_cat_img;  // assign(=) computed result, or(/) default_cute_cat_img
	*/
	template <typename T>
	class optional {
	private:
		using this_type = optional;
		using internal_type = tl::optional<T>;		
		template <typename U> friend class optional;

	public:
		using value_type = T;

	// -------------------------------------------------------------------------
	//  constructors
	//
		constexpr optional() noexcept = default;
		constexpr optional(nullopt_t) noexcept : internal_(tl::nullopt) {}
		constexpr optional(const this_type&) = default;
		constexpr optional(this_type&&) noexcept(std::is_nothrow_move_constructible_v<internal_type>) = default;

		template <
			typename U,
			std::enable_if_t<std::is_constructible_v<tl::optional<T>, const tl::optional<U>&>, std::nullptr_t> = nullptr
		>
		explicit optional(const optional<U>& other) : internal_(other.internal_) {}

		template <
			typename U,
			std::enable_if_t<std::is_constructible_v<tl::optional<T>, tl::optional<U>&&>, std::nullptr_t> = nullptr
		>
		explicit optional(optional<U>&& other) : internal_(other.internal_) {}

		template <
			typename ...Args,
			std::enable_if_t<std::is_constructible_v<tl::optional<T>, Args&&...>, std::nullptr_t> = nullptr
		>
		explicit constexpr optional(in_place_t, Args&&... args)
			: internal_(in_place, std::forward<Args>(args)...) {}

		template <
			typename U = T,
			std::enable_if_t<std::is_constructible_v<tl::optional<T>, U&&>, std::nullptr_t> = nullptr
		>
		explicit constexpr optional(U&& value)
			: internal_(std::forward<U>(value)) {}

	// -------------------------------------------------------------------------
	//  assignments
	//
		this_type& operator =(nullopt_t) noexcept
		{
			internal_ = tl::nullopt;
			return *this;
		}

		constexpr this_type& operator =(const this_type&) = default;
		constexpr this_type& operator =(this_type&&) 
			noexcept(std::is_nothrow_move_assignable_v<internal_type>) = default;

		template <typename U = T>
		auto operator =(U&& value)
			-> std::enable_if_t<std::is_assignable_v<internal_type&, U&&>, this_type&>
		{
			internal_ = std::forward<U>(value);
			return *this;
		}

		template <typename U>
		auto operator =(const optional<U>& other)
			-> std::enable_if_t<std::is_assignable_v<internal_type&, const optional<U>&>, this_type&>
		{
			internal_ = other.internal_;
			return *this;
		}

		template <typename U>
		auto operator =(optional<U>&& other)
			-> std::enable_if_t<std::is_assignable_v<internal_type&, optional<U>&&>, this_type&>
		{
			internal_ = std::move(other).internal_;
			return *this;
		}

	// ---------------------------------------------------------------------
	//	pointer like access
	//
		constexpr T* operator->() noexcept
		{
			return internal_.operator->();
		}

		constexpr const T* operator->() const noexcept
		{
			return internal_.operator->();
		}

		constexpr T& operator *() & noexcept
		{
			return *internal_;
		}

		constexpr const T& operator *() const & noexcept
		{
			return *internal_;
		}

		constexpr T&& operator *() && noexcept
		{
			return *std::move(internal_);
		}

		constexpr const T&& operator *() const&& noexcept
		{
			return *std::move(internal_);
		}

	// ---------------------------------------------------------------------
	//	get
	//
		constexpr bool has_value() const noexcept
		{
			return internal_.has_value();
		}

		constexpr T& value() &
		{
			return internal_.value();
		}

		constexpr const T& value() const &
		{
			return internal_.value();
		}

		constexpr T&& value() &&
		{
			return std::move(internal_).value();
		}

		constexpr const T&& value() const &&
		{
			return std::move(internal_).value();
		}

		template <typename U>
		constexpr T value_or(U&& dvalue) const &
		{
			return internal_.value_or(std::forward<U>(dvalue));
		}

		template <typename U>
		constexpr T value_or(U&& dvalue)&&
		{
			return std::move(internal_).value_or(std::forward<U>(dvalue));
		}

	// -------------------------------------------------------------------------
	//  monadic
	//
		template <typename F>
		constexpr auto and_then(F&& f) & -> tl::detail::invoke_result_t<F, const T&>
		{
			using result = tl::detail::invoke_result_t<F, T&>;
			static_assert(opt_detail::is_optional<std::remove_cv_t<std::remove_reference_t<result>>>::value,
						  "F must return an optional");

			return has_value() ? tl::detail::invoke(std::forward<F>(f), **this)
							   : result(nullopt);
		}

		template <typename F>
		constexpr auto and_then(F&& f) const & -> tl::detail::invoke_result_t<F, const T&>
		{
			using result = tl::detail::invoke_result_t<F, const T&>;
			static_assert(opt_detail::is_optional<std::remove_cv_t<std::remove_reference_t<result>>>::value,
						  "F must return an optional");

			return has_value() ? tl::detail::invoke(std::forward<F>(f), **this)
							   : result(nullopt);
		}

		template <typename F>
		constexpr auto and_then(F&& f) && -> tl::detail::invoke_result_t<F, T&&>
		{
			using result = tl::detail::invoke_result_t<F, T&&>;
			static_assert(opt_detail::is_optional<std::remove_cv_t<std::remove_reference_t<result>>>::value,
						  "F must return an optional");

			return has_value() ? tl::detail::invoke(std::forward<F>(f), std::move(**this))
							   : result(nullopt);
		}

		template <typename F>
		constexpr auto and_then(F&& f) const && -> tl::detail::invoke_result_t<F, const T&&>
		{
			using result = tl::detail::invoke_result_t<F, const T&&>;
			static_assert(opt_detail::is_optional<std::remove_cv_t<std::remove_reference_t<result>>>::value,
						  "F must return an optional");

			return has_value() ? tl::detail::invoke(std::forward<F>(f), std::move(**this))
							   : result(nullopt);
		}

		template <typename F>
		constexpr auto transform(F&& f) & -> optional<tl::detail::invoke_result_t<F, T&>>
		{
			using result_t = optional<tl::detail::invoke_result_t<F, T&>>;
			return has_value() ? result_t(tl::detail::invoke(std::forward<F>(f), **this))
							   : result_t(nullopt);
		}

		template <typename F>
		constexpr auto transform(F&& f) const & -> optional<tl::detail::invoke_result_t<F, const T&>>
		{
			using result_t = optional<tl::detail::invoke_result_t<F, const T&>>;
			return has_value() ? result_t(tl::detail::invoke(std::forward<F>(f), **this))
							   : result_t(nullopt);
		}

		template <typename F>
		constexpr auto transform(F&& f) && -> optional<tl::detail::invoke_result_t<F, T&&>>
		{
			using result_t = optional<tl::detail::invoke_result_t<F, T&&>>;
			return has_value() ? result_t(tl::detail::invoke(std::forward<F>(f), std::move(**this)))
							   : result_t(nullopt);
		}

		template <typename F>
		constexpr auto transform(F&& f) const && -> optional<tl::detail::invoke_result_t<F, const T&&>>
		{
			using result_t = optional<tl::detail::invoke_result_t<F, const T&&>>;
			return has_value() ? result_t(tl::detail::invoke(std::forward<F>(f), std::move(**this)))
							   : result_t(nullopt);
		}

		template <typename F>
		constexpr auto or_else(F&& f) const &
			-> std::enable_if_t<std::is_void_v<std::decay_t<tl::detail::invoke_result_t<F>>>, this_type>
		{
			if (has_value()) {
				return *this;
			}
			else {
				tl::detail::invoke(std::forward<F>(f));
				return this_type(nullopt);
			}
		}

		template <typename F>
		constexpr auto or_else(F&& f) const &
			-> std::enable_if_t<!std::is_void_v<std::decay_t<tl::detail::invoke_result_t<F>>>, this_type>
		{
			return has_value() ? *this
							   : this_type(tl::detail::invoke(std::forward<F>(f)));
		}

		template <typename F, tl::detail::enable_if_ret_void<F>* = nullptr>
		constexpr auto or_else(F&& f) &&
		{
			if (has_value()) {
				return std::move(*this);
			}
			else {
				tl::detail::invoke(std::forward<F>(f));
				return this_type(nullopt);
			}
		}

		template <typename F, tl::detail::disable_if_ret_void<F>* = nullptr>
		constexpr auto or_else(F&& f) &&
		{
			return has_value() ? std::move(*this)
							   : this_type(tl::detail::invoke(std::forward<F>(f)));
		}

		// pipe. function will be auto dispatched to either of and_then, transform or or_else.
		// if its overload resolution is failed, please use opt_helpers::transform,
		// opt_helpers::and_then or opt_helpers::or_else to restrict functor object interface.
		template <typename F>
		friend constexpr auto operator |(this_type& self, F&& f)
		{
			using dispatcher = opt_detail::pipe_dispatch<this_type&, F&&>;
			return dispatcher::apply(self, std::forward<F>(f));
		}

		template <typename F>
		friend constexpr auto operator |(const this_type& self, F&& f)
		{
			using dispatcher = opt_detail::pipe_dispatch<const this_type&, F&&>;
			return dispatcher::apply(self, std::forward<F>(f));
		}

		template <typename F>
		friend constexpr auto operator |(this_type&& self, F&& f)
		{
			using dispatcher = opt_detail::pipe_dispatch<this_type&&, F&&>;
			return dispatcher::apply(std::move(self), std::forward<F>(f));
		}

		template <typename F>
		friend constexpr auto operator |(const this_type&& self, F&& f)
		{
			using dispatcher = opt_detail::pipe_dispatch<const this_type&&, F&&>;
			return dispatcher::apply(std::move(self), std::forward<F>(f));
		}

		// unwrap optional with value_or
		template <typename U>
		constexpr decltype(auto) operator/=(U&& dvalue) const &
		{
			return (*this).value_or(std::forward<U>(dvalue));
		}

		template <typename U>
		constexpr decltype(auto) operator/=(U&& dvalue) &&
		{
			return std::move(*this).value_or(std::forward<U>(dvalue));
		}

	// -------------------------------------------------------------------------
	//  update
	//
		void swap(this_type& other) noexcept(noexcept(internal_.swap(other.internal_)))
		{
			internal_.swap(other.internal_);
		}

		friend void swap(this_type& self, this_type& other) noexcept(noexcept(self.swap(other)))
		{
			self.swap(other);
		}

		void reset() noexcept
		{
			internal_.reset();
		}

		template <typename ...Args>
		T& emplace(Args&& ...args)
		{
			return internal_.emplace(std::forward<Args>(args)...);
		}

	// -------------------------------------------------------------------------
	//  bool cast
	//
		constexpr explicit operator bool() const noexcept
		{
			return static_cast<bool>(internal_);
		}

	private:
		internal_type internal_;

	}; // class optional

#if __cplusplus >= 201703L
	template <typename T>
	optional(T&&) -> optional<std::remove_cv_t<std::remove_reference_t<T>>>;
	
	template <typename T>
	optional(std::reference_wrapper<T>) -> optional<T&>;
#endif

// -----------------------------------------------------------------------------
//  make_optional
// -----------------------------------------------------------------------------
	template <typename T>
	constexpr optional<std::decay_t<T>> make_optional(T&& value)
	{
		using result_t = optional<std::decay_t<T>>;
		return result_t(std::forward<T>(value));
	}

	template <typename T, typename ...Args>
	constexpr optional<T> make_optional(Args&& ...args)
	{
		return optional<T>(in_place, std::forward<Args>(args)...);
	}

} // namespace qsb

namespace qsb { namespace opt_helpers {
// -----------------------------------------------------------------------------
//	unary_invoker
// -----------------------------------------------------------------------------
	template <typename F>
	struct unary_invoker {
		F f;

		template <typename T>
		constexpr auto operator()(T&& value) & -> tl::detail::invoke_result_t<F&, T&&>
		{
			return tl::detail::invoke(f, std::forward<T>(value));
		}

		template <typename T>
		constexpr auto operator()(T&& value) const & -> tl::detail::invoke_result_t<const F&, T&&>
		{
			return tl::detail::invoke(f, std::forward<T>(value));
		}

		template <typename T>
		constexpr auto operator()(T&& value) && -> tl::detail::invoke_result_t<F, T&&>
		{
			return tl::detail::invoke(std::move(f), std::forward<T>(value));
		}

		template <typename T>
		constexpr auto operator()(T&& value) const && -> tl::detail::invoke_result_t<const F, T&&>
		{
			return tl::detail::invoke(std::move(f), std::forward<T>(value));
		}
	};

// -----------------------------------------------------------------------------
//	nullary_invoker
// -----------------------------------------------------------------------------
	template <typename F>
	struct nullary_invoker {
		F f;

		constexpr auto operator()() const & -> tl::detail::invoke_result_t<const F&>
		{
			return tl::detail::invoke(f);
		}
		constexpr auto operator()() && -> tl::detail::invoke_result_t<F&>
		{
			return tl::detail::invoke(std::move(f));
		}
	};

// -----------------------------------------------------------------------------
//  transform
// -----------------------------------------------------------------------------
	template <typename F>
	constexpr unary_invoker<F> transform(F&& f)
	{
		return unary_invoker<F>(std::forward<F>(f));
	}

// -----------------------------------------------------------------------------
//  and_then
// -----------------------------------------------------------------------------
	template <typename F>
	constexpr unary_invoker<F> and_then(F&& f)
	{
		return unary_invoker<F>(std::forward<F>(f));
	}

// -----------------------------------------------------------------------------
//  or_else
// -----------------------------------------------------------------------------
	template <typename F>
	constexpr nullary_invoker<F> or_else(F&& f)
	{
		return nullary_invoker<F>(std::forward<F>(f));
	}

}} // namespace qsb::opt_helpers

namespace qsb { namespace opt_detail {
// -----------------------------------------------------------------------------
//  is_optional
// -----------------------------------------------------------------------------
	template <typename T>
	struct is_optional<optional<T>> : std::true_type {};

}} // namespace qsb::opt_detail

namespace std {
// -----------------------------------------------------------------------------
//  hash
// -----------------------------------------------------------------------------
	template <class T>
	struct hash<qsb::optional<T>> {
		::std::size_t operator()(const qsb::optional<T> &o) const
		{
			if (!o.has_value())
			  return 0;

			return std::hash<typename std::remove_const<T>::type>()(*o);
		}
	};

} // namespace std
