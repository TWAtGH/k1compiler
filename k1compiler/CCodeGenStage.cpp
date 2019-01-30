#include "CCodeGenStage.hpp"
#ifdef _DEBUG
#include <iostream>
#include <iomanip>
#endif
namespace k1
{
#ifdef _DEBUG
	static inline string CTToStr(AddressType t, unsigned int &skip)
	{
		typedef CCodeGenOut::E_CMD ct;
		skip = 0;
		switch (t)
		{
		case static_cast<AddressType>(ct::NOP) :
			return K1T("NOP");

		case static_cast<AddressType>(ct::PUSH) :
			skip = 1;
			return K1T("PUSH");

		case static_cast<AddressType>(ct::LOAD) :
			skip = 1;
			return K1T("LOAD");

		case static_cast<AddressType>(ct::POP) :
			return K1T("POP");

		case static_cast<AddressType>(ct::MOVI) :
			return K1T("MOVI");

		case static_cast<AddressType>(ct::MOVS) :
			return K1T("MOVS");

		case static_cast<AddressType>(ct::ADD) :
			return K1T("ADD");

		case static_cast<AddressType>(ct::SUB) :
			return K1T("SUB");

		case static_cast<AddressType>(ct::LESS) :
			return K1T("LESS");

		case static_cast<AddressType>(ct::EQU) :
			return K1T("EQU");

		case static_cast<AddressType>(ct::GRT) :
			return K1T("GRT");

		case static_cast<AddressType>(ct::JMP) :
			skip = 1;
			return K1T("JMP");

		case static_cast<AddressType>(ct::JMZ) :
			skip = 1;
			return K1T("JMZ");

		case static_cast<AddressType>(ct::CALL) :
			skip = 2;
			return K1T("CALL");

		case static_cast<AddressType>(ct::RET) :
			return K1T("RET");

		case static_cast<AddressType>(ct::PRINTI) :
			return K1T("PRINTI");

		case static_cast<AddressType>(ct::PRINTS) :
			return K1T("PRINTS");

		case static_cast<AddressType>(ct::SCANI) :
			return K1T("SCANI");

		case static_cast<AddressType>(ct::SCANS) :
			return K1T("SCANS");

		case static_cast<AddressType>(ct::STOP) :
			return K1T("STOP");
		};
		return K1T("");
	}
	void CCodeGenOut::dump() const
	{
		AddressType i = 0;
		for (auto cmd = mCode.cbegin(); cmd != mCode.cend(); ++cmd)
		{
			std::wcout << std::setfill(K1T(' '));
			std::wcout << std::setw(5);
			std::wcout << (i++) << K1T(": ");
			unsigned int skip=0;
			string s = CTToStr(*cmd,skip);
			if (s.empty())
				std::wcout << *cmd << std::endl;
			else
				std::wcout << s << std::endl;
			while (skip--)
			{
				std::wcout << std::setfill(K1T(' '));
				std::wcout << std::setw(5);
				std::wcout << (i++) << K1T(": ");
				std::wcout << *(++cmd) << std::endl;
			}
		}
	}
#endif
	CCodeGenOut::CCodeGenOut()
	{}
	CCodeGenOut::~CCodeGenOut()
	{}

	std::uint8_t CCodeGenOut::getType() const
	{
		return static_cast<std::uint8_t>(E_PIPELINE_TYPES::CODEGEN);
	}


	CCodeGenStage::CCodeGenStage(IPipelineStage *prev)
		:	IPipelineStage(prev),
			mCodeOut(nullptr),
			mCurLoop(0)
	{}

	CCodeGenStage::~CCodeGenStage()
	{
		if (mOutput)
		{
			delete mOutput;
			mOutput = nullptr;
		}
		mCodeOut = nullptr;
		mInput = nullptr;
	}

	void CCodeGenStage::pushCmd(ECmdType cmd)
	{
		mCodeOut->mCode.push_back(static_cast<AddressType>(cmd));
	}

	void CCodeGenStage::pushCmd(AddressType op)
	{
		mCodeOut->mCode.push_back(op);
	}

	void CCodeGenStage::execute()
	{
		if (!mPreviousStage->getData())
			throw K1T("No input data given!");

		mInput = dynamic_cast<CTokenParseOut*>(mPreviousStage->getData());
		if (!mInput)
			throw K1T("Unknown input data format!");

		mCodeOut = new CCodeGenOut;
		mOutput = mCodeOut;

		pushCmd(ECmdType::CALL);
		pushCmd(0);
		pushCmd(0);
		pushCmd(ECmdType::STOP);
		for (const auto &prog : mInput->mRoot->getChilds())
		{
			if (prog->mData.type != ENodeType::PROG)
				continue;

			for (const auto &proc : prog->getChilds())
			{
				if (proc->mData.type == ENodeType::MAIN)
				{
					const auto &p = mProcs.find(proc->mData.value);
					if (p == mProcs.cend())
						throw K1T("");//unresolved symbol main [ proc->mData.value ]
					mCodeOut->mCode[2] = p->second;
				}
				else if (proc->mData.type == ENodeType::PROC)
				{
					mProcs[proc->mData.value] = static_cast<AddressType>(mCodeOut->mCode.size());
					gen_procedure(proc);//TODO: call with NodeType* (without const) would be legal
				}
				else
					throw K1T("");//unexpected node

			}
		}

		//resolve unresolved procs
		for (auto proc : mUnresolvedProcs)
		{
			const auto &p = mProcs.find(proc.second);
			if (p == mProcs.cend())
				throw K1T("Unresolved external symbol!");
			mCodeOut->mCode[proc.first] = p->second;
		}

		pushCmd(ECmdType::STOP);

#ifdef _DEBUG
		std::wcout << K1T("\n --- CODE GENERATED: ---\n");
		mCodeOut->dump();
#endif
	}

	void CCodeGenStage::gen_procedure(const NodeType *parent)
	{
		typedef CTokenParseOut::E_SYMBOL_TYPE SymType;

		mLocalSymbols.clear();
		mLocalSpace = 0;
		for (const auto &symbol : mInput->mOrderedParams)
		{
			if (parent->mData.value == symbol.proc)
			{
				mLocalSymbols[symbol.name] = (mLocalSpace++);
				if (symbol.type == SymType::STRING)
					++mLocalSpace;
			}
		}

		for (const auto &symbol : mInput->mOrderedDeclarations)
		{
			if (parent->mData.value == symbol.proc)
			{
				pushCmd(ECmdType::PUSH);
				pushCmd(0);
				mLocalSymbols[symbol.name] = (mLocalSpace++);
				if (symbol.type == SymType::STRING)
				{
					pushCmd(ECmdType::PUSH);
					pushCmd(0);
					++mLocalSpace;
				}
			}
		}


		for (auto node : parent->getChilds())
		{
			controlflow(node);
		}


		for (auto i : mInput->mDeclarations)
			if (parent->mData.value == i.second.proc)
				pushCmd(ECmdType::POP);
		
		pushCmd(ECmdType::RET);
	}
	void CCodeGenStage::controlflow(const NodeType *node)
	{
		if (node->mData.type == ENodeType::EXITLOOP)
		{
			pushCmd(ECmdType::JMP);
			pushCmd(0);
			mLoopExits[mCurLoop].push_back(mCodeOut->mCode.size() - 1);
		}
		else if (node->mData.type == ENodeType::LOOP)
		{
			gen_loop(node);
		}
		else if (node->mData.type == ENodeType::CASE)
		{
			gen_case(node);
		}
		else
			gen_stmt(node);
	}
	void CCodeGenStage::gen_stmt(const NodeType *node)
	{
		typedef CTokenParseOut::E_SYMBOL_TYPE SymType;

		if (node->mData.type == ENodeType::PRINT)
		{
			//child 0: operand
			const auto &operand = node->getChild(0)->mData;

			if (operand.type == ENodeType::LOCALVAR)
			{
				const auto &src = mInput->getSymbol(operand.value);
				pushCmd(ECmdType::PUSH);
				pushCmd(mLocalSymbols[src.name]);

				if (src.type == SymType::INTEGER)
					pushCmd(ECmdType::PRINTI);
				else if (src.type == SymType::STRING)
					pushCmd(ECmdType::PRINTS);

				pushCmd(ECmdType::POP);
			}
			else if (operand.type == ENodeType::CONSTINT)
			{
				pushCmd(ECmdType::PUSH);
				pushCmd(static_cast<AddressType>(std::stoi(operand.value)));

				pushCmd(ECmdType::PUSH);
				pushCmd(static_cast<AddressType>(mLocalSymbols.size()));

				pushCmd(ECmdType::PRINTI);

				pushCmd(ECmdType::POP);
				pushCmd(ECmdType::POP);
			}
			else if (operand.type == ENodeType::CONSTSTR)
			{
				//TODO
			}
			else
				throw K1T("");
		}
		else if (node->mData.type == ENodeType::INPUT)
		{
			//child 0: prompt (const string)
			//child 1: resultvar

			pushCmd(ECmdType::PRINTS);
			pushCmd(0);

			//resolve later to str pointer
			mUnresolvedStrings[mCodeOut->mCode.size() - 1] = mStrings.size();
			mStrings.push_back(node->getChild(0)->mData.value);

			const auto &resultVar = mInput->getSymbol(node->getChild(1)->mData.value);

			pushCmd(ECmdType::PUSH);
			pushCmd(mLocalSymbols[resultVar.name]);

			if (resultVar.type == SymType::STRING)
				pushCmd(ECmdType::SCANS);
			else if (resultVar.type == SymType::INTEGER)
				pushCmd(ECmdType::SCANI);
			else
				throw K1T("");

			pushCmd(ECmdType::POP);
		}
		else if (node->mData.type == ENodeType::ASSIGN)
		{
			//child 0: resultvar
			//child 1: operand

			const auto &resultVar = mInput->getSymbol(node->getChild(0)->mData.value);
			const auto &operand = node->getChild(1)->mData;

			if (operand.type == ENodeType::LOCALVAR)
			{
				const auto &symbol = mInput->getSymbol(operand.value);

				//dst address
				pushCmd(ECmdType::PUSH);
				pushCmd(mLocalSymbols[resultVar.name]);

				//src address
				pushCmd(ECmdType::PUSH);
				pushCmd(mLocalSymbols[symbol.name]);

				if (symbol.type == SymType::INTEGER)
					pushCmd(ECmdType::MOVI);
				else if (symbol.type == SymType::STRING)
					pushCmd(ECmdType::MOVS);

				pushCmd(ECmdType::POP); //operand
				pushCmd(ECmdType::POP); //resultvar
			}
			else if (operand.type == ENodeType::CONSTINT)
			{
				//value
				pushCmd(ECmdType::PUSH);
				pushCmd(static_cast<AddressType>(std::stoi(operand.value)));

				//dst address
				pushCmd(ECmdType::PUSH);
				pushCmd(mLocalSymbols[resultVar.name]);

				//src address
				pushCmd(ECmdType::PUSH);
				pushCmd(static_cast<AddressType>(mLocalSymbols.size()));

				pushCmd(ECmdType::MOVI);

				pushCmd(ECmdType::POP); //operand
				pushCmd(ECmdType::POP); //resultvar
				pushCmd(ECmdType::POP); //int const
			}
			else if (operand.type == ENodeType::CONSTSTR)
			{
				//TODO
			}
		}
		else if (node->mData.type == ENodeType::INTOP)
		{
			const auto &resVar = node->getChild(1)->mData;
			for (std::size_t i = 2; i < 4; ++i)
			{
				const auto &opi = node->getChild(i)->mData;
				if (opi.type == ENodeType::LOCALVAR)
				{
					pushCmd(ECmdType::LOAD);
					pushCmd(mLocalSymbols[opi.value]);
				}
				else
				{
					pushCmd(ECmdType::PUSH);
					pushCmd(static_cast<AddressType>(std::stoi(opi.value)));
				}
			}

			if (node->getChild(0)->mData.value == K1T("add"))
				pushCmd(ECmdType::ADD);
			else if (node->getChild(0)->mData.value == K1T("sub"))
				pushCmd(ECmdType::SUB);

			pushCmd(ECmdType::PUSH);
			pushCmd(mLocalSymbols[resVar.value]);
			pushCmd(ECmdType::PUSH);
			pushCmd(mLocalSymbols.size());

			pushCmd(ECmdType::MOVI);

			pushCmd(ECmdType::POP);
			pushCmd(ECmdType::POP);
			pushCmd(ECmdType::POP);
		}
		else if (node->mData.type == ENodeType::STROP)
		{
		}
		else if (node->mData.type == ENodeType::CALL)
		{
			unsigned int paramCount = 0;
			for (auto paramNode : node->getChilds())
			{
				const auto &paramSymbol = mInput->getSymbol(paramNode->mData.value);
				//TODO: check access here
				++paramCount;
				pushCmd(ECmdType::LOAD);
				pushCmd(mLocalSymbols[paramSymbol.name]);
				if (paramSymbol.type == SymType::STRING)
				{
					++paramCount;
					pushCmd(ECmdType::LOAD);
					pushCmd(mLocalSymbols[paramSymbol.name] + 1);
				}
			}
			pushCmd(ECmdType::CALL);
			pushCmd(mLocalSpace);
			pushCmd(0);
			mUnresolvedProcs[mCodeOut->mCode.size() - 1] = node->mData.value;
			
			auto pr = mInput->mOrderedParams.rbegin();
			for (auto i = node->getChilds().rbegin(); i != node->getChilds().rend(); ++i)
			{
				while (pr->proc != node->mData.value)
					++pr;
				const auto p = *i;

				string dbg1 = pr->name;
				string dbg2 = p->mData.value;
				//TODO: handle strings
				if (pr->access != CTokenParseOut::E_SYMBOL_ACCESS::RD)
				{
					pushCmd(ECmdType::PUSH);
					pushCmd(mLocalSymbols[p->mData.value]);
					pushCmd(ECmdType::PUSH);
					pushCmd(mLocalSymbols.size() + paramCount - 1);
					pushCmd(ECmdType::MOVI);
					pushCmd(ECmdType::POP);
					pushCmd(ECmdType::POP);
				}
				pushCmd(ECmdType::POP);
				++pr;
			}
		}
	}

	void CCodeGenStage::gen_loop(const NodeType *node)
	{
		AddressType begin = mCodeOut->mCode.size();
		++mCurLoop;

		for (auto node : node->getChilds())
			controlflow(node);

		pushCmd(ECmdType::JMP);
		pushCmd(begin);

		pushCmd(ECmdType::NOP);
		for (auto adr : mLoopExits[mCurLoop])
			mCodeOut->mCode[adr] = mCodeOut->mCode.size() - 1;

		--mCurLoop;
	}
	
	void CCodeGenStage::gen_case(const NodeType *parent)
	{
		std::vector<AddressType> unresolvedExits;
		AddressType exit = 0;
		
		AddressType resolver = 0;
		for (auto it = parent->getChilds().cbegin(); it != parent->getChilds().cend(); ++it)
		{
			auto node = (*it);
			if (node->mData.type == ENodeType::WHEN)
			{
				//TODO: only legal for ints atm
				node = node->getChild(0);//expression
				const string op = node->getChild(0)->mData.value;

				pushCmd(ECmdType::LOAD);
				pushCmd(mLocalSymbols[node->getChild(1)->mData.value]);
				
				pushCmd(ECmdType::LOAD);
				pushCmd(mLocalSymbols[node->getChild(2)->mData.value]);
				
				if (op == K1T("less"))
					pushCmd(ECmdType::LESS);
				else if (op == K1T("equal"))
					pushCmd(ECmdType::EQU);
				else if (op == K1T("greater"))
					pushCmd(ECmdType::GRT);

				node = (*it); //when

				//jmp to nop1 if condition is not met
				pushCmd(ECmdType::JMZ);
				pushCmd(0);
				resolver = mCodeOut->mCode.size() - 1;

				for (std::size_t i = 1; i < node->getChilds().size(); ++i)
					controlflow(node->getChild(i));
				
				pushCmd(ECmdType::JMP);
				mCodeOut->mCode[resolver] = mCodeOut->mCode.size() - 1;
				unresolvedExits.push_back(mCodeOut->mCode.size() - 1);
			}
			else
			{
				controlflow(node);
				pushCmd(ECmdType::NOP);
				exit = mCodeOut->mCode.size() - 1;
			}
		}

		for (auto e : unresolvedExits)
			mCodeOut->mCode[e] = exit;
	}
}