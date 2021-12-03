#include "main.h"

std::vector<char> *g_com_putc_data = nullptr;
std::vector<char> *g_com_getc_data = nullptr;

extern "C" void com_putc(char c)
{
	if (g_com_putc_data)
	{
		g_com_putc_data->push_back(c);
	}
}

extern "C" char com_getc()
{
	char r = 0;
	if (g_com_getc_data && !g_com_getc_data->empty())
	{
		r = g_com_getc_data->back();
		g_com_getc_data->pop_back();
	}
	return r;
}

TEST(main, typedefs)
{
	EXPECT_EQ(sizeof(int8_t), 1);
	EXPECT_EQ(sizeof(int16_t), 2);
	EXPECT_EQ(sizeof(int32_t), 4);
	EXPECT_EQ(sizeof(uint8_t), 1);
	EXPECT_EQ(sizeof(uint16_t), 2);
	EXPECT_EQ(sizeof(uint32_t), 4);
}

int main(int argc, char **argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

