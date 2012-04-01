/*
 * STL streams that perform unicode encoding and decoding (with iconv)
 *
 * Copyright 2011 Janne Kulmala <janne.t.kulmala@iki.fi>
 *
 * See LICENSE file for license.
 *
 */
#include "encoding.h"
#include <iterator>
#include <algorithm>

std::string encode(const ustring &in, const char *enc)
{
	enc_istream<ustring::const_iterator> is(in.begin(), in.end(), enc);
	return std::string(std::istreambuf_iterator<char>(is),
			   std::istreambuf_iterator<char>());
}

ustring decode(const std::string &in, const char *enc)
{
	dec_istream<std::string::const_iterator> is(in.begin(), in.end(), enc);
	return ustring(std::istreambuf_iterator<unicode_t>(is),
		       std::istreambuf_iterator<unicode_t>());
}

