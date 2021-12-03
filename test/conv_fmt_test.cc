#include "main.h"

struct PrintfHelper
{
	static std::string format(const char *fmt, ...)
	{
		putc_data.clear();
		va_list args;
		va_start(args, fmt);
		strf_print(PrintfHelper::putc, fmt, args);
		va_end(args);
		return std::string(putc_data.data(), putc_data.size());
	}
	
	static std::vector<char> putc_data;
	static void putc(char c)
	{
		putc_data.push_back(c);
	}
};
std::vector<char> PrintfHelper::putc_data;

struct SprintfHelper
{
	static std::string format(const char *fmt, ...)
	{
		std::vector<char> data(1024, 0);
		va_list args;
		va_start(args, fmt);
		strf_sprint(data.data(), data.size(), fmt, args);
		va_end(args);
		return std::string(data.data());
	}
};

template <typename T>
class ConvFmtTest : public testing::Test
{
public:
	using Helper = T;
};

using ConvFmtTestTypes = ::testing::Types<SprintfHelper, PrintfHelper>;
TYPED_TEST_SUITE(ConvFmtTest, ConvFmtTestTypes);


TYPED_TEST(ConvFmtTest, basic) {
	EXPECT_EQ(TypeParam::format(""), "");
	EXPECT_EQ(TypeParam::format("abc"), "abc");
	EXPECT_EQ(TypeParam::format("%%"), "%");
	EXPECT_EQ(TypeParam::format("a%%%%c"), "a%%c");
	EXPECT_EQ(TypeParam::format("%"), "%?");
	EXPECT_EQ(TypeParam::format("%Q"), "%Q");
	
	// Kinda an idiosyncrasy, but bad types are formattable...
	EXPECT_EQ(TypeParam::format("%#^6Q"), "##%Q##");
}

TYPED_TEST(ConvFmtTest, string) {
	EXPECT_EQ(TypeParam::format("%s", ""), "");
	EXPECT_EQ(TypeParam::format("%s", "abc"), "abc");
	EXPECT_EQ(TypeParam::format("%s", "%i"), "%i");

	EXPECT_EQ(TypeParam::format("x%<sx", "abc"), "xabcx");
	EXPECT_EQ(TypeParam::format("x%<3sx", "abc"), "xabcx");
	EXPECT_EQ(TypeParam::format("x%^3sx", "abc"), "xabcx");
	EXPECT_EQ(TypeParam::format("x%>3sx", "abc"), "xabcx");
	EXPECT_EQ(TypeParam::format("x%<4sx", "abc"), "xabc x");
	EXPECT_EQ(TypeParam::format("x%10sx", "abc"), "x       abcx");
	EXPECT_EQ(TypeParam::format("x%<10sx", "abc"), "xabc       x");
	EXPECT_EQ(TypeParam::format("x%/<10sx", "abc"), "xabc///////x");
	EXPECT_EQ(TypeParam::format("x%/-10sx", "abc"), "xabc///////x");
	EXPECT_EQ(TypeParam::format("x%/>10sx", "abc"), "x///////abcx");
	EXPECT_EQ(TypeParam::format("x%/=10sx", "abc"), "x///////abcx");
	EXPECT_EQ(TypeParam::format("x%/^10sx", "abc"), "x///abc////x");
	
	EXPECT_EQ(TypeParam::format("x%/^10.12sx", "abc"), "x///abc////x");
	EXPECT_EQ(TypeParam::format("x%/^10.3sx", "abc"), "x///abc////x");
	EXPECT_EQ(TypeParam::format("x%/^10.2sx", "abc"), "x////ab////x");
	EXPECT_EQ(TypeParam::format("x%/^10.1sx", "abc"), "x////a/////x");
	EXPECT_EQ(TypeParam::format("x%/^10.0sx", "abc"), "x//////////x");
	EXPECT_EQ(TypeParam::format("x%/^10.sx", "abc"), "x//////////x");
	
	EXPECT_EQ(TypeParam::format("x%/^*.*s%#^*.*sx", 10, 2, "abc", 8, 4, "xyz"), "x////ab////##xyz###x");
	EXPECT_EQ(TypeParam::format("x%/^*.*s%#>*.*sx", 3, 4, "abc", 3, 2, "xyz"), "xabc#xyx");
}

TYPED_TEST(ConvFmtTest, ints) {
	EXPECT_EQ(TypeParam::format("x%ix", 1234), "x1234x");
	EXPECT_EQ(TypeParam::format("x%ux", 1234), "x1234x");
	EXPECT_EQ(TypeParam::format("x%ix", -1234), "x-1234x");
	EXPECT_EQ(TypeParam::format("x%ux", -1234), "x4294966062x");
	
	EXPECT_EQ(TypeParam::format("x%xx", 0xFF), "xffx");
	EXPECT_EQ(TypeParam::format("x%xx", -0xFF), "xffffff01x");
	EXPECT_EQ(TypeParam::format("x%Xx", 0xFF), "xFFx");
	EXPECT_EQ(TypeParam::format("x%ox", 0773), "x773x");
	EXPECT_EQ(TypeParam::format("x%bx", 0xA5), "x10100101x");
	EXPECT_EQ(TypeParam::format("x%px", 0xA5), "xA5x");

	// The normal "string" aligns should work without special behavor (e.g for sign)
	EXPECT_EQ(TypeParam::format("x%10ix", -1234), "x     -1234x");
	EXPECT_EQ(TypeParam::format("x%@<10ix", -1234), "x-1234@@@@@x");
	EXPECT_EQ(TypeParam::format("x%@^10ix", -1234), "x@@-1234@@@x");
	EXPECT_EQ(TypeParam::format("x%@>10ix", -1234), "x@@@@@-1234x");
	
	// The number align should now do useful stuff, note '0' override string format
	EXPECT_EQ(TypeParam::format("x%@=10ix", -1234), "x-@@@@@1234x");
	EXPECT_EQ(TypeParam::format("x%010ix", -1234), "x-000001234x");
	EXPECT_EQ(TypeParam::format("x%@^010ix", -1234), "x-000001234x");
	
	// Test sign handling function
	EXPECT_EQ(TypeParam::format("x%+010ix", -1234), "x-000001234x");
	EXPECT_EQ(TypeParam::format("x%+010ix", +1234), "x+000001234x");
	EXPECT_EQ(TypeParam::format("x% 010ix", -1234), "x-000001234x");
	EXPECT_EQ(TypeParam::format("x% 010ix", +1234), "x 000001234x");
	EXPECT_EQ(TypeParam::format("x%@<+10ix", +1234), "x+1234@@@@@x");
	
	// Test precision
	// Note that standard printf allows "%6.4i" to give " -0123" for -123 but
	// that isn't supported here.  While it wouldn't be terrible, it would
	// be overhead for something of limited value (versus just using fill).
	// So basically precision doesn't do anything for ints
	EXPECT_EQ(TypeParam::format("x%.3ix", -1234), "x-1234x");
	EXPECT_EQ(TypeParam::format("x%.5ix", -1234), "x-1234x");
	EXPECT_EQ(TypeParam::format("x%7.5ix", -1234), "x  -1234x");
	
	// The '$' type for "fixed" point, e.g. "1000" means "10.00"
	EXPECT_EQ(TypeParam::format("x%$x", 1234), "x12.34x");
	EXPECT_EQ(TypeParam::format("x%.0$x", 1234), "x1234.x");
	EXPECT_EQ(TypeParam::format("x%.1$x", 1234), "x123.4x");
	EXPECT_EQ(TypeParam::format("x%.2$x", 1234), "x12.34x");
	EXPECT_EQ(TypeParam::format("x%.3$x", 1234), "x1.234x");
	EXPECT_EQ(TypeParam::format("x%.4$x", 1234), "x.1234x");
	EXPECT_EQ(TypeParam::format("x%.5$x", 1234), "x.01234x");
	EXPECT_EQ(TypeParam::format("x%.99$x", -1234), "x-.00000000000000000000000000001234x");
	EXPECT_EQ(TypeParam::format("x%010$x", 1234), "x0000012.34x");
	EXPECT_EQ(TypeParam::format("x%010.6$x", 1234), "x000.001234x");
	EXPECT_EQ(TypeParam::format("x%010.6$x", -1234), "x-00.001234x");
	EXPECT_EQ(TypeParam::format("x%+010.6$x", 1234), "x+00.001234x");
	EXPECT_EQ(TypeParam::format("x%+10.6$x", 1234), "x  +.001234x");
}

TYPED_TEST(ConvFmtTest, floats) {
	// Nothing is supported, since float support on current platforms isn't needed
	EXPECT_EQ(TypeParam::format("x%fx", 12.34), "x<float?>x");
	EXPECT_EQ(TypeParam::format("x%Fx", 12.34), "x<float?>x");
	EXPECT_EQ(TypeParam::format("x%ex", 12.34), "x<float?>x");
	EXPECT_EQ(TypeParam::format("x%Ex", 12.34), "x<float?>x");
	EXPECT_EQ(TypeParam::format("x%gx", 12.34), "x<float?>x");
	EXPECT_EQ(TypeParam::format("x%Gx", 12.34), "x<float?>x");
	EXPECT_EQ(TypeParam::format("x%ax", 12.34), "x<float?>x");
	EXPECT_EQ(TypeParam::format("x%Ax", 12.34), "x<float?>x");

	// The normal "string" aligns should work without special behavor (e.g for sign)
	EXPECT_EQ(TypeParam::format("x%13fx", -12.34), "x     <float?>x");
	EXPECT_EQ(TypeParam::format("x%@<13fx", -12.34), "x<float?>@@@@@x");
	EXPECT_EQ(TypeParam::format("x%@^13fx", -12.34), "x@@<float?>@@@x");
	EXPECT_EQ(TypeParam::format("x%@>13fx", -12.34), "x@@@@@<float?>x");
}
