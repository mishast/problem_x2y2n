
#define __CL_ENABLE_EXCEPTIONS

#include "thirdparty\khronos\cl.hpp"
#include "thirdparty\err_code.h"
#include "thirdparty\util.hpp"
#include "thirdparty\nlohmann\json.hpp"

#include <vector>
#include <cstdio>
#include <cstdlib>
#include <string>

#include <iostream>
#include <fstream>
#include <limits.h>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <chrono>
#include <sstream>


//------------------------------------------------------------------------------

// Задача для устройства
struct TaskOnDevice
{
	// Количество ответов
	std::vector<cl_uint>		res_count = { 0 };

	// Ответ, пары чисел: x1 y1 x2 y2 ...
	std::vector<cl_ulong>		result;

	// Объекты OpenCL
	cl::Buffer					buff_res_count;
	cl::Buffer					buff_result;
	cl::Device					device;
	cl::Context					context;
	cl::CommandQueue			queue;
	cl::Program					program;
	
	// Флаг, свободно устройство или нет
	bool						free = true;

	// Событие на завершение работы устройства
	cl::Event					completed_event;

	// Количество запусков на устройстве
	int							usage_count = 0;

	// Функтор
	std::function<cl::Event(cl::EnqueueArgs&, cl::Buffer, cl::Buffer, cl_ulong, cl_ulong, cl_ulong, cl_ulong, cl_ulong)> kernel;

	TaskOnDevice()
	{
		result.resize(1024);
	}
};

// Описывает задачи выполняемых на устройствах
struct DevicesTasks
{
	// Объекты для получения первого освободившегося устройства

	// Mutex для condition variable
	std::mutex					mutex;
	// Сигнал о том, что одно из устройств освободилось
	std::condition_variable		free_device_cv;
	// Дополнительная переменная сигнализирующая, что одно из устройств освободилось
	bool						device_ready = false;

	std::vector<TaskOnDevice>	tasks_on_devices;
};

// Запрашивает список устройств
std::vector<cl::Device> getDevices()
{
	std::vector<cl::Device> ret_devices;

	cl_device_type type = CL_DEVICE_TYPE_ALL;

	cl_int error;

	std::vector<cl::Platform> platforms;
	error = cl::Platform::get(&platforms);
		
	if (error != CL_SUCCESS)
	{
		throw cl::Error(error, "GET_PLATFORMS");
	}

	for (unsigned int i = 0; i < platforms.size(); i++)
	{
		std::vector<cl::Device> devices;

		error = platforms[i].getDevices(type, &devices);

		ret_devices.insert(ret_devices.end(), devices.begin(), devices.end());
	}

	return ret_devices;
}

// Получить не занятое устройство
int getFreeDevice(DevicesTasks& tasks)
{
	// Сначала просто проходим по циклу, может есть свободные
	for (decltype(tasks.tasks_on_devices.size()) i = 0; i < tasks.tasks_on_devices.size(); i++)
	{
		if (tasks.tasks_on_devices[i].free)
		{
			return i;
		}
	}

	// Крутимся в цикле, пока какое-нибудь устройство не освободится
	while (true)
	{
		for (decltype(tasks.tasks_on_devices.size()) i = 0; i < tasks.tasks_on_devices.size(); i++)
		{
			if (!tasks.tasks_on_devices[i].free)
			{
				cl_uint status;
				tasks.tasks_on_devices[i].completed_event.getInfo(CL_EVENT_COMMAND_EXECUTION_STATUS, &status);

				if (status == CL_COMPLETE)
				{
					tasks.tasks_on_devices[i].free = true;
				}
			}
		}

		// Возвращаем свободное устройство
		for (decltype(tasks.tasks_on_devices.size()) i = 0; i < tasks.tasks_on_devices.size(); i++)
		{
			if (tasks.tasks_on_devices[i].free)
			{
				return i;
			}
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(3));
	}

	return 0;
}

void pauseAtExit()
{
	std::cout << "Press enter to exit..." << std::endl;
	getchar();
}

int main(void)
{
	util::Timer timer;
	DevicesTasks tasks;

	cl_ulong x = 60001;
	cl_ulong y = 60002;

	try
	{
		// Запрашиваем список устройств
		std::vector<cl::Device> devices = getDevices();

		if (devices.size() == 0)
		{
			std::cout << "No devices found" << std::endl;
			
			pauseAtExit();
		}

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

		// Выводим на экран
		std::cout << "Available devices: " << std::endl;
		for (unsigned int i = 0; i < devices.size(); i++)
		{
			cl::STRING_CLASS name;
			devices[i].getInfo(CL_DEVICE_NAME, &name);

			std::cout << "Id = " << devices[i]() << " Device = " << name << std::endl;
		}

		// Загружаем программу
		std::string program_source = util::loadProgram("x2y2n.cl");

		// Для каждого устройства создаем свою задачу: контекст устройства, очередь,
		// задаем данные, задаем программу, создаем задачу
		for (decltype(devices.size()) i = 0; i < devices.size(); i++)
		{
			TaskOnDevice task;

			task.device = devices[i];

			task.context = cl::Context(task.device);

			task.program = cl::Program(task.context, program_source);
			
			cl_int err;

			try
			{
				err = task.program.build();
			}
			catch (cl::Error)
			{
				std::cout << "Build Status: " << task.program.getBuildInfo<CL_PROGRAM_BUILD_STATUS>(task.device) << std::endl;
				std::cout << "Build Options:\t" << task.program.getBuildInfo<CL_PROGRAM_BUILD_OPTIONS>(task.device) << std::endl;
				std::cout << "Build Log:\t " << task.program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(task.device) << std::endl;

				throw;
			}

			task.free = true;

			task.queue = cl::CommandQueue(task.context);
			
			task.buff_res_count = cl::Buffer(task.context, task.res_count.begin(), task.res_count.end(), false);

			task.buff_result = cl::Buffer(task.context, task.result.begin(), task.result.end(), false);
			
			task.kernel = cl::make_kernel<cl::Buffer, cl::Buffer, cl_ulong, cl_ulong, cl_ulong, cl_ulong, cl_ulong>(task.program, "x2y2n");

			tasks.tasks_on_devices.push_back(task);
		}

		// Одно ядро сканирует область размером kernel_step_width х kernel_step_width
		// На одном устройстве за раз выполняется grid_width x grid_width ядер
		// Таким образом одно устройство за раз сканирует область размером
		// (kernel_step_width*grid_width) х (kernel_step_width*grid_width)
		// При старте ядер передаем начальную точку сканирования start_x, start_y

		timer.reset();
		std::cout << "Start tasks" << std::endl;

		cl_ulong N = x * x + y * y;
		cl_ulong max_xy = (int)sqrt(N) + 1;

		cl_ulong start_x = 0;
		cl_ulong start_y = 0;

		cl_ulong grid_width = 128;
		cl_ulong kernel_step_width = 128;
		cl_ulong device_step_width = kernel_step_width * grid_width - 1;

		int currentDevice = 0;

		// Запуск ядер
		for (cl_ulong start_x = 0; start_x <= max_xy; start_x += device_step_width)
		{
			for (cl_ulong start_y = 0; start_y <= max_xy; start_y += device_step_width)
			{
				currentDevice = getFreeDevice(tasks);
				
				tasks.tasks_on_devices[currentDevice].free = false;
				tasks.tasks_on_devices[currentDevice].usage_count++;
								
				cl::Event event =
					tasks.tasks_on_devices[currentDevice].kernel(
						cl::EnqueueArgs(
							tasks.tasks_on_devices[currentDevice].queue,
							cl::NDRange(grid_width, grid_width)),
						tasks.tasks_on_devices[currentDevice].buff_res_count,
						tasks.tasks_on_devices[currentDevice].buff_result,
						N,
						start_x,
						start_y,
						kernel_step_width,
						max_xy
					);

				tasks.tasks_on_devices[currentDevice].queue.flush();

				tasks.tasks_on_devices[currentDevice].completed_event = event;
			}
		}

		// Ждем окончания работы всех устройств
		for (decltype(tasks.tasks_on_devices.size()) i = 0; i < tasks.tasks_on_devices.size(); i++)
		{
			tasks.tasks_on_devices[i].queue.finish();
		}

		uint64_t elapsed_microsec = timer.getTimeMicroseconds();
		std::cout << "Elapsed = " << elapsed_microsec / 1000 << " ms " << elapsed_microsec % 1000 << " microsec." << std::endl;

		// Получение результата
		
		for (unsigned int i = 0; i < tasks.tasks_on_devices.size(); i++)
		{
			cl::copy(tasks.tasks_on_devices[i].queue, tasks.tasks_on_devices[i].buff_res_count, tasks.tasks_on_devices[i].res_count.begin(), tasks.tasks_on_devices[i].res_count.end());
			cl::copy(tasks.tasks_on_devices[i].queue, tasks.tasks_on_devices[i].buff_result, tasks.tasks_on_devices[i].result.begin(), tasks.tasks_on_devices[i].result.end());
		}

		// Вывод результата

		bool found = false;

		for (unsigned int ti = 0; ti < tasks.tasks_on_devices.size(); ti++)
		{
			int pos = 0;
			int result_count = tasks.tasks_on_devices[ti].res_count[0];

			std::cout << "device = " << ti << " result count = " << result_count << " usage count = " << tasks.tasks_on_devices[ti].usage_count << std::endl;

			for (int ri = 0; ri < result_count; ri++)
			{
				found = true;
				int pos = (ri << 1);
				std::cout << "x = " << tasks.tasks_on_devices[ti].result[pos] << " y = " << tasks.tasks_on_devices[ti].result[pos + 1] << std::endl;
			}
		}

		if (!found)
		{
			std::cout << "x and y not found" << std::endl;
		}
	}
	catch (cl::Error err) {
		std::cout << "Exception\n";
		std::cerr
			<< "ERROR: "
			<< err.what()
			<< "("
			<< err_code(err.err())
			<< ")"
			<< std::endl;
	}

	pauseAtExit();
}