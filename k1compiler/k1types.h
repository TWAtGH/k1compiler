#pragma once

#include <string>
#include <cstdint>

namespace k1
{
#ifdef _UNICODE
#define K1T(str) (L ## str)
	typedef wchar_t k1char;
	typedef std::wstring string;
	typedef std::wostringstream ostringstream;
	typedef std::wifstream ifstream;
#else
#define K1T(str) (str)
	typedef char k1char;
	typedef std::string string;
	typedef std::ostringstream ostringstream;
	typedef std::ifstream ifstream;
#endif

	enum class E_PIPELINE_TYPES : std::uint8_t
	{
		UNSPECIFIC = 0,
		WORDSCAN,
		TOKENPARSE,
		CODEGEN,
		CODEEXECUTION,

		COUNT
	};
}