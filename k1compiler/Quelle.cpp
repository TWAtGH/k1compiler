#include <iostream>

#include "CCodeExecutionStage.hpp"
#include "CExceptions.hpp"


int main()
{
	std::vector<k1::IPipelineStage*> tasks;

	k1::IPipelineStage *stage = nullptr;

	stage = new k1::CWordScanStage(K1T("C:\\Users\\Tobias\\Desktop\\abc.k1"), stage);
	tasks.push_back(stage);

	stage = new k1::CTokenParseStage(stage);
	tasks.push_back(stage);

	stage = new k1::CCodeGenStage(stage);
	tasks.push_back(stage);

	stage = new k1::CCodeExecutionStage(stage);
	tasks.push_back(stage);

	for (auto task : tasks)
	{
		try
		{
			task->execute();
		}
		catch (k1::string excp)
		{
			std::wcerr << K1T("Exception thrown:\n") << excp;
			return EXIT_FAILURE;
		}
		catch (const k1::CParseError &excp)
		{
			std::wcerr << K1T("Exception thrown:\n") << excp.toString();
			return EXIT_FAILURE;
		}
		catch (...)
		{
			std::wcerr << K1T("Unexpected exception thrown!\n");
			return EXIT_FAILURE;
		}
	}

	while (!tasks.empty())
	{
		delete tasks.back();
		tasks.pop_back();
	}

	return 0;
}