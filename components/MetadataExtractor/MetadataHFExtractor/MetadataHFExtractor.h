#ifndef METADATA_HFE_EXTRACTOR_H
#define METADATA_HFE_EXTRACTOR_H

#include "IMetadataHFExtractor.h"

#include "onnxruntime/cpu_provider_factory.h"
#include "onnxruntime/core/session/onnxruntime_cxx_api.h"
#include "tokenizers_cpp.h"
#include "FileUtilsLocal.h"
#include "Document.h"

namespace MetadataHFExtractor
{
    const std::string modelPath = "models/dbmdz/bert-large-cased-finetuned-conll03-english/model.onnx";
    const std::string tokenizerPath = "models/dbmdz/bert-large-cased-finetuned-conll03-english/tokenizer/tokenizer.json";
    const std::string labelMapPath = "models/dbmdz/bert-large-cased-finetuned-conll03-english/label_map.json";

    class MetadataHFExtractor : public IMetadataHFExtractor
    {
    public:
        MetadataHFExtractor();
        ~MetadataHFExtractor() = default;

        void InitializeNERModel() final;
        std::vector<std::pair<std::string, std::string>> ExtractMetadata(const std::vector<std::string> &text) final;
        ::MetadataExtractor::Document ProcessDocument(::MetadataExtractor::Document doc) final;

    private:
        std::shared_ptr<Ort::Env> m_env;
        Ort::SessionOptions m_sessionOptions;
        std::shared_ptr<Ort::Session> m_session;
        std::unique_ptr<tokenizers::Tokenizer> m_tokenizer;
        std::map<int, std::string> m_labelMap;

        void ReadingFromLabelMap(const std::string &filePath);
        std::vector<float> SoftMax(const std::vector<float> &logits);
        std::vector<std::string> ProcessLogits(float *logits, size_t seqLength, size_t numLabels, const std::map<int, std::string> &labelMap);
    };
}
#endif
