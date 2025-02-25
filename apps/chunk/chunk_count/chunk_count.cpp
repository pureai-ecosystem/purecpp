#include <chrono>
#include <iostream>
#include <syncstream>
#include <vector>

#include <boost/program_options.hpp>

#include "ChunkCount/ChunkCount.h"
#include "Utils.h"

namespace po = boost::program_options;

int main(int argc, char const *argv[])
{
    po::options_description desc("Allowed options");
    po::variables_map vm;
    try
    {
        desc.add_options()
            ("help", "Show help")
            ("overlap", po::value<int>()->default_value(600), "Overlap value (int)")
            ("count_unit", po::value<std::string>()->required(), "Count unit (string|regex)")
            ("count_threshold", po::value<int>()->default_value(1), "Count threshold value (int)")
            ("filenamepath", po::value<std::string>()->required(), "File path (string)")
            ;

        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);

        if (vm.count("help"))
        {
            std::cout << desc << std::endl;
        }
    }
    catch(const po::required_option& e)
    {
        std::cerr << e.what() << '\n';
        std::cout << desc << std::endl;
        return EXIT_FAILURE;
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        return EXIT_FAILURE;
    }

    int overlap = vm["overlap"].as<int>();
    std::string count_unit = vm["count_unit"].as<std::string>();
    int count_threshold = vm["count_threshold"].as<int>();
    std::string filenamepath = vm["filenamepath"].as<std::string>();

    RAGLibrary::LoaderDataStruct input(filenamepath, Utils::GetLines(filenamepath));

    auto initialTime = std::chrono::high_resolution_clock::now();

    auto chunk = Chunk::ChunkCount(count_unit, overlap, count_threshold);
    auto documents = chunk.ProcessSingleDocument(input);

    std::osyncstream(std::cout) << "ChunkCount("
        <<"count_unit='" << count_unit
        << "',overlap="<< overlap
        << ",count_threshold=" << count_threshold
    << ")" << std::endl;
    std::osyncstream(std::cout) << "  from '"<< filenamepath << "': documents.size=" << documents.size() << std::endl;

    auto endTime = std::chrono::high_resolution_clock::now();
    auto durationTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - initialTime).count();

    std::osyncstream(std::cout) << "execution time: " << durationTime << "ms" << std::endl;

    return EXIT_SUCCESS;
}
