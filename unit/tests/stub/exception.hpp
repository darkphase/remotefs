#ifndef STUB_EXCEPTION_HPP
#define STUB_EXCEPTION_HPP

#include <errno.h>
#include <string.h>

#include <exception>

#define EXCEPTION(errno_val) Exception((__LINE__), (errno_val))

namespace Stub
{
	class Exception : public std::exception
	{
	public:
		Exception(unsigned line, int err) : m_line(line), m_errno(err) {}

		unsigned get_line() const {  return m_line; }
		int get_errno() const { return m_errno; }
		const char *describe() const { return strerror(m_errno); }
		
	protected:
		unsigned m_line;
		int m_errno;
	};
}

#endif // STUB_EXCEPTION_HPP

