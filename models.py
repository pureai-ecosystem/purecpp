
"""
Script unificado para converter modelos Hugging Face para o formato ONNX.
Você pode alterar as variáveis globais abaixo para configurar os modelos a serem convertidos.
O script cria a pasta "models" (no diretório pai) caso ela não exista e realiza:
  1. Conversão de modelo para extração de características (ex: sentence-transformers).
  2. Conversão de modelo para classificação de tokens (ex: NER).
  
Antes de executar, certifique-se de instalar:
  - torch
  - transformers
  - onnx
  - onnxruntime
  - optimum
"""

import os
import json
from pathlib import Path

# =====================
# Variáveis Globais
# =====================
# Altere estes valores conforme necessário
FEATURE_MODEL_NAME = "sentence-transformers/all-MiniLM-L6-v2"
TOKEN_MODEL_NAME   = "dslim/bert-base-NER"
TOKEN_OUTPUT_NAME  = "bert-base-ner-converted"

# Diretório base onde os modelos serão salvos (um diretório "models" no diretório pai deste script)
BASE_DIR = os.path.join(os.path.dirname(__file__), "models")

# Cria o diretório BASE_DIR se ele não existir
if not os.path.exists(BASE_DIR):
    os.makedirs(BASE_DIR)
    print(f"Diretório criado: {BASE_DIR}")

# =====================
# Função para conversão de modelo para extração de características
# =====================
def convert_feature_extraction_model(model_name):
    print(f"\nConvertendo modelo de extração de características: {model_name}")
    from optimum.onnxruntime import ORTModelForFeatureExtraction
    from transformers import AutoTokenizer

    dir_path = os.path.join(BASE_DIR, model_name)
    if not os.path.exists(dir_path):
        os.makedirs(dir_path)
        print(f"Diretório criado para o modelo: {dir_path}")

    model = ORTModelForFeatureExtraction.from_pretrained(model_name, export=True)
    tokenizer = AutoTokenizer.from_pretrained(model_name)

    model.save_pretrained(dir_path)
    tokenizer.save_pretrained(dir_path)
    print(f"Modelo de extração salvo em: {dir_path}")

# =====================
# Função para conversão de modelo para classificação de tokens (ex: NER)
# =====================
def convert_token_classification_model(model_name, output_name):
    print(f"\nConvertendo modelo de classificação de tokens: {model_name}")
    from transformers import AutoModelForTokenClassification, AutoTokenizer, AutoConfig
    from transformers.onnx import export, FeaturesManager

    config = AutoConfig.from_pretrained(model_name)
    label_map = config.id2label
    dir_path = os.path.join(BASE_DIR, model_name)
    if not os.path.exists(dir_path):
        os.makedirs(dir_path)
        print(f"Diretório criado para o modelo: {dir_path}")

    # Salva o label map em um arquivo JSON
    label_map_path = os.path.join(dir_path, "label_map.json")
    with open(label_map_path, "w") as f:
        json.dump(label_map, f)
    print(f"Label map salvo em: {label_map_path}")

    model = AutoModelForTokenClassification.from_pretrained(model_name)
    tokenizer = AutoTokenizer.from_pretrained(model_name)
    tokenizer_save_path = os.path.join(dir_path, "tokenizer")
    tokenizer.save_pretrained(tokenizer_save_path)
    print(f"Tokenizador salvo em: {tokenizer_save_path}")

    model_type = model.config.model_type
    feature = "token-classification"
    outpath = Path(os.path.join(dir_path, "model.onnx"))

    onnx_config = FeaturesManager.get_config(model_type=model_type, feature=feature)
    onnx_config = onnx_config(model.config)

    export(model=model, output=outpath, opset=14, preprocessor=tokenizer, config=onnx_config)
    print(f"Modelo de classificação exportado para: {outpath}")

# =====================
# Função principal
# =====================
def main():
    # Conversão para extração de características
    convert_feature_extraction_model(FEATURE_MODEL_NAME)
    
    # Conversão para classificação de tokens (NER)
    convert_token_classification_model(TOKEN_MODEL_NAME, TOKEN_OUTPUT_NAME)

if __name__ == "__main__":
    main()
