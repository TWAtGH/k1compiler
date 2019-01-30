#include "CCodeExecutionStage.hpp"
#include <iostream>
#include <stack>

namespace k1
{

	std::uint8_t CCodeExecutionOut::getType() const
	{
		return static_cast<std::uint8_t>(E_PIPELINE_TYPES::CODEEXECUTION);
	}

	CCodeExecutionOut::CCodeExecutionOut()
	{}

	CCodeExecutionOut::~CCodeExecutionOut()
	{}

	CCodeExecutionStage::CCodeExecutionStage(IPipelineStage *prev)
		:IPipelineStage(prev)
	{}

	CCodeExecutionStage::~CCodeExecutionStage()
	{}

	AddressType CCodeExecutionStage::CmdToAdr(CCodeGenOut::E_CMD cmd)
	{
		return static_cast<AddressType>(cmd);
	}

	void CCodeExecutionStage::execute()
	{
		//return;
		if (!mPreviousStage->getData())
			throw K1T("No input data given!");

		mInput = dynamic_cast<CCodeGenOut*>(mPreviousStage->getData());
		if (!mInput)
			throw K1T("Unknown input data format!");

		typedef CCodeGenOut::E_CMD ECMD;

		struct SCallInfo
		{
			AddressType offset;
			AddressType origin;
		};

		const auto &code = mInput->mCode;
		std::vector<AddressType> stack;

		auto pc = code.size();
		std::stack<SCallInfo> offsets;
		offsets.push({0,0});

		for (pc = 0; pc < code.size();)
		{
			const auto &proc = offsets.top();
			if (code[pc] == CmdToAdr(ECMD::PUSH))
			{
				stack.push_back(code[++pc]);
			}
			else if (code[pc] == CmdToAdr(ECMD::LOAD))
			{
				stack.push_back(stack[proc.offset + code[++pc]]);
			}
			else if (code[pc] == CmdToAdr(ECMD::POP))
			{
				stack.pop_back();
			}
			else if (code[pc] == CmdToAdr(ECMD::MOVI))
			{
				const auto src = stack.back();
				stack.pop_back();
				const auto dst = stack.back();
				stack.push_back(src);
				stack[proc.offset + dst] = stack[proc.offset + src];
			}
			else if (code[pc] == CmdToAdr(ECMD::ADD))
			{
				int left = static_cast<int>(stack.back());
				stack.pop_back();
				int right = static_cast<int>(stack.back());
				stack.pop_back();
				stack.push_back(left + right);
			}
			else if (code[pc] == CmdToAdr(ECMD::SUB))
			{
				int left = static_cast<int>(stack.back());
				stack.pop_back();
				int right = static_cast<int>(stack.back());
				stack.pop_back();
				stack.push_back(static_cast<AddressType>(left - right));
			}
			else if (code[pc] == CmdToAdr(ECMD::LESS))
			{
				int left = static_cast<int>(stack.back());
				stack.pop_back();
				int right = static_cast<int>(stack.back());
				stack.pop_back();
				left = (left < right) ? 1 : 0;
				stack.push_back(left);
			}
			else if (code[pc] == CmdToAdr(ECMD::EQU))
			{
				int left = static_cast<int>(stack.back());
				stack.pop_back();
				int right = static_cast<int>(stack.back());
				stack.pop_back();
				left = (left == right) ? 1 : 0;
				stack.push_back(left);
			}
			else if (code[pc] == CmdToAdr(ECMD::GRT))
			{
				int left = static_cast<int>(stack.back());
				stack.pop_back();
				int right = static_cast<int>(stack.back());
				stack.pop_back();
				left = (left > right) ? 1 : 0;
				stack.push_back(left);
			}
			else if (code[pc] == CmdToAdr(ECMD::JMP))
			{
				pc = static_cast<decltype(pc)>(code[pc + 1]);
				continue;
			}
			else if (code[pc] == CmdToAdr(ECMD::JMZ))
			{
				const auto val = stack.back();
				stack.pop_back();

				++pc;
				if (val == 0)
				{
					pc = static_cast<decltype(pc)>(code[pc]);
					continue;
				}
			}
			else if (code[pc] == CmdToAdr(ECMD::CALL))
			{
				SCallInfo call;
				call.offset = code[++pc] + proc.offset;
				call.origin = ++pc;
				pc = code[pc];
				offsets.push(call);
				continue;
			}
			else if (code[pc] == CmdToAdr(ECMD::RET))
			{
				pc = proc.origin;
				offsets.pop();
			}
			else if (code[pc] == CmdToAdr(ECMD::PRINTI))
			{
				const auto src = stack.back();
				std::wcout << stack[proc.offset + src] << std::endl;
			}
			else if (code[pc] == CmdToAdr(ECMD::PRINTS))
			{
			}
			else if (code[pc] == CmdToAdr(ECMD::SCANI))
			{
				//TODO: negative values....
				int val;
				std::wcin >> val;
				const auto dst = stack.back();
				stack[proc.offset + dst] = val;
			}
			else if (code[pc] == CmdToAdr(ECMD::SCANS))
			{
			}
			else if (code[pc] == CmdToAdr(ECMD::STOP))
			{
				break;
			}
			++pc;
		}
	}

}