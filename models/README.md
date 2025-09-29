---
---

# Download Pre-trained Models

## 🛠️ Hugging Face to **ONNX** Converter:

These Python scripts convert Hugging Face models into the ONNX format for optimized inference.

These scripts handle two primary use cases:
1. **Feature extraction models** (e.g., `sentence-transformers`).
2. **Token classification models** (e.g., Named Entity Recognition - NER).

It automatically downloads the model and organizes the exported files in a structured subdirectory.

## Requirements
  
 *Before running the script, make sure you have the following Python packages installed:*  
  ```bash
  pip install torch transformers onnx onnxruntime optimum
  ```

## Examples

```bash
python3 models/hf_model_to_onnx.py -m="dbmdz/bert-large-cased-finetuned-conll03-english" -o="bert-large-cased-finetuned-conll03-english"
```

```bash
python3 models/hf_model_to_onnx.py -m="sentence-transformers/all-MiniLM-L6-v2" -o="sentence-transformers/all-MiniLM-L6-v2"
```

## Output

```
./models/
    ├── hf_extract_model.py
    ├── hf_model_to_onnx.py
    ├── sentence-transformers/all-MiniLM-L6-v2/ 
    │    ├── model.onnx (via optimum)
    │    └── tokenizer/ 
    └── dslim/bert-base-NER/  
        ├── model.onnx  
        ├── label_map.json  
        └── tokenizer/ 
```

---
