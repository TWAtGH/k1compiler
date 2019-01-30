#pragma once

#include <cstdint>

namespace k1
{
	class IPipelineData
	{
	public:
		virtual ~IPipelineData(){};
		virtual std::uint8_t getType() const = 0;
	};

	class IPipelineStage
	{
	public:
		IPipelineStage(IPipelineStage *prev)
			:	mPreviousStage(prev),
				mOutput(nullptr)
		{}
		virtual ~IPipelineStage(){};

		virtual void execute() = 0;

		IPipelineData *getData()
		{
			return mOutput;
		}

	protected:
		IPipelineStage *mPreviousStage;
		IPipelineData *mOutput;
	};
}