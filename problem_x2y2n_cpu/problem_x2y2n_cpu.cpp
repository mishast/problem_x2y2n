
#include "thirdparty\util.hpp"
#include "thirdparty\nlohmann\json.hpp"
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <cmath>

using result_pair = std::pair<unsigned long, unsigned long>;

void pauseAtExit()
{
	std::cout << "Press enter to exit..." << std::endl;
	getchar();
}

int main()
{
	unsigned long long x = 60001;
	unsigned long long y = 60002;

	std::ifstream ifs("config.json");

	if (ifs.fail())
	{
		std::cout << "No config.json found. Use default values: x = " << x << " y = " << y << std::endl;
	}
	else
	{
		try
		{
			nlohmann::json json(ifs);

			x = json.at("x").get<int>();
			y = json.at("y").get<int>();
		}
		catch (std::exception ex)
		{
			std::cerr << "config.json parse error" << std::endl;
			std::cerr << "EXCEPTION: " << ex.what() << std::endl;

			pauseAtExit();

			return 0;
		}
	}

	std::cout << "Use values: x = " << x << " y = " << y << std::endl;

	unsigned long long N = x*x + y*y;
	unsigned long max_xy = sqrt(N) + 1;

	std::vector<result_pair> results;

	unsigned long long N1;

	util::Timer timer;
	timer.reset();

	// Перебор
	for (unsigned long long x1 = 0; x1 < max_xy; x1++)
	{
		for (unsigned long long y1 = 0; y1 < max_xy; y1++)
		{
			N1 = x1 * x1 + y1 * y1;

			if (N == N1)
			{
				results.push_back(result_pair(x1, y1));
			}
		}
	}

	uint64_t elapsed_microsec = timer.getTimeMicroseconds();
	std::cout << "Elapsed = " << elapsed_microsec / 1000 << " ms " << elapsed_microsec % 1000 << " microsec." << std::endl;

	// Вывод результатов
	std::cout << "Results count = " << results.size() << std::endl;
	for (decltype(results.size()) i = 0; i < results.size(); i++)
	{
		std::cout << "x = " << results[i].first << " y = " << results[i].second << std::endl;
	}

	pauseAtExit();

    return 0;
}

