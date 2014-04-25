#include <cstdlib>
#include <ctime>

#include <iostream>
#include <exception>
#include <stdexcept>
#include <sstream>
#include <locale>

#include <X11/Xlib.h>


void print_time(std::ostream& out)
{
	auto const& time_put = std::use_facet<std::time_put<char>>(out.getloc());

	std::time_t const timestamp = std::time(nullptr);
	tm* now = std::localtime(&timestamp);

	out << u8"[";
	time_put.put(out, out, ' ', now, 'c');
	out << u8"]";
}


int main()
{
	Display* display = nullptr;

	try
	{
		display = ::XOpenDisplay(nullptr);
		if (!display)
			throw std::runtime_error(u8"Could not open display.");

		std::ostringstream out;
		std::locale loc("");
		out.imbue(loc);

		print_time(out);

		std::cout << out.str() << std::endl;
		::XStoreName(display, DefaultRootWindow(display), out.str().c_str());
		::XSync(display, False);
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

	if (display)
		::XCloseDisplay(display);

	return EXIT_SUCCESS;
}
