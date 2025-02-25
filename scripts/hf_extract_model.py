from optimum.onnxruntime import ORTModelForFeatureExtraction
from transformers import AutoTokenizer
import os
import argparse

if __name__ == "__main__":

    parser = argparse.ArgumentParser()
    parser.add_argument("-m", "--model_name", help="Put the model name.", required=True)
    # parser.add_argument("-o", "--output_name", help="Put the output name for the model after conversion.", required=True)

    args = parser.parse_args()

    model_name = args.model_name

    dir_path= os.path.join(os.path.dirname(__file__), "..", "models", model_name)
    if not os.path.exists(dir_path):
        os.makedirs(dir_path)

    model = ORTModelForFeatureExtraction.from_pretrained(model_name, export=True)
    tokenizer = AutoTokenizer.from_pretrained(model_name)

    model.save_pretrained(dir_path)
    tokenizer.save_pretrained(dir_path)
