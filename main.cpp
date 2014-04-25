#include <cstdlib>
#include <ctime>

#include <iostream>
#include <exception>
#include <sstream>
#include <locale>


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
	try
	{
		std::ostringstream out;
		std::locale loc("");
		out.imbue(loc);

		print_time(out);

		std::cout << out.str() << std::endl;
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

	return EXIT_SUCCESS;
}
