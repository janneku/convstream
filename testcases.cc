#include "encoding.h"
#include <stdio.h>
#include <assert.h>
#include <iterator>
#include <sstream>

ustring decode_push(const std::string &in, const char *enc)
{
	std::basic_ostringstream<unicode_t> out;
	{
		dec_ostream<std::ostreambuf_iterator<unicode_t> > os(
			std::ostreambuf_iterator<unicode_t>(out), enc);
		os << in;
	}
	return out.str();
}

std::string encode_push(const ustring &in, const char *enc)
{
	std::ostringstream out;
	{
		enc_ostream<std::ostreambuf_iterator<char> > os(
			std::ostreambuf_iterator<char>(out), enc);
		std::copy(in.begin(), in.end(),
			std::ostreambuf_iterator<unicode_t>(os));
	}
	return out.str();
}


int main()
{
	const uint32_t in_chars[] = {' ', 'f', 'o', 'o', 0x2660, 'b', 'a', 'r', ' '};
	ustring in(in_chars, sizeof in_chars / sizeof(uint32_t));
	const uint8_t utf8_cmp[] =
	{' ', 'f', 'o', 'o', 0xe2, 0x99, 0xa0, 'b', 'a', 'r', ' '};

	std::string utf8 = encode(in, "UTF-8");
	assert(utf8 == std::string((const char *) utf8_cmp, sizeof utf8_cmp));

	utf8 = encode_push(in, "UTF-8");
	assert(utf8 == std::string((const char *) utf8_cmp, sizeof utf8_cmp));

	ustring out = decode(utf8, "UTF-8");
	assert(in == out);

	out = decode_push(utf8, "UTF-8");
	assert(in == out);

	printf("ok\n");
	return 0;
}
