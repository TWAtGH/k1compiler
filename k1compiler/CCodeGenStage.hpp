#pragma once

#include "CTokenParseStage.hpp"

namespace k1
{
	typedef std::uint32_t AddressType;

	class CCodeGenOut : public IPipelineData
	{
	public:
		enum class E_CMD : AddressType
		{
			NOP = 0,
			PUSH,	//push value from op1 (inc sp)
			LOAD,	//push value from *op1 (inc sp)
			POP,	//dec sp

			MOVI,	//*(sp-1) = *sp
			MOVS,	//

			ADD,	//pop last 2 elements and push result
			SUB,	//pop last 2 elements and push result

			LESS,	//pop last 2 elements and push result ((sp-1)<(sp))
			EQU,	//pop last 2 elements and push result ((sp-1)=(sp))
			GRT,	//pop last 2 elements and push result ((sp-1)>(sp))

			JMP,	//set pc = op1
			JMZ,	//if (*sp == 0) pc = op1; else pc+=2; !dec sp!

			CALL,
			RET,

			PRINTI,	//print (*sp) to stdout
			PRINTS,	//print chars from (*sp) to (*(sp+1)) stdout
			SCANI,	//read int and store at element at op1
			SCANS,	//read string and store at element at op1 and op1+1

			STOP,
		};

	public:
		CCodeGenOut();
		~CCodeGenOut();

		virtual std::uint8_t getType() const;
#ifdef _DEBUG
		void dump() const;
#endif
		std::vector<AddressType> mCode;
	};

	class CCodeGenStage : public IPipelineStage
	{
	public:
		CCodeGenStage(IPipelineStage *prev);
		~CCodeGenStage();

		virtual void execute();

	protected:
		typedef CTokenParseOut::CNode NodeType;
		typedef CTokenParseOut::CNode::E_NODE_TYPE ENodeType;
		typedef CCodeGenOut::E_CMD ECmdType;

		inline void pushCmd(ECmdType cmd);
		inline void pushCmd(AddressType op);

		void gen_procedure(const NodeType *node);
		void gen_stmt(const NodeType *node);
		void gen_loop(const NodeType *node);
		void gen_case(const NodeType *node);

		inline void controlflow(const NodeType *node);
	private:
		CCodeGenOut *mCodeOut;
		CTokenParseOut *mInput;

		std::unordered_map<string, AddressType> mLocalSymbols;
		AddressType mLocalSpace;

		std::unordered_map<string, AddressType> mProcs;
		std::unordered_map<AddressType, string> mUnresolvedProcs;

		std::vector<string> mStrings;
		std::unordered_map<AddressType, AddressType> mUnresolvedStrings;

		std::unordered_map<unsigned int, std::vector<AddressType>> mLoopExits;
		unsigned int mCurLoop;
	};
}