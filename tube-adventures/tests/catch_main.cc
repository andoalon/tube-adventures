#define CATCH_CONFIG_RUNNER

#include <catch2/catch.hpp>

int main(int argc, const char * argv[])
{
	Catch::Session session;

	bool wait_for_keypress_on_failure = false;
	for (int i = 1; i < argc; ++i)
	{
		if (std::strcmp("--wait-for-keypress-on-failure", argv[i]) != 0)
			continue;

		wait_for_keypress_on_failure = true;

		for (int j = i; j < argc; ++j)
			argv[j] = argv[j + 1];

		--argc;
		break;
	}

	const auto retval = session.run(argc, argv);

	if (wait_for_keypress_on_failure && retval != 0)
	{
		std::getchar();
	}

	return retval;
}


