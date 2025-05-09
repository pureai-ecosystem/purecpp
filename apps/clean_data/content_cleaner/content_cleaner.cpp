#include <chrono>
#include <fstream>
#include <iostream>
#include <syncstream>
#include <vector>

#include <boost/program_options.hpp>

#include "ContentCleaner/ContentCleaner.h"
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
            ("custom_patterns", po::value<std::vector<std::string>>()->multitoken(), "Custom patterns (vector<string|regex>)")
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

    std::vector<std::string> custom_patterns;
    if (vm.count("custom_patterns"))
    {
        custom_patterns = vm["custom_patterns"].as<std::vector<std::string> >();
    }
    std::string filenamepath = vm["filenamepath"].as<std::string>();

    RAGLibrary::Metadata metadata = {{"fileIdentifer", filenamepath}};
    RAGLibrary::Document doc(metadata, Utils::GetText(filenamepath));

    auto initialTime = std::chrono::high_resolution_clock::now();

    std::osyncstream(std::cout) <<  "doc.page_content.size=" << doc.page_content.size() << std::endl;

    CleanData::ContentCleaner cleaner;
    auto document = cleaner.ProcessDocument(doc, custom_patterns);

    std::osyncstream(std::cout) <<  "ContentCleaner from '"<< filenamepath << "': document.page_content.size=" << document.page_content.size() << std::endl;

    auto endTime = std::chrono::high_resolution_clock::now();
    auto durationTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - initialTime).count();

    std::osyncstream(std::cout) << "execution time: " << durationTime << "ms" << std::endl;

    return EXIT_SUCCESS;
}
