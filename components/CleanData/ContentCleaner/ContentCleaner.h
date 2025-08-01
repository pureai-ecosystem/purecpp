#ifndef CONTENT_CLEANER_H
#define CONTENT_CLEANER_H

#include <vector>
#include <string>

#include "CommonStructs.h"

namespace CleanData
{
    class ContentCleaner
    {
        public:
            ContentCleaner(const std::vector<std::string>& default_patterns = {});
            ~ContentCleaner() = default;
            RAGLibrary::Document ProcessDocument(const RAGLibrary::Document& doc, const std::vector<std::string>& custom_patterns = {});
            std::vector<RAGLibrary::Document> ProcessDocuments(const std::vector<RAGLibrary::Document>& docs, const std::vector<std::string>& custom_patterns = {}, int max_workers = 4);
        protected:
            std::string CleanContent(const std::string& text, const std::vector<std::string>& custom_patterns = {});
            void ValidatePatterns(const std::vector<std::string>& patterns);
        private:
            std::vector<std::string> m_default_patterns;
    };

}
#endif
