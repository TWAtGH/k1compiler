#include "CExceptions.hpp"
#include <sstream>

namespace k1
{

	CParseError::CParseError()
	{}

	CParseError::CParseError(const string &msg)
		:mMsg(msg)
	{}

	const string &CParseError::toString() const
	{
		return mMsg;
	}

	CUnexpectedToken::CUnexpectedToken(const string &expected, const CWordScanOut::STokenInfo &given)
	{
		ostringstream ss;
		ss << K1T("Unexpected token \"") << given.value;
		ss << K1T("\" at line '") << given.lineNr;
		ss << K1T("'! Expected: \"") << expected;
		ss << K1T("\".");

		mMsg = ss.str();
	}

	CEndOfProgram::CEndOfProgram(const CWordScanOut::STokenInfo &given)
	{
		ostringstream ss;
		ss << K1T("Unexpected end of program at line '");
		ss << given.lineNr;
		ss << K1T("'! More tokens available!");
	
		mMsg = ss.str();
	}

	CEndOfInput::CEndOfInput(const string &msg)
	{
		mMsg = msg;
	}
}