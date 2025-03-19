#include "MetadataHFExtractor.h"

#include <algorithm>
#include <cmath>
#include <unordered_map>

#include "nlohmann/json.hpp"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#undef min
#undef max
#undef format

static std::wstring to_wstring_utf16(const std::string &utf8)
{
    int needed_size = MultiByteToWideChar(CP_UTF8, 0, utf8.data(),
                                          (int)utf8.size(), nullptr, 0);
    std::wstring wstr(needed_size, 0);
    MultiByteToWideChar(CP_UTF8, 0, utf8.data(),
                        (int)utf8.size(), wstr.data(), needed_size);
    return wstr;
}
#endif

namespace MetadataHFExtractor
{
    MetadataHFExtractor::MetadataHFExtractor()
    {
    }

    void MetadataHFExtractor::InitializeNERModel()
    {
        m_env = std::make_shared<Ort::Env>(ORT_LOGGING_LEVEL_WARNING, "NER");
        m_sessionOptions.SetInterOpNumThreads(1);

        #ifdef _WIN32
        std::wstring wpath = to_wstring_utf16(modelPath);
        m_session = std::make_shared<Ort::Session>(*m_env, wpath.c_str(), m_sessionOptions);
        #else
        m_session = std::make_shared<Ort::Session>(*m_env, modelPath.c_str(), m_sessionOptions);
        #endif

        auto blob = RAGLibrary::FileReader(tokenizerPath);
        m_tokenizer = tokenizers::Tokenizer::FromBlobJSON(blob);
        ReadingFromLabelMap(labelMapPath);

        std::cout << "Model loaded successfully!" << std::endl;
    }

    std::vector<std::pair<std::string, std::string>> MetadataHFExtractor::ExtractMetadata(const std::vector<std::string> &text)
    {
        Ort::AllocatorWithDefaultOptions allocator;
        std::string fullText;
        for (auto elem : text)
        {
            fullText += elem + " ";
        }
        auto tmpIds = m_tokenizer->Encode(fullText);

        std::vector<int64_t> inputIds(tmpIds.size(), 0);
        std::vector<int64_t> attentionMask(tmpIds.size(), 0);
        for (auto index = 0; index < inputIds.size(); ++index)
        {
            inputIds[index] = tmpIds[index];
            inputIds[index] > 0 ? attentionMask[index] = 1 : attentionMask[index] = 0;
        }

        std::vector<int64_t> tokenTypeIds(tmpIds.size(), 0);
        std::vector<int64_t> inputShape{1, static_cast<int64_t>(tmpIds.size())};

        Ort::Value attentionTensor = Ort::Value::CreateTensor<int64_t>(allocator.GetInfo(), attentionMask.data(), attentionMask.size(), inputShape.data(), inputShape.size());
        Ort::Value inputTensor = Ort::Value::CreateTensor<int64_t>(allocator.GetInfo(), inputIds.data(), inputIds.size(), inputShape.data(), inputShape.size());
        Ort::Value tokenTypeTensor = Ort::Value::CreateTensor<int64_t>(allocator.GetInfo(), tokenTypeIds.data(), tokenTypeIds.size(), inputShape.data(), inputShape.size());

        const char *inputNames[] = {"input_ids", "attention_mask", "token_type_ids"};
        std::vector<Ort::Value> inputTensors;
        inputTensors.emplace_back(std::move(inputTensor));
        inputTensors.emplace_back(std::move(attentionTensor));
        inputTensors.emplace_back(std::move(tokenTypeTensor));

        const char *outputNames[] = {"logits"};
        auto outputTensors = m_session->Run(Ort::RunOptions(nullptr), inputNames, inputTensors.data(), 3, outputNames, 1);

        auto logits = outputTensors[0].GetTensorMutableData<float>();
        auto outputSize = outputTensors[0].GetTensorTypeAndShapeInfo().GetElementCount();
        size_t seqLength = inputIds.size();
        size_t numLabels = m_labelMap.size();
        auto returnVec = ProcessLogits(logits, seqLength, numLabels, m_labelMap);

        std::vector<std::pair<std::string, std::string>> returnValues;
        for (auto index = 0; index < tmpIds.size(); ++index)
        {
            returnValues.emplace_back(m_tokenizer->IdToToken(tmpIds[index]), returnVec[index]);
        }
        return returnValues;
    }

    ::MetadataExtractor::Document MetadataHFExtractor::ProcessDocument(::MetadataExtractor::Document doc)
    {
        doc.metadata = ExtractMetadata(doc.pageContent);
        return doc;
    }

    void MetadataHFExtractor::ReadingFromLabelMap(const std::string &filePath)
    {
        std::ifstream labelMapFile(filePath);
        if (labelMapFile.is_open())
        {
            nlohmann::json labelJson;
            labelMapFile >> labelJson;
            for (const auto &[key, value] : labelJson.items())
            {
                m_labelMap[std::stoi(key)] = value;
            }
        }
        else
        {
            std::cerr << "Failed to load label map!" << std::endl;
        }
    }

    std::vector<float> MetadataHFExtractor::SoftMax(const std::vector<float> &logits)
    {
        std::vector<float> probabilities(logits.size());
        float max_logit = *std::max_element(logits.begin(), logits.end());

        float sum = 0.0;
        for (auto index = 0; index < logits.size(); ++index)
        {
            probabilities[index] = std::exp(logits[index] - max_logit);
            sum += probabilities[index];
        }

        for (float &prob : probabilities)
        {
            prob /= sum;
        }

        return probabilities;
    }

    std::vector<std::string> MetadataHFExtractor::ProcessLogits(float *logits, size_t seqLength, size_t numLabels, const std::map<int, std::string> &labelMap)
    {
        std::vector<std::string> returnValues;
        for (int tokenIdx = 0; tokenIdx < seqLength; ++tokenIdx)
        {
            std::vector<float> tokenLogits(logits + tokenIdx * numLabels, logits + (tokenIdx + 1) * numLabels);
            std::vector<float> probabilities = SoftMax(tokenLogits);

            int labelId = std::distance(probabilities.begin(), std::max_element(probabilities.begin(), probabilities.end()));

            std::string tokenLabel = m_labelMap.at(labelId);
            returnValues.emplace_back(tokenLabel);
        }
        return returnValues;
    }

}