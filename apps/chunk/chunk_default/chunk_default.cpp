
#include "ChunkDefault/ChunkDefault.h"
#include "Utils.h"

#include <boost/program_options.hpp>

#include <chrono>
#include <iostream>
#include <syncstream>
#include <vector>

namespace po = boost::program_options;

int main(int argc, char const *argv[])
{
    po::options_description desc("Allowed options");
    po::variables_map vm;
    try
    {
        desc.add_options()
            ("help", "Show help")
            ("chunk_size", po::value<int>()->default_value(100), "Chunk size value (int)")
            ("overlap", po::value<int>()->default_value(20), "Overlap value (int)")
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

    int chunk_size = vm["chunk_size"].as<int>();
    int overlap = vm["overlap"].as<int>();
    std::string filenamepath = vm["filenamepath"].as<std::string>();

    RAGLibrary::LoaderDataStruct input(filenamepath, Utils::GetLines(filenamepath));

    auto initialTime = std::chrono::high_resolution_clock::now();

    auto chunk = Chunk::ChunkDefault(chunk_size, overlap);
    auto documents = chunk.ProcessSingleDocument(input);

    std::osyncstream(std::cout) << "ChunkDefault("
        <<"chunk_size='" << chunk_size
        << "',overlap="<< overlap
    << ")" << std::endl;
    std::osyncstream(std::cout) << "  from '"<< filenamepath << "': documents.size=" << documents.size() << std::endl;

    auto endTime = std::chrono::high_resolution_clock::now();
    auto durationTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - initialTime).count();

    std::osyncstream(std::cout) << "execution time: " << durationTime << "ms" << std::endl;

    return EXIT_SUCCESS;
}
