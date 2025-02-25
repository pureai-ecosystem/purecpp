
#include "ChunkSimilarity/ChunkSimilarity.h"
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
    std::string embedding_model = "hf";
    try
    {
        desc.add_options()
            ("help", "Show help")
            ("chunk_size", po::value<int>()->default_value(100), "Chunk size value (int)")
            ("overlap", po::value<int>()->default_value(20), "Overlap value (int)")
            ("embedding_model", po::value<std::string>()->default_value("hf"), "Embedding model value (hf or openai)")
            ("filenamepath", po::value<std::string>()->required(), "File path (string)")
            ;

        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);

        if (vm.count("help"))
        {
            std::cout << desc << std::endl;
            return EXIT_FAILURE;
        }
        if (vm.count("embedding_model"))
        {
            auto em = vm["embedding_model"].as<std::string>();
            std::transform(em.begin(), em.end(), em.begin(), [](unsigned char c){
                return std::tolower(c);
            });
            if (em != "hf" && em != "openai")
            {
                auto e = po::required_option("option '%canonical_option%' must be hf or openai");
                e.set_option_name(em);
                throw e;
            }
            embedding_model = em;
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

    auto lines = Utils::GetLines(filenamepath);

    for (auto& line : lines)
    {
        line = StringUtils::removeAccents(line);
    }

    RAGLibrary::LoaderDataStruct input(filenamepath, lines);

    auto initialTime = std::chrono::high_resolution_clock::now();

    auto chunk = Chunk::ChunkSimilarity(chunk_size, overlap, Utils::GetEmbeddingModel(embedding_model));
    auto documents = chunk.ProcessSingleDocument(input);

    std::osyncstream(std::cout) << "ChunkSimilarity("
        <<"chunk_size='" << chunk_size
        << "',overlap="<< overlap
    << ")" << std::endl;
    std::osyncstream(std::cout) << "  from '"<< filenamepath << "': documents.size=" << documents.size() << std::endl;
    for(const auto&doc : documents)
    {
        std::osyncstream(std::cout) << "  "<< doc << std::endl;
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    auto durationTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - initialTime).count();

    std::osyncstream(std::cout) << "execution time: " << durationTime << "ms" << std::endl;

    return EXIT_SUCCESS;
}
