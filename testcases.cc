#include "encoding.h"
#include <stdio.h>
#include <assert.h>

int main()
{
	const uint32_t in_chars[] = {'f', 'o', 'o', 0x2660, 'b', 'a', 'r'};
	ustring in(in_chars, sizeof in_chars / sizeof(uint32_t));

	std::string utf8 = encode(in, "UTF-8");

	const uint8_t utf8_cmp[] =
	{'f', 'o', 'o', 0xe2, 0x99, 0xa0, 'b', 'a', 'r'};
	assert(utf8 == std::string((const char *) utf8_cmp, sizeof utf8_cmp));

	ustring out = decode(utf8, "UTF-8");

	assert(in == out);

	printf("ok\n");
	return 0;
}
