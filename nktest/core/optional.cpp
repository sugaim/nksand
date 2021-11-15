#include <type_traits>
#include <cstdlib>
#include "gtest/gtest.h"
#include "../../nksand/core/utility/optional.h"

TEST(Optional, HasValue) {
	qsb::optional<int> invalid0;
	EXPECT_FALSE(invalid0.has_value());

	qsb::optional<int> invalid1 { qsb::nullopt };
	EXPECT_FALSE(invalid1.has_value());

	qsb::optional<int> valid0 = 42;
	EXPECT_TRUE(valid0.has_value());
}

TEST(Optional, AsBool) {
	qsb::optional<int> invalid0;
	EXPECT_FALSE(invalid0);
	EXPECT_TRUE(!invalid0);

	qsb::optional<int> invalid1 { qsb::nullopt };
	EXPECT_FALSE(invalid1);
	EXPECT_TRUE(!invalid1);

	qsb::optional<int> valid0 = 42;
	EXPECT_TRUE(valid0);
	EXPECT_FALSE(!valid0);
}

TEST(Optional, Assign) {
	qsb::optional<int> invalid0;
	EXPECT_FALSE(invalid0.has_value());

	invalid0 = 42;
	EXPECT_TRUE(invalid0.has_value());
	EXPECT_TRUE(invalid0);
	EXPECT_EQ(*invalid0, 42);
}

struct test {
	void non_const() {}
	void nothrow() const {}
};

TEST(Optional, AccessInvalidValue) {
	//
	//	not tested all, such as rvalue related,
	//	return value type (ref->ref, cref->cref and so on)
	//
	qsb::optional<test> invalid0;
	EXPECT_ANY_THROW(invalid0.value());
	EXPECT_NO_THROW(*invalid0);				// no check
	EXPECT_NO_THROW(invalid0->nothrow());	// no check
	EXPECT_NO_THROW(invalid0->non_const());
	EXPECT_NO_THROW((*invalid0).non_const());
	EXPECT_NO_THROW((*invalid0).nothrow());

	const qsb::optional<test> invalid1;
	EXPECT_ANY_THROW(invalid1.value());
	EXPECT_NO_THROW(*invalid1);				// no check
	EXPECT_NO_THROW(invalid1->nothrow());	// no check
	EXPECT_NO_THROW((*invalid1).nothrow());
}

TEST(Optional, AccessValidValue) {
	qsb::optional<int> valid0 = 42;
	EXPECT_NO_THROW(valid0.value());
	EXPECT_EQ(valid0.value(), 42);
	EXPECT_NO_THROW(*valid0);
	EXPECT_EQ(*valid0, 42);

	const qsb::optional<int> valid1 = 42;
	EXPECT_NO_THROW(valid1.value());
	EXPECT_EQ(valid1.value(), 42);
	EXPECT_NO_THROW(*valid1);
	EXPECT_EQ(*valid1, 42);
}

TEST(Optional, ValueOr) {
	qsb::optional<int> invalid0;
	EXPECT_NO_THROW(invalid0.value_or(24));
	EXPECT_EQ(invalid0.value_or(24), 24);

	const qsb::optional<int> invalid1;
	EXPECT_NO_THROW(invalid1.value_or(24));
	EXPECT_EQ(invalid1.value_or(24), 24);

	qsb::optional<int> invalid2;
	EXPECT_NO_THROW(invalid2.value_or(true));
	static_assert(std::is_same<int, decltype(invalid2.value_or(true))>::value, "");
	EXPECT_EQ(invalid2.value_or(true), 1);	// cast to int

	qsb::optional<int> valid0 = 42;
	EXPECT_NO_THROW(valid0.value_or(24));
	EXPECT_EQ(valid0.value_or(24), 42);

	const qsb::optional<int> valid1 = 42;
	EXPECT_NO_THROW(valid1.value_or(24));
	EXPECT_EQ(valid1.value_or(24), 42);
}

namespace qtest {
	qsb::optional<int> get_opt(int x, bool b)
	{
		return b ? qsb::optional<int>(x) : qsb::optional<int>{};
	}

} // namespace qtest

TEST(Optional, AndThen) {
	const auto twice_opt = [](int x) { return qsb::optional<int>(2 * x); };
	auto invalid0 = qtest::get_opt(21, false).and_then(twice_opt);
	EXPECT_FALSE(invalid0);

	auto valid0 = qtest::get_opt(21, true).and_then(twice_opt);
	EXPECT_TRUE(valid0);
	EXPECT_EQ(valid0.value(), 42);
}

TEST(Optional, Transform) {
	const auto twice = [](int x) { return 2 * x; };
	auto invalid0 = qtest::get_opt(21, false).transform(twice);
	EXPECT_FALSE(invalid0);

	auto valid0 = qtest::get_opt(21, true).transform(twice);
	EXPECT_TRUE(valid0);
	EXPECT_EQ(valid0.value(), 42);
}

TEST(Optional, OrElse) {
	const auto throw_exception = []{ throw "exception"; };
	EXPECT_ANY_THROW(qtest::get_opt(42, false).or_else(throw_exception));

	EXPECT_NO_THROW(qtest::get_opt(42, true).or_else(throw_exception));
	auto valid0 = qtest::get_opt(42, true).or_else(throw_exception);
	EXPECT_TRUE(valid0);
	EXPECT_EQ(valid0.value(), 42);
}

TEST(Optional, OperatorSupport) {
	const auto twice_opt = [](int x) { return qsb::optional<int>(2 * x); };
	const auto twice = [](int x) { return 2 * x; };
	const auto validator = []{ throw "optional has not value"; };

	int i = 0;
	const auto overwriter = [&i]{ return ++i; };

	// pipe: optional<T> -> optional<U>
	// /=  : optional<T> -> T
	EXPECT_FALSE(qtest::get_opt(21, false) | twice | twice_opt);
	EXPECT_FALSE(qtest::get_opt(21, false) | twice_opt | twice);
	EXPECT_EQ(qtest::get_opt(21, false) | twice_opt | twice /= 42, 42);
	EXPECT_ANY_THROW(qtest::get_opt(21, false) | twice_opt | validator | twice);
	EXPECT_EQ(qtest::get_opt(21, false) | twice_opt | overwriter | twice /= 42, 2);
	EXPECT_EQ(qtest::get_opt(21, false) | twice_opt | overwriter | twice /= 42, 4);

	EXPECT_TRUE(qtest::get_opt(21, true) | twice | twice_opt);
	EXPECT_TRUE(qtest::get_opt(21, true) | twice_opt | twice);
	EXPECT_NO_THROW(qtest::get_opt(21, true) | twice_opt | validator | twice);
	EXPECT_EQ(qtest::get_opt(21, true) | twice_opt | validator | twice /= 42, 84);
	EXPECT_EQ(qtest::get_opt(21, true) | twice_opt | overwriter | twice /= 42, 84);
}
