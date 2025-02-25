# Before running this script, install with pip the following packages: torch, transformers, onnx, onnxruntime

import json
from transformers import AutoModelForTokenClassification, AutoTokenizer, AutoConfig
from transformers.onnx import export, FeaturesManager
from pathlib import Path
import argparse
import os

parser = argparse.ArgumentParser()
parser.add_argument("-m", "--model_name", help="Put the model name.", required=True)
parser.add_argument("-o", "--output_name", help="Put the output name for the model after conversion.", required=True)

args = parser.parse_args()

model_name = args.model_name
config = AutoConfig.from_pretrained(model_name)
label_map = config.id2label
dir_path= os.path.join(os.path.dirname(__file__), "..", "models", model_name)
if not os.path.exists(dir_path):
    os.makedirs(dir_path)

with open(f"{dir_path}/label_map.json", "w") as f:
    json.dump(label_map,f)

model = AutoModelForTokenClassification.from_pretrained(model_name)
tokenizer = AutoTokenizer.from_pretrained(model_name)
tokenizer.save_pretrained(f"{dir_path}/tokenizer")

model_type = model.config.model_type

feature = "token-classification"
outpath_path = Path(f"{dir_path}/model.onnx")

onnx_config = FeaturesManager.get_config(model_type=model_type,feature=feature)
onnx_config = onnx_config(model.config)

export(model=model, output=outpath_path, opset=14, preprocessor=tokenizer, config=onnx_config)
print(f"Model exported to {outpath_path}")