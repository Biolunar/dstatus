#include "config.hpp"

#include <cstdlib>
#include <ctime>
#include <cstdio>
#include <cmath>
#include <csignal>

#include <iostream>
#include <exception>
#include <stdexcept>
#include <sstream>
#include <locale>
#include <vector>
#include <algorithm>
#include <fstream>

#include <X11/Xlib.h>
#include <sensors/sensors.h>
#include <sensors/error.h>
#include <unistd.h>


static bool running = true;


void print_time(std::ostream& out)
{
	auto const& time_put = std::use_facet<std::time_put<char>>(out.getloc());

	std::time_t const timestamp = std::time(nullptr);
	tm* now = std::localtime(&timestamp);

	out << u8"[";
	time_put.put(out, out, ' ', now, 'c');
	out << u8"]";
}


static std::vector<int> sub_numbers;
static sensors_chip_name const* chip = nullptr;


void enum_chips()
{
	int chip_nr = 0;
	while ((chip = sensors_get_detected_chips(nullptr, &chip_nr)))
	{
		if (chip_name == chip->prefix)
		{
			int feature_nr = 0;
			sensors_feature const* feat = nullptr;
			while ((feat = sensors_get_features(chip, &feature_nr)))
			{
				if (feat->type == SENSORS_FEATURE_TEMP)
				{
					sensors_subfeature const* sub = sensors_get_subfeature(chip, feat, SENSORS_SUBFEATURE_TEMP_INPUT);
					if (!sub)
						throw std::runtime_error(u8"Could not get chip subfeature.");
					sub_numbers.push_back(sub->number);
				}
			}

			return;
		}
	}

	if (!chip)
		throw std::runtime_error(u8"Could not find chip.");
}


void print_temp(std::ostream& out)
{
	double temp = 0.0;
	for (auto const& i : sub_numbers)
	{
		double val = 0.0;
		auto ret = sensors_get_value(chip, i, &val);
		if (ret < 0)
		{
			std::ostringstream err;
			err << u8"Could not get sensor value (" << sensors_strerror(ret) << u8")";
			throw std::runtime_error(err.str());
		}

		temp = std::max(temp, val);
	}

	out << u8"[" << temp << u8"°C]";
}


void print_battery(std::ostream& out)
{
	std::ifstream file(battery_path + u8"present");
	file.exceptions(std::ios_base::badbit);

	bool present = false;
	file >> present;
	if (!present)
	{
		out << u8"[N/A]";
		return;
	}
	file.close();

	file.open(battery_path + u8"charge_full_design");
	unsigned long cfd = 0;
	file >> cfd;
	file.close();

	file.open(battery_path + u8"charge_now");
	unsigned long cn = 0;
	file >> cn;

	out << u8"[" << std::round((static_cast<double>(cn) / cfd) * 100) << u8"%]";
}


void handler(int)
{
	running = false;
}


int main()
{
	Display* display = nullptr;

	try
	{
		std::signal(SIGINT, &handler);
		std::signal(SIGTERM, &handler);

		display = ::XOpenDisplay(nullptr);
		if (!display)
			throw std::runtime_error(u8"Could not open display.");

		if (sensors_init(nullptr))
			throw std::runtime_error(u8"Could not initialize sensors.");

		//std::cout << u8"libsensors version: " << libsensors_version << std::endl;
		enum_chips();

		std::ostringstream out;
		std::locale loc("");
		while (running)
		{
			out.imbue(loc);

			print_battery(out);
			out << u8" ";
			print_temp(out);
			out << u8" ";
			print_time(out);

			//std::cout << out.str() << std::endl;
			::XStoreName(display, DefaultRootWindow(display), out.str().c_str());
			::XSync(display, False);
			out.str(u8"");

			sleep(1);
		}
	}
	catch (std::exception const& err)
	{
		std::cerr << u8"Exception: " << err.what() << std::endl;
		return EXIT_FAILURE;
	}
	catch (...)
	{
		std::cerr << u8"Unknown exception." << std::endl;
		return EXIT_FAILURE;
	}

	sensors_cleanup();
	if (display)
		::XCloseDisplay(display);

	return EXIT_SUCCESS;
}
