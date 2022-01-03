#include <windows.h>
#include <string>
#include <iostream>
#include "posture_graph.h"
#include "filesystem_helper.hpp"


int main(int argc, char* argv[])
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);

	if (5 != argc)
	{
		std::cerr << "Usage:\textract_joint_rotation <PATH_INTERESTS_CONF> <PATH_SRC> <PATH_DST_ROT> <PATH_DST_LIMIT> \t\t//to remove theta noise of <PATH_SRC>, and save it to <PATH_DST>" << std::endl;
	}
	else
	{
		auto tick_start = ::GetTickCount64();
		bool removed = extract_joint_rotation(argv[1], argv[2], argv[3], argv[4]);
		auto tick = ::GetTickCount64() - tick_start;
		float tick_sec = tick / 1000.0f;
		const char* res[] = { "failed", "successful" };
		int i_res = (removed ? 1 : 0);
		printf("Removing noise from %s, and saving it to %s takes %.2f seconds: %s\n", argv[1], argv[2], tick_sec, res[i_res]);
	}
	return 0;
}
