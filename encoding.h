/*
 * STL streams that perform unicode encoding and decoding (with iconv)
 *
 * Copyright 2011 Janne Kulmala <janne.t.kulmala@iki.fi>
 *
 * See LICENSE file for license.
 *
 */
#ifndef _ENCODING_H
#define _ENCODING_H

#include <stdexcept>
#include <stdint.h>
#include <streambuf>
#include <istream>
#include <iconv.h>
#include <errno.h>
#include <string.h>

typedef uint32_t unicode_t;

/* An unicode string (UTF-32) */
typedef std::basic_string<unicode_t> ustring;

class conv_error: public std::runtime_error {
public:
	conv_error(const std::string &what) :
		std::runtime_error(what)
	{}
};

/* helpers for string conversions */
std::string encode(const ustring &in, const char *enc);
ustring decode(const std::string &in, const char *enc);

template <class Iter>
class dec_istreambuf: public std::basic_streambuf<unicode_t> {
public:
	dec_istreambuf(const Iter &first, const Iter &last, const std::string &enc) :
		m_cur(first), m_end(last), m_enc(enc), m_input(NULL), m_avail(0)
	{
	}

private:
	Iter m_cur, m_end;
	std::string m_enc;
	char m_readbuf[4096];
	char *m_input;
	size_t m_avail;
	unicode_t m_buf[256];

	int_type underflow()
	{
		fillbuf();
		if (m_avail == 0) {
			return traits_type::eof();
		}

		/*
		 * iconv likes to write a byte order marker to the beginning of
		 * the output if the used byte order is not specified.
		 */
#if (BYTE_ORDER == LITTLE_ENDIAN)
		iconv_t conv = iconv_open("UTF-32LE", m_enc.c_str());
#else
		iconv_t conv = iconv_open("UTF-32BE", m_enc.c_str());
#endif
		if (conv == iconv_t(-1)) {
			throw conv_error("Unable to initialize iconv");
		}

		char *out_ptr = (char *) m_buf;
		size_t out_left = sizeof m_buf;

		size_t ret = iconv(conv, &m_input, &m_avail, &out_ptr, &out_left);
		if (ret == size_t(-1)) {
			if (errno == EILSEQ) {
				throw conv_error("Invalid char sequence");
			} else if (errno != E2BIG) {
				throw conv_error("Conversion failed");
			}
		}
		iconv_close(conv);
		setg(m_buf, m_buf, (unicode_t *) out_ptr);
		return m_buf[0];
	}

	void fillbuf()
	{
		/* iconv updates m_input and m_avail */
		memmove(m_readbuf, m_input, m_avail);
		m_input = m_readbuf;

		while (m_cur != m_end && m_avail < sizeof m_readbuf) {
			m_readbuf[m_avail++] = *m_cur++;
		}
	}
};


template <class Iter>
class enc_ostreambuf: public std::basic_streambuf<unicode_t> {
public:
	enc_ostreambuf(const Iter &result, const std::string &enc) :
		m_cur(result), m_enc(enc)
	{
		setp(m_writebuf, &m_writebuf[sizeof m_writebuf / sizeof(unicode_t)]);
	}
	~enc_ostreambuf()
	{
		overflow();
	}

private:
	Iter m_cur;
	std::string m_enc;
	unicode_t m_writebuf[1024];

	int_type overflow(int_type c = traits_type::eof())
	{
		iconv_t conv = iconv_open(m_enc.c_str(), "UTF-32");
		if (conv == iconv_t(-1)) {
			throw conv_error("Unable to initialize iconv");
		}

		char *in_ptr = (char *) m_writebuf;
		size_t avail = (char *) pptr() - (char *) m_writebuf;
		while (avail) {
			char buf[1024];
			char *out_ptr = buf;
			size_t out_left = sizeof buf;

			size_t ret = iconv(conv, &in_ptr, &avail, &out_ptr, &out_left);
			if (ret == size_t(-1)) {
				if (errno == EILSEQ) {
					throw conv_error("Invalid char sequence");
				} else if (errno != E2BIG) {
					throw conv_error("Conversion failed");
				}
			}
			m_cur = std::copy(buf, out_ptr, m_cur);
		}
		iconv_close(conv);
		setp(m_writebuf, &m_writebuf[sizeof m_writebuf / sizeof(unicode_t)]);
		if (c != traits_type::eof())
			sputc(c);
		return 0;
	}
};

template <class Iter>
class dec_ostreambuf: public std::streambuf {
public:
	dec_ostreambuf(const Iter &result, const std::string &enc) :
		m_cur(result), m_enc(enc)
	{
		setp(m_writebuf, &m_writebuf[sizeof m_writebuf]);
	}
	~dec_ostreambuf()
	{
		overflow();
	}

private:
	Iter m_cur;
	std::string m_enc;
	char m_writebuf[4096];

	int_type overflow(int_type c = traits_type::eof())
	{
		/*
		 * iconv likes to write a byte order marker to the beginning of
		 * the output if the used byte order is not specified.
		 */
#if (BYTE_ORDER == LITTLE_ENDIAN)
		iconv_t conv = iconv_open("UTF-32LE", m_enc.c_str());
#else
		iconv_t conv = iconv_open("UTF-32BE", m_enc.c_str());
#endif
		if (conv == iconv_t(-1)) {
			throw conv_error("Unable to initialize iconv");
		}

		char *in_ptr = m_writebuf;
		size_t avail = pptr() - m_writebuf;
		while (avail) {
			unicode_t buf[256];
			char *out_ptr = (char *) buf;
			size_t out_left = sizeof buf;

			size_t ret = iconv(conv, &in_ptr, &avail, &out_ptr, &out_left);
			if (ret == size_t(-1)) {
				if (errno == EILSEQ) {
					throw conv_error("Invalid char sequence");
				} else if (errno != E2BIG) {
					throw conv_error("Conversion failed");
				}
			}
			m_cur = std::copy(buf, (unicode_t *) out_ptr, m_cur);
		}
		iconv_close(conv);
		setp(m_writebuf, &m_writebuf[sizeof m_writebuf]);
		if (c != traits_type::eof())
			sputc(c);
		return 0;
	}
};

template <class Iter>
class enc_istreambuf: public std::streambuf {
public:
	enc_istreambuf(const Iter &first, const Iter &last, const std::string &enc) :
		m_cur(first), m_end(last), m_enc(enc), m_input(NULL), m_avail(0)
	{
	}

private:
	Iter m_cur, m_end;
	std::string m_enc;
	unicode_t m_readbuf[1024];
	char *m_input;
	size_t m_avail;
	char m_buf[1024];

	int_type underflow()
	{
		fillbuf();
		if (m_avail == 0) {
			return traits_type::eof();
		}

		iconv_t conv = iconv_open(m_enc.c_str(), "UTF-32");
		if (conv == iconv_t(-1)) {
			throw conv_error("Unable to initialize iconv");
		}

		char *out_ptr = m_buf;
		size_t out_left = sizeof m_buf;

		size_t ret = iconv(conv, &m_input, &m_avail, &out_ptr, &out_left);
		if (ret == size_t(-1)) {
			if (errno == EILSEQ) {
				throw conv_error("Invalid char sequence");
			} else if (errno != E2BIG) {
				throw conv_error("Conversion failed");
			}
		}
		iconv_close(conv);
		setg(m_buf, m_buf, out_ptr);
		return m_buf[0];
	}

	void fillbuf()
	{
		/* iconv updates m_input and m_avail */
		memmove(m_readbuf, m_input, m_avail);
		m_input = (char *) m_readbuf;

		/* Note, m_avail is in bytes */
		while (m_cur != m_end && m_avail < sizeof m_readbuf) {
			m_readbuf[m_avail / sizeof(unicode_t)] = *m_cur++;
			m_avail += sizeof(unicode_t);
		}
	}
};

/*
 * An input stream that reads bytes from an input iterator and decodes them as
 * unicode. Can be used to read unicode from a file.
 */
template <class Iter>
class dec_istream: public std::basic_istream<unicode_t> {
public:
	dec_istream(const Iter &first, const Iter &last, const std::string &enc) :
		std::basic_istream<unicode_t>(&streambuf),
		streambuf(first, last, enc)
	{
	}

private:
	dec_istreambuf<Iter> streambuf;
};

/*
 * An output stream that encodes unicode and writes the bytes with an output
 * iterator. Can be used to write unicode to a file.
 */
template <class Iter>
class enc_ostream: public std::basic_ostream<unicode_t> {
public:
	enc_ostream(const Iter &result, const std::string &enc) :
		std::basic_ostream<unicode_t>(&streambuf),
		streambuf(result, enc)
	{
	}

private:
	enc_ostreambuf<Iter> streambuf;
};

/*
 * An output stream that decodes written bytes and writes the unicode with an
 * output iterator. Can be used to push data for decoding.
 */
template <class Iter>
class dec_ostream: public std::ostream {
public:
	dec_ostream(const Iter &result, const std::string &enc) :
		std::ostream(&streambuf),
		streambuf(result, enc)
	{
	}

private:
	dec_ostreambuf<Iter> streambuf;
};

/*
 * An input stream that reads unicode from an input iterator and encodes them
 * them as bytes. Can be used to pull-style encoding.
 */
template <class Iter>
class enc_istream: public std::istream {
public:
	enc_istream(const Iter &first, const Iter &last, const std::string &enc) :
		std::istream(&streambuf),
		streambuf(first, last, enc)
	{
	}

private:
	enc_istreambuf<Iter> streambuf;
};

#endif
