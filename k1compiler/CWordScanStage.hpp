#pragma once

#include "IPipelineStage.hpp"
#include "k1types.h"
#include <vector>

namespace k1
{
	class CWordScanOut : public IPipelineData
	{
	public:
		enum class E_TOKEN_TYPE : std::uint16_t
		{
			IDENTIFIER = 1,
			LBRA,
			RBRA,
			CONSTINT,
			CONSTSTR,
		};

		struct STokenInfo
		{
			E_TOKEN_TYPE type;
			string value;
			unsigned int lineNr;
		};

	public:
		virtual std::uint8_t getType() const;
		std::vector<STokenInfo> tokens;
	};

	class CWordScanStage : public IPipelineStage
	{
	public:
		enum class E_SCANSTATE : std::uint16_t
		{
			OUTSIDE = 0,
			IDENTIFIER,
			LBRA,
			RBRA,
			NUMBER,
			QUOTE,
		};

	public:
		CWordScanStage(const string &filePath, IPipelineStage *prev = nullptr);
		~CWordScanStage();
		
		virtual void execute();

	private:
		string mFilePath;
	};
}