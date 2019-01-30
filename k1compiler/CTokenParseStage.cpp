#include "CTokenParseStage.hpp"
#include "CExceptions.hpp"
#include <unordered_set>

#ifdef _DEBUG
#include <iostream>
#endif


namespace k1
{

#ifdef _DEBUG
	static inline string NTToStr(CTokenParseOut::CNode::E_NODE_TYPE t)
	{
		typedef CTokenParseOut::CNode::E_NODE_TYPE nt;
		switch (t)
		{
		case nt::UNDEFINED:
			return K1T("UNDEFINED");
		case nt::PROG:
			return K1T("PROG");
		case nt::PROC:
			return K1T("PROC");
		case nt::MAIN:
			return K1T("MAIN");
		case nt::EXITLOOP:
			return K1T("EXITLOOP");
		case nt::LOOP:
			return K1T("LOOP");
		case nt::CASE:
			return K1T("CASE");
		case nt::PRINT:
			return K1T("PRINT");
		case nt::INPUT:
			return K1T("INPUT");
		case nt::ASSIGN:
			return K1T("ASSIGN");
		case nt::INTOP:
			return K1T("INTOP");
		case nt::STROP:
			return K1T("STROP");
		case nt::CALL:
			return K1T("CALL");
		case nt::CONSTINT:
			return K1T("CONSTINT");
		case nt::CONSTSTR:
			return K1T("CONSTSTR");
		case nt::LOCALVAR:
			return K1T("LOCALVAR");
		case nt::RESULTVAR:
			return K1T("RESULTVAR");
		case nt::PARAM:
			return K1T("PARAM");
		case nt::OPERATOR:
			return K1T("OPERATOR");
		case nt::EXPRESSION:
			return K1T("EXPRESSION");
		case nt::WHEN:
			return K1T("WHEN");
		case nt::OTHERWISE:
			return K1T("OTHERWISE");
		};
		return K1T("UNKNOWN");
	}
	void CTokenParseOut::CNode::dump(int l) const
	{
		for (int i = 0; i < l; ++i)
			std::wcout << K1T("-");
		std::wcout << NTToStr(mData.type);
		std::wcout << K1T(" [") << mData.value << K1T("]");
		std::wcout << std::endl;
		for (const auto &it : mNodes)
			it->dump(l + 1);
	}
#endif

	CTokenParseOut::CTokenParseOut()
		:	mRoot(nullptr)
	{
		mRoot = new CNode(nullptr);
	}

	std::uint8_t CTokenParseOut::getType() const
	{
		return static_cast<std::uint8_t>(E_PIPELINE_TYPES::TOKENPARSE);
	}

	CTokenParseOut::~CTokenParseOut()
	{
		if (mRoot)
		{
			delete mRoot;
			mRoot = nullptr;
		}
	}

	CTokenParseOut::CNode::CNode(CNode *parent)
		:	mParent(parent)
	{}

	auto CTokenParseOut::CNode::createChild(E_NODE_TYPE type, const string &val) -> CNode*
	{
		CNode *node = new CNode(this);

		node->mData.type = type;
		node->mData.value = val;

		mNodes.push_back(node);
		return node;
	}

	auto CTokenParseOut::CNode::createSib(E_NODE_TYPE type, const string &val) -> CNode*
	{
		if (!mParent)
			throw CParseError( K1T("Root cannot have sibling!") );
		return mParent->createChild(type, val);
	}

	auto CTokenParseOut::CNode::getParent() const -> const CNode*
	{
		return mParent;
	}
	auto CTokenParseOut::CNode::getParent() -> CNode*
	{
		return mParent;
	}

	auto CTokenParseOut::CNode::getChilds() const -> const std::vector<CNode*>&
	{
		return mNodes;
	}
	auto CTokenParseOut::CNode::getChilds() -> std::vector<CNode*>&
	{
		return mNodes;
	}

	auto CTokenParseOut::CNode::getChild(std::vector<CNode*>::size_type i) const -> const CNode*
	{
		return mNodes[i];
	}
	auto CTokenParseOut::CNode::getChild(std::vector<CNode*>::size_type i) -> CNode*
	{
		return mNodes[i];
	}

	CTokenParseOut::CNode::~CNode()
	{
		while (!mNodes.empty())
		{
			delete mNodes.back();
			mNodes.pop_back();
		}
	}

	CTokenParseOut::SSymbol::SSymbol(const string &p, const string &n, E_SYMBOL_TYPE t, E_SYMBOL_ACCESS a)
		:	proc(p),
			name(n),
			type(t),
			access(a)
	{}
	
	auto CTokenParseOut::getSymbol(const string &s) const -> const CTokenParseOut::SSymbol&
	{
		auto ret = mParams.find(s);
		if (ret != mParams.cend())
			return (ret->second);

		ret = mDeclarations.find(s);
		if (ret == mParams.cend())
			throw K1T("");

		return (ret->second);
	}
	auto CTokenParseOut::getSymbol(const string &s) -> CTokenParseOut::SSymbol&
	{
		auto ret = mParams.find(s);
		if (ret != mParams.cend())
			return (ret->second);

		ret = mDeclarations.find(s);
		if (ret == mDeclarations.cend())
			throw K1T("");

		return (ret->second);
	}

	bool CTokenParseOut::hasDeclared(const string &s) const
	{
		return (mDeclarations.find(s) != mDeclarations.cend());
	}
	bool CTokenParseOut::hasParam(const string &s) const
	{
		return (mParams.find(s) != mParams.cend());
	}
	bool CTokenParseOut::hasSymbol(const string &s) const
	{
		return (hasParam(s) || hasDeclared(s));
	}
	bool CTokenParseOut::isValidSymbol(const string& s, E_SYMBOL_ACCESS access) const
	{
		const auto &param = mParams.find(s);
		if (param != mParams.cend())
		{
			if (param->second.access == access)
				return false;
		}
		else if (!hasDeclared(s))
			return false;

		return true;
	}


	CTokenParseStage::CTokenParseStage(IPipelineStage *prev)
		:	IPipelineStage(prev),
			mCurNode(nullptr)
	{}

	CTokenParseStage::~CTokenParseStage()
	{
		if (mOutput)
		{
			delete mOutput;
			mOutput = nullptr;
		}
	}

	//*************************************************************
	void CTokenParseStage::execute()
	{
		if (!mPreviousStage->getData())
			throw CParseError( K1T("No input data given!") );

		CWordScanOut *input = dynamic_cast<CWordScanOut*>(mPreviousStage->getData());
		if (!input)
			throw CParseError( K1T("Unknown input data format!") );

		mParseOut = new CTokenParseOut;
		mOutput = mParseOut;

		mCurToken = input->tokens.cbegin();
		mCurNode = mParseOut->mRoot;
		try
		{
			tok_program();
		}
		catch (const std::out_of_range&)
		{
			throw CEndOfInput(K1T("Expected more tokens!"));
		}
		
		if (mCurToken != input->tokens.cend())
			throw CEndOfProgram(*(mCurToken-1));

#ifdef _DEBUG
		mParseOut->mRoot->dump();
		std::wcout << K1T("Params:\n");
		for (const auto &symbol : mParseOut->mParams)
			std::wcout << symbol.first << std::endl;
		std::wcout << K1T("Declarations:\n");
		for (const auto &symbol : mParseOut->mDeclarations)
			std::wcout << symbol.first << std::endl;
#endif
	}

	bool CTokenParseStage::curHasVal(const string &s) const
	{
		return (mCurToken->value == s);
	}

	void CTokenParseStage::tok_program()
	{
		if (!curHasVal(K1T("program")))
			throw CUnexpectedToken(K1T("program"), *mCurToken);
		++mCurToken;

		mCurNode = mCurNode->createChild(NodeType::PROG, K1T("K1Program"));
		do
		{
			tok_procedure();
		} while (curHasVal(K1T("procedure")));

		tok_main();

		if (!curHasVal(K1T("end")))
			throw CUnexpectedToken(K1T("end"), *mCurToken);
		++mCurToken;

		mCurNode = mCurNode->getParent();
	}

	void CTokenParseStage::tok_main()
	{
		if (!curHasVal(K1T("main")))
			throw CUnexpectedToken(K1T("main"), *mCurToken);
		++mCurToken;
		mCurNode->createChild(NodeType::MAIN, tok_procName());
	}

	void CTokenParseStage::tok_procedure()
	{
		if (!curHasVal(K1T("procedure")))
			throw CUnexpectedToken(K1T("procedure"), *mCurToken);
		++mCurToken;
		
		mCurNamespace = tok_procName();
		mCurNode = mCurNode->createChild(NodeType::PROC, mCurNamespace);

		tok_parameter();
		tok_declaration();

		while (!curHasVal(K1T("end")))
			tok_controlflow();

		mCurNode = mCurNode->getParent();
		++mCurToken;
	}
	string CTokenParseStage::tok_procName()
	{
		return (mCurToken++)->value;
	}


	/*
	======= PARAMETER =======
	*/
	void CTokenParseStage::tok_parameter()
	{
		if (!curHasVal(K1T("parameter")))
			throw CUnexpectedToken(K1T("parameter"), *mCurToken);
		++mCurToken;

		while (!curHasVal(K1T("end")))
			tok_pVar();

		++mCurToken;
	}
	void CTokenParseStage::tok_pVar()
	{
		string varname = (mCurToken++)->value;
		
		std::unordered_map<string,SymType> knownTypes = 
		{ 
			{ K1T("integer"), SymType::INTEGER },
			{ K1T("string"), SymType::STRING }
		};

		const auto &type = knownTypes.find(mCurToken->value);
		if (type == knownTypes.cend())
			throw CUnexpectedToken(K1T("(integer | string)"), *mCurToken);
		++mCurToken;


		std::unordered_map<string, SymAccess> knownModes =
		{
			{ K1T("in"), SymAccess::RD },
			{ K1T("out"), SymAccess::RW },
			{ K1T("inout"), SymAccess::RW }
		};

		const auto &access = knownModes.find(mCurToken->value);
		if (access == knownModes.cend())
			throw CUnexpectedToken(K1T("(in | out | inout)"), *mCurToken);
		++mCurToken;

		auto sym = CTokenParseOut::SSymbol(mCurNamespace, varname, type->second, access->second);
		mParseOut->mParams.insert({ varname, sym });
		mParseOut->mOrderedParams.push_back(sym);
	}


	/*
	======= DECLARATION =======
	*/
	void CTokenParseStage::tok_declaration()
	{
		if (!curHasVal(K1T("declaration")))
			throw CUnexpectedToken(K1T("declaration"), *mCurToken);
		++mCurToken;

		while (!curHasVal(K1T("end")))
			tok_var();

		++mCurToken;
	}
	void CTokenParseStage::tok_var()
	{
		string varname = (mCurToken++)->value;

		std::unordered_map<string, SymType> knownTypes =
		{
			{ K1T("integer"), SymType::INTEGER },
			{ K1T("string"), SymType::STRING }
		};

		const auto &type = knownTypes.find(mCurToken->value);
		if (type == knownTypes.cend())
			throw CUnexpectedToken(K1T("(integer | string)"), *mCurToken);
		++mCurToken;

		auto sym = CTokenParseOut::SSymbol(mCurNamespace, varname, type->second, SymAccess::RW);
		mParseOut->mDeclarations.insert({ varname, sym });
		mParseOut->mOrderedDeclarations.push_back(sym);
	}



	void CTokenParseStage::tok_controlflow()
	{
		if (curHasVal(K1T("exitloop")))
		{
			mCurNode->createChild(NodeType::EXITLOOP);
			++mCurToken;
		}
		else if (curHasVal(K1T("loop")))
			tok_loop();
		else if (curHasVal(K1T("case")))
			tok_case();
		else
			tok_statement();
	}

	void CTokenParseStage::tok_statement()
	{
		std::unordered_set<string> intops =
		{
			K1T("add"),
			K1T("sub")
		};
		std::unordered_set<string> strops =
		{
			K1T("concat")
		};

		if (curHasVal(K1T("print")))
			tok_printStmt();
		else if (curHasVal(K1T("input")))
			tok_inputStmt();
		else if (curHasVal(K1T("set")))
			tok_assignStmt();
		else if (intops.find(mCurToken->value) != intops.cend())
			tok_integerOp();
		else if (strops.find(mCurToken->value) != strops.cend())
			tok_stringOp();
		else if (curHasVal(K1T("call")))
			tok_callStmt();
		else
			throw CUnexpectedToken(K1T("(print | input | set | add | sub | concat | call)"), *mCurToken);
	}

	void CTokenParseStage::tok_printStmt()
	{
		if (!curHasVal(K1T("print")))
			throw CUnexpectedToken(K1T("print"), *mCurToken);
		++mCurToken;

		mCurNode = mCurNode->createChild(NodeType::PRINT);
		tok_operand();
		mCurNode = mCurNode->getParent();
	}

	void CTokenParseStage::tok_inputStmt()
	{
		if (!curHasVal(K1T("input")))
			throw CUnexpectedToken(K1T("input"), *mCurToken);
		++mCurToken;

		if (mCurToken->type != TokenType::CONSTSTR)
			throw CUnexpectedToken(K1T("string const"), *mCurToken);

		mCurNode = mCurNode->createChild(NodeType::INPUT);
		mCurNode->createChild(NodeType::CONSTSTR, mCurToken->value);
		++mCurToken;

		const string str = tok_resultVar();
		if (!mParseOut->isValidSymbol(str, SymAccess::RD))
			throw K1T("");
		mCurNode->createChild(NodeType::RESULTVAR, str);
		mCurNode = mCurNode->getParent();
	}

	void CTokenParseStage::tok_assignStmt()
	{
		if (!curHasVal(K1T("set")))
			throw CUnexpectedToken(K1T("set"), *mCurToken);
		++mCurToken;

		mCurNode = mCurNode->createChild(NodeType::ASSIGN);

		string str = tok_resultVar();
		if (!mParseOut->isValidSymbol(str, SymAccess::RD))
			throw K1T("");
		mCurNode->createChild(NodeType::RESULTVAR, str);
		
		tok_operand();

		mCurNode = mCurNode->getParent();
	}

	string CTokenParseStage::tok_localVar()
	{
		return (mCurToken++)->value;
	}

	void CTokenParseStage::tok_integerOp()
	{
		mCurNode = mCurNode->createChild(NodeType::INTOP);
		mCurNode->createChild(NodeType::OPERATOR, tok_integerOperator());

		string str = tok_resultVar();
		if (!mParseOut->isValidSymbol(str, SymAccess::RD))
			throw K1T("");

		mCurNode->createChild(NodeType::RESULTVAR, str);
		if (mParseOut->getSymbol(str).type != SymType::INTEGER)
			throw K1T("");

		for (int i = 2; i < 4; ++i)
		{
			tok_operand();
			const auto &n = mCurNode->getChild(i)->mData;
			if (n.type == NodeType::LOCALVAR)
			{
				if (mParseOut->getSymbol(n.value).type != SymType::INTEGER)
					throw K1T("");
			}
			else if (n.type != NodeType::CONSTINT)
				throw K1T("");
		}

		mCurNode = mCurNode->getParent();
	}

	string CTokenParseStage::tok_integerOperator()
	{
		std::unordered_set<string> ops =
		{
			K1T("add"),
			K1T("sub")
		};

		if (ops.find(mCurToken->value) == ops.cend())
			throw CUnexpectedToken(K1T("(add | sub)"), *mCurToken);

		return (mCurToken++)->value;
	}

	void CTokenParseStage::tok_stringOp()
	{
		mCurNode = mCurNode->createChild(NodeType::STROP);
		mCurNode->createChild(NodeType::OPERATOR, tok_stringOperator());

		string str = tok_resultVar();
		if (!mParseOut->isValidSymbol(str, SymAccess::RD))
			throw K1T("");
		mCurNode->createChild(NodeType::RESULTVAR, str);
		if (mParseOut->getSymbol(str).type != SymType::STRING)
			throw K1T("");

		for (int i = 2; i < 4; ++i)
		{
			tok_operand();
			const auto &n = mCurNode->getChild(i)->mData;
			if (n.type == NodeType::LOCALVAR)
			{
				if (mParseOut->getSymbol(n.value).type != SymType::STRING)
					throw K1T("");
			}
			else if (n.type != NodeType::CONSTSTR)
				throw K1T("");
		}
		mCurNode = mCurNode->getParent();
	}

	string CTokenParseStage::tok_stringOperator()
	{
		std::unordered_set<string> ops =
		{
			K1T("concat")
		};

		if (ops.find(mCurToken->value) == ops.cend())
			throw CUnexpectedToken(K1T("(concat)"), *mCurToken);

		return (mCurToken++)->value;
	}

	string CTokenParseStage::tok_resultVar()
	{
		return (mCurToken++)->value;
	}

	void CTokenParseStage::tok_callStmt()
	{
		if (!curHasVal(K1T("call")))
			throw CUnexpectedToken(K1T("call"), *mCurToken);
		++mCurToken;

		mCurNode = mCurNode->createChild(NodeType::CALL, tok_procName());

		if (mCurToken->type != TokenType::LBRA)
			throw CUnexpectedToken(K1T("LBRA"), *mCurToken);
		++mCurToken;

		while (mCurToken->type != TokenType::RBRA)
		{
			mCurNode->createChild(NodeType::PARAM, tok_localVar());
		}
		mCurNode = mCurNode->getParent();
		++mCurToken;
	}

	void CTokenParseStage::tok_loop()
	{
		if (!curHasVal(K1T("loop")))
			throw CUnexpectedToken(K1T("loop"), *mCurToken);
		++mCurToken;
		
		mCurNode = mCurNode->createChild(NodeType::LOOP);
		
		while (!curHasVal(K1T("end")))
			tok_controlflow();

		mCurNode = mCurNode->getParent();
		++mCurToken;
	}

	void CTokenParseStage::tok_case()
	{
		if (!curHasVal(K1T("case")))
			throw CUnexpectedToken(K1T("case"), *mCurToken);
		++mCurToken;

		mCurNode = mCurNode->createChild(NodeType::CASE);

		do
		{
			tok_when();
		} while (!curHasVal(K1T("otherwise")) && !curHasVal(K1T("end")));
		
		if (curHasVal(K1T("otherwise")))
			tok_otherwise();

		if (!curHasVal(K1T("end")))
			throw CUnexpectedToken(K1T("end"), *mCurToken);

		mCurNode = mCurNode->getParent();
		++mCurToken;
	}
	
	void CTokenParseStage::tok_when()
	{
		if (!curHasVal(K1T("when")))
			throw CUnexpectedToken(K1T("when"), *mCurToken);
		++mCurToken;

		mCurNode = mCurNode->createChild(NodeType::WHEN);

		tok_expression();

		while (!curHasVal(K1T("otherwise")) && !curHasVal(K1T("end")))
			tok_controlflow();

		mCurNode = mCurNode->getParent();
	}

	void CTokenParseStage::tok_otherwise()
	{
		if (!curHasVal(K1T("otherwise")))
			throw CUnexpectedToken(K1T("otherwise"), *mCurToken);
		++mCurToken;

		mCurNode = mCurNode->createChild(NodeType::OTHERWISE);
		while (!curHasVal(K1T("end")))
			tok_controlflow();
		mCurNode = mCurNode->getParent();
	}

	void CTokenParseStage::tok_expression()
	{
		mCurNode = mCurNode->createChild(NodeType::EXPRESSION);
		mCurNode->createChild(NodeType::OPERATOR, tok_logOperator());
		
		tok_operand();
		tok_operand();
		//TODO: op1.type == op2.type

		mCurNode = mCurNode->getParent();
	}

	string CTokenParseStage::tok_logOperator()
	{
		std::unordered_set<string> ops =
		{
			K1T("less"),
			K1T("equal"),
			K1T("greater")
		};

		if (ops.find(mCurToken->value) == ops.cend())
			throw CUnexpectedToken(K1T("(less | equal | greater)"), *mCurToken);

		return (mCurToken++)->value;
	}

	void CTokenParseStage::tok_operand()
	{
		if (mCurToken->type == TokenType::IDENTIFIER)
		{
			if (!mParseOut->isValidSymbol(mCurToken->value, SymAccess::WR))
				throw K1T("");
			mCurNode->createChild(NodeType::LOCALVAR, mCurToken->value);
		}
		else if (mCurToken->type == TokenType::CONSTSTR)
			mCurNode->createChild(NodeType::CONSTSTR, mCurToken->value);
		else if (mCurToken->type == TokenType::CONSTINT)
			mCurNode->createChild(NodeType::CONSTINT, mCurToken->value);
		else
			throw K1T("");

		++mCurToken;
	}
}