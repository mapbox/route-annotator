#include <chrono>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <string>
#include <utility>

#include "annotator.hpp"
#include "database.hpp"
#include "extractor.hpp"

#include <boost/timer/timer.hpp>

/**
 * Simple program to show how to initalize the annotator
 * from a C++ utility.
 */
int main(int argc, char *argv[])
{

    if (argc == 0)
    {
        std::cerr << "Usage: " << argv[0] << " filename.[osm|pbf]" << std::endl;
        return EXIT_FAILURE;
    }

    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();

    Database db(false);
    Extractor extractor({std::string(argv[1])}, db);
    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();

    std::cout << "Done in "
              << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms"
              << std::endl;
}
