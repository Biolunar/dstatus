#include "config.h"

#include <locale.h>
#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

#include <X11/Xlib.h>
#include <sensors/sensors.h>
#include <sensors/error.h>
#include <unistd.h>


struct temperature
{
	size_t count;
	int* numbers;
	sensors_chip_name const* chip;
};


static void signal_handler(int sig);
static size_t print_time(char* buffer, size_t size);
static struct temperature* temperature_init(void);
static void temperature_cleanup(struct temperature* temp);
static double get_temperature(struct temperature* temp);


static sig_atomic_t volatile running = 1;


static void signal_handler(int sig)
{
	running = 0;
}

static size_t print_time(char* buffer, size_t size)
{
	time_t const raw = time(0);
	if (raw == -1)
	{
		fprintf(stderr, u8"Could not get the current calender time.\n");
		return 0;
	}

	struct tm const* const tm = localtime(&raw);
	if (!tm)
	{
		fprintf(stderr, u8"Could not get the local time.\n");
		return 0;
	}

	return strftime(buffer, size, u8" [%c]", tm);
}

static struct temperature* temperature_init(void)
{
	int chip_nr = 0;
	struct temperature* temp = 0;

	if (sensors_init(0))
	{
		fprintf(stderr, u8"Could not initialize libsensors.\n");
		return 0;
	}

	temp = calloc(1, sizeof(struct temperature));
	if (!temp)
	{
		fprintf(stderr, u8"Could not allocate memory for temperature struct.\n");
		return 0;
	}

	while ((temp->chip = sensors_get_detected_chips(0, &chip_nr)))
	{
		if (!strcmp(chip_name, temp->chip->prefix))
		{
			int feature_nr = 0;
			sensors_feature const* feat = 0;
			while ((feat = sensors_get_features(temp->chip, &feature_nr)))
			{
				if (feat->type == SENSORS_FEATURE_TEMP)
				{
					sensors_subfeature const* sub = sensors_get_subfeature(temp->chip, feat, SENSORS_SUBFEATURE_TEMP_INPUT);
					if (!sub)
					{
						fprintf(stderr, u8"Could not get chip subfeature.\n");
						return 0;
					}

					++temp->count;
					temp->numbers = realloc(temp->numbers, temp->count * sizeof(int));
					temp->numbers[temp->count - 1] = sub->number;
				}
			}

			return temp;
		}
	}

	return 0;
}

static void temperature_cleanup(struct temperature* temp)
{
	if (temp)
	{
		if (temp->count)
			free(temp->numbers);
		free(temp);
	}
	sensors_cleanup();
}

static double get_temperature(struct temperature* temp)
{
	assert(temp);

	double value = 0.0;

	for (size_t i = 0; i < temp->count; ++i)
	{
		double dummy = 0.0;
		int const ret = sensors_get_value(temp->chip, temp->numbers[i], &dummy);
		if (ret < 0)
		{
			fprintf(stderr, u8"Could not get temperature value from subfeature %d.\n", temp->numbers[i]);
		}

		value = value >= dummy ? value : dummy;
	}

	return value;
}

static size_t print_temperature(char* buffer, size_t size, struct temperature* temp)
{
	int const ret = snprintf(buffer, size, u8"[%.0f°C]", get_temperature(temp));
	return ret < 0 ? 0 : (size_t)ret;
}

/*void print_battery(std::ostream& out)
{
	std::ifstream file(std::string{battery_path} + u8"present");
	file.exceptions(std::ios_base::badbit);

	bool present = false;
	file >> present;
	if (!present)
	{
		out << u8"[N/A]";
		return;
	}
	file.close();

	file.open(std::string{battery_path} + u8"charge_full_design");
	unsigned long cfd = 0;
	file >> cfd;
	file.close();

	file.open(std::string{battery_path} + u8"charge_now");
	unsigned long cn = 0;
	file >> cn;

	out << u8"[" << std::round((static_cast<double>(cn) / cfd) * 100) << u8"%]";
}*/

int main()
{
	Display* display = 0;
	struct temperature* temperature = 0;
	int ret = EXIT_FAILURE;

	signal(SIGINT, &signal_handler);
	signal(SIGTERM, &signal_handler);
	setlocale(LC_ALL, u8"");

	display = XOpenDisplay(0);
	if (!display)
	{
		fprintf(stderr, u8"Could not open X11 display.\n");
		goto cleanup;
	}

	temperature = temperature_init();
	if (!temperature)
	{
		fprintf(stderr, u8"Could not initialize temperature.\n");
		goto cleanup;
	}

	while (running)
	{
		char buffer[1024];
		char* cur = buffer;
		size_t size = sizeof buffer;
		size_t check = 0;

		check = print_temperature(cur, size, temperature);
		size -= check;
		cur += check;

		check = print_time(cur, size);
		size -= check;
		cur += check;

		XStoreName(display, DefaultRootWindow(display), buffer);
		XSync(display, False);
		sleep(1);
	}

	ret = EXIT_SUCCESS;

cleanup:
	temperature_cleanup(temperature);
	if (display)
		XCloseDisplay(display);

	return ret;
}
