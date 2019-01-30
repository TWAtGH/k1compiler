#include "CWordScanStage.hpp"

#include <fstream>
#include <unordered_map>
#include <unordered_set>
#include <cctype>

namespace k1
{
	std::uint8_t CWordScanOut::getType() const
	{
		return static_cast<std::uint8_t>(E_PIPELINE_TYPES::WORDSCAN);
	}

	CWordScanStage::CWordScanStage(const string &filePath, IPipelineStage *prev)
		:	IPipelineStage(prev),
			mFilePath(filePath)
			
	{}

	CWordScanStage::~CWordScanStage()
	{
		if (mOutput)
		{
			delete mOutput;
			mOutput = nullptr;
		}
	}

	void CWordScanStage::execute()
	{
		ifstream input(mFilePath, ifstream::in);

		if (!input)
			throw K1T("Could not open file: ") + mFilePath;

		CWordScanOut *output = new CWordScanOut;

		typedef CWordScanOut::E_TOKEN_TYPE TokenType;
		typedef std::pair<TokenType, E_SCANSTATE> TypeToState;

		const std::unordered_map<k1char, TypeToState> tokenMap =
		{
			{
				K1T('('),
				{ TokenType::LBRA, E_SCANSTATE::LBRA }
			},
			{
				K1T(')'),
				{ TokenType::RBRA, E_SCANSTATE::RBRA }
			},
			{
				K1T('\"'),
				{ TokenType::CONSTSTR, E_SCANSTATE::QUOTE }
			}
		};
		const std::unordered_set<k1char> outsideIgnores =
		{
			K1T(' '),
			K1T('\t'),
			K1T('\r'),
			K1T('\n')
		};

		for (unsigned int lineNr = 1; !input.eof(); ++lineNr)
		{
			string line;
			std::getline(input, line);

			//analyze
			E_SCANSTATE curState = E_SCANSTATE::OUTSIDE;
			std::vector<CWordScanOut::STokenInfo>::pointer curToken = nullptr;

			for (auto c : line)
			{
				if (curState == E_SCANSTATE::LBRA)
				{
					if (c != K1T('('))
						curState = E_SCANSTATE::OUTSIDE;
					else
						curToken->value += c;
				}
				else if (curState == E_SCANSTATE::RBRA)
				{
					if (c != K1T(')'))
						curState = E_SCANSTATE::OUTSIDE;
					else
						curToken->value += c;
				}
				else if (curState == E_SCANSTATE::IDENTIFIER)
				{
					if (tokenMap.find(c) != tokenMap.cend() || outsideIgnores.find(c) != outsideIgnores.cend())
						curState = E_SCANSTATE::OUTSIDE;
					else
						curToken->value += c;
				}
				else if (curState == E_SCANSTATE::NUMBER)
				{
					if (!std::isdigit(c))
						curState = E_SCANSTATE::OUTSIDE;
					else
						curToken->value += c;
				}
				else if (curState == E_SCANSTATE::QUOTE)
				{
					if (c == K1T('"'))
					{
						curState = E_SCANSTATE::OUTSIDE;
						continue;
					}
					/*else if (c == K1T('\\'))
						curToken->value += (c = *(++curChar));*/
					else
						curToken->value += c;
				}


				if (curState == E_SCANSTATE::OUTSIDE)
				{
					if (c == K1T('#'))
						break;

					if (outsideIgnores.find(c) != outsideIgnores.cend())
						continue;

					output->tokens.push_back(CWordScanOut::STokenInfo());
					curToken = &(output->tokens.back());

					curToken->lineNr = lineNr;

					const auto mapIt = tokenMap.find(c);
					if (mapIt != tokenMap.cend())
					{
						curToken->type = mapIt->second.first;
						curState = mapIt->second.second;
					}
					else
					{
						if (std::isdigit(c))
						{
							curToken->type = TokenType::CONSTINT;
							curState = E_SCANSTATE::NUMBER;
							curToken->value = c;
						}
						else if (std::isalpha(c))
						{
							curToken->type = TokenType::IDENTIFIER;
							curState = E_SCANSTATE::IDENTIFIER;
							curToken->value = c;
						}
						else
							throw K1T("Unexpected non alphanumerical character scanned!");
					}
				}
			}
		}

		input.close();
		mOutput = output;
	}
}