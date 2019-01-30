#pragma once

#include "CCodeGenStage.hpp"

namespace k1
{
	class CCodeExecutionOut : public IPipelineData
	{
	public:
	public:
		CCodeExecutionOut();
		~CCodeExecutionOut();

		virtual std::uint8_t getType() const;
	};

	class CCodeExecutionStage : public IPipelineStage
	{
	public:
		CCodeExecutionStage(IPipelineStage *prev);
		~CCodeExecutionStage();

		virtual void execute();
		static inline AddressType CmdToAdr(CCodeGenOut::E_CMD cmd);

	protected:

	private:
		CCodeGenOut *mInput;
	};
}