#include "ChunkCommons.h"
#include "RagException.h"
#include "StringUtils.h"

#include <nlohmann/json.hpp>
#include <onnxruntime/core/session/onnxruntime_cxx_api.h>
#include <onnxruntime/cpu_provider_factory.h>
#include <openai/openai.hpp>
#include <tokenizers_cpp.h>
#include <torch/script.h>

#include <algorithm>
#include <cmath>
#include <numeric>
#include <omp.h>
#include <syncstream>
#include <format>

using namespace Chunk;

namespace RAGLibrary
{
    static std::string FileReader(const std::string &filePath)
    {
        std::shared_ptr<std::ifstream> filePtr(new std::ifstream, [](std::ifstream *fil)
                                               { fil->close(); });

        try
        {
            filePtr->exceptions(std::ios::failbit);
            filePtr->open(filePath, std::ios::in);
            return {std::istreambuf_iterator<char>{*filePtr}, std::istreambuf_iterator<char>{}};
        }
        catch (const std::ios::failure &e)
        {
            std::osyncstream(std::cerr) << e.what() << std::endl;
            throw RAGLibrary::RagException(e.what());
        }
        return std::string();
    }

}

template <typename T>
inline Ort::Value CreateTensorOrt(Ort::AllocatorWithDefaultOptions &allocator,
                                  std::vector<T> &data, std::vector<int64_t> &shape)
{
    return Ort::Value::CreateTensor<T>(allocator.GetInfo(), data.data(), data.size(), shape.data(), shape.size());
}

std::vector<float> Chunk::MeanPooling(const std::vector<float> &token_embeddings, const std::vector<int64_t> &attention_mask, size_t embedding_size)
{
    size_t num_tokens = token_embeddings.size() / embedding_size;
    std::vector<float> pooled_embeddings(embedding_size, 0.0f);

    for (int i = 0; i < num_tokens; ++i)
    {
        if (attention_mask[i] == 1)
        {
            for (int j = 0; j < embedding_size; ++j)
            {
                pooled_embeddings[j] += token_embeddings[i * embedding_size + j];
            }
        }
    }

    int valid_tokens = std::accumulate(attention_mask.begin(), attention_mask.end(), 0);
    for (int j = 0; j < embedding_size; ++j)
    {
        pooled_embeddings[j] /= std::max(valid_tokens, 1);
    }

    return pooled_embeddings;
}

void Chunk::NormalizeEmbeddings(std::vector<float> &embeddings)
{
    float norm = std::sqrt(std::inner_product(embeddings.begin(), embeddings.end(), embeddings.begin(), 0.0f));
    for (float &value : embeddings)
    {
        value /= norm;
    }
}

std::vector<std::vector<float>> Chunk::EmbeddingModelBatch(const std::vector<std::string> &chunks, const std::string &model, const int batch_size)
{
    const auto model_c = model.c_str();
    const std::string modelPath = std::format("models/{}/model.onnx", model_c);
    const std::string tokenizerPath = std::format("models/{}/tokenizer.json", model_c);

    auto env = std::make_shared<Ort::Env>(ORT_LOGGING_LEVEL_WARNING, "NER");
    Ort::SessionOptions sessionOptions;
    sessionOptions.SetInterOpNumThreads(1);

    auto session = std::make_shared<Ort::Session>(*env, modelPath.c_str(), sessionOptions);
    auto blob = RAGLibrary::FileReader(tokenizerPath);
    auto tokenizer = tokenizers::Tokenizer::FromBlobJSON(blob);

    Ort::AllocatorWithDefaultOptions allocator;

    std::vector<std::vector<float>> results;
    for (int start_idx = 0; start_idx < chunks.size(); start_idx += batch_size)
    {
        size_t end_idx = std::min<ptrdiff_t>(start_idx + batch_size, chunks.size());
        std::vector<std::string> texts(chunks.begin() + start_idx, chunks.begin() + end_idx);
        auto encode_batch = tokenizer->EncodeBatch(chunks);

        size_t total_size = std::accumulate(encode_batch.begin(), encode_batch.end(), std::size_t(0),
                                            [](std::size_t sum, const std::vector<int32_t> &encode)
                                            {
                                                return sum + encode.size();
                                            });

        std::vector<int64_t> inputIds(total_size, 0);
        std::vector<int64_t> attentionMask(total_size, 0);
        size_t ii = 0;
        for (int i = 0; i < encode_batch.size(); ++i)
        {
            for (auto index = 0; index < encode_batch[i].size(); ++index)
            {
                inputIds[ii + index] = encode_batch[i][index];
                inputIds[ii + index] > 0 ? attentionMask[ii + index] = 1 : attentionMask[ii + index] = 0;
            }
            ii += encode_batch[i].size();
        }
        std::vector<int64_t> tokenTypeIds(total_size, 0);
        std::vector<int64_t> inputShape{int64_t(encode_batch.size()), int64_t(encode_batch.front().size())};

        auto attentionTensor = CreateTensorOrt<int64_t>(allocator, attentionMask, inputShape);
        auto inputTensor = CreateTensorOrt<int64_t>(allocator, inputIds, inputShape);
        auto tokenTypeTensor = CreateTensorOrt<int64_t>(allocator, tokenTypeIds, inputShape);

        std::vector<Ort::Value> inputTensors;
        inputTensors.emplace_back(std::move(inputTensor));
        inputTensors.emplace_back(std::move(attentionTensor));
        inputTensors.emplace_back(std::move(tokenTypeTensor));

        const char *inputNames[] = {"input_ids", "attention_mask", "token_type_ids"};
        const char *outputNames[] = {"logits"};
        std::vector<Ort::Value> outputTensors = session->Run(Ort::RunOptions(nullptr), inputNames, inputTensors.data(), 3, outputNames, 1);

        float *logits = outputTensors.front().GetTensorMutableData<float>();
        size_t outputSize = outputTensors.front().GetTensorTypeAndShapeInfo().GetElementCount();

        results.reserve(results.size() + encode_batch.size());
        ii = 0;
        size_t numLabels = outputSize / total_size;
        for (int i = 0; i < encode_batch.size(); i++)
        {
            std::vector<float> tokenLogits(logits + ii * numLabels, logits + (ii + encode_batch[i].size()) * numLabels);
            results.push_back(std::move(tokenLogits));
            ii += encode_batch[i].size();
        }
    }

    return results;
}

std::vector<std::vector<float>> Chunk::EmbeddingOpeanAI(const std::vector<std::string> &chunks, const std::string &openai_api_key)
{
    std::vector<std::vector<float>> results;
    openai::start(openai_api_key);
    results.reserve(chunks.size());
    for (int i = 0; i < chunks.size(); i++)
    {
        auto &chunk = chunks[i];
        auto startTime = std::chrono::high_resolution_clock::now();
        auto values = openai::embedding().create(openai::_detail::Json{
            {"input", std::vector<std::string>{chunk}},
            {"model", "text-embedding-ada-002"},
        })["data"][0]["embedding"];
        if (values.is_array())
        {
            results.push_back(values.get<std::vector<float>>());
        }
    }
    return results;
}

at::Tensor Chunk::toTensor(std::vector<std::vector<float>> &vect)
{
    int64_t n = vect.size();
    int64_t m = vect.front().size();
    auto options = torch::TensorOptions().dtype(torch::kFloat32);
    auto tensor = torch::zeros({n, m}, options);
    for (int64_t i = 0; i < n; ++i)
    {
        tensor.slice(0, i, i + 1) = torch::from_blob(vect[i].data(), {m}, options);
    }
    return tensor;
}
std::vector<std::string> Chunk::SplitText(const std::vector<std::string> &inputs, const int overlap, const int chunk_size)
{
    std::string text;
    StringUtils::joinStr("\n", inputs, text);

    if (text.empty()){
        return {};
    }

    size_t step = chunk_size > overlap ? chunk_size - overlap : 0;
    if (!step){
        if (text.size() > chunk_size){
            return {text};
        }
    }

    std::vector<std::string> chunks;
    size_t start_index = 0;

    while (start_index < text.size()){
        size_t end_index = std::min(start_index + chunk_size, text.size());

        // Adjust end_index to not split a UTF-8 character
        while (end_index > start_index && (text[end_index] & 0xC0) == 0x80){
            end_index--;
        }

        std::string current_chunk = text.substr(start_index, end_index - start_index);
        chunks.push_back(current_chunk);

        if (end_index == text.size()){
            break;
        }

        size_t next_start_index = (end_index > overlap) ? (end_index - overlap) : 0;

        // Adjust start_index to not split a UTF-8 character
        while (next_start_index > start_index && (text[next_start_index] & 0xC0) == 0x80){
            next_start_index--;
        }

        if (next_start_index <= start_index){
            start_index = end_index;
        }else{
            start_index = next_start_index;
        }
    }

    return chunks;
}

std::vector<std::string> Chunk::SplitTextByCount(const std::string &input, int overlap, int count_threshold, const std::shared_ptr<re2::RE2> regex)
{
    std::vector<std::string> chunks;
    std::vector<re2::StringPiece> matches;

    re2::StringPiece text(input);
    re2::StringPiece match;

    while (re2::RE2::FindAndConsume(&text, *regex, &match))
    {
        matches.push_back(match);
    }

    size_t start_idx = size_t(0);
    for (int i = 0; i < matches.size(); i += count_threshold)
    {
        size_t j = i + count_threshold;
        size_t end_idx = text.size() + 1;
        if (j < matches.size())
        {
            end_idx = (matches[j - 1].data() - input.data()) + matches[j - 1].size();
        }
        chunks.push_back(std::string(text.substr(start_idx, end_idx - start_idx)));
        start_idx = end_idx > overlap ? size_t(end_idx - overlap) : size_t(0);
    }

    return chunks;
}
