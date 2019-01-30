#pragma once

#include "k1types.h"
#include "CWordScanStage.hpp"

namespace k1
{
	class CParseError
	{
	public:
		CParseError();
		CParseError(const string &msg);
		const string &toString() const;

	protected:
		string mMsg;
	};

	class CUnexpectedToken : public CParseError
	{
	public:
		CUnexpectedToken(const string &expected, const CWordScanOut::STokenInfo &given);
	};

	class CEndOfProgram : public CParseError
	{
	public:
		CEndOfProgram(const CWordScanOut::STokenInfo &given);
	};

	class CEndOfInput : public CParseError
	{
	public:
		CEndOfInput(const string &msg);
	};
}