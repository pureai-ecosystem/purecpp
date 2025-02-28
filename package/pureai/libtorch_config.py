import os
import ctypes
import sys
import shutil
import zipfile
import requests
import platform


def load_libtorch():
    LIBTORCH_DIR = "./libtorch"
    LIBTORCH_CPU_ZIP = "libtorch-cxx11-abi-shared-with-deps-2.5.1+cpu.zip"
    if platform.system() == "Windows":
        LIBTORCH_CPU_URL = "https://download.pytorch.org/libtorch/cpu/libtorch-win-shared-with-deps-debug-2.6.0%2Bcpu.zip"
    else:
        LIBTORCH_CPU_URL = "https://download.pytorch.org/libtorch/cpu/libtorch-cxx11-abi-shared-with-deps-2.5.1%2Bcpu.zip"
    LIBTORCH_CPU_PATH = os.path.join(LIBTORCH_DIR, "cpu")
    LIBTORCH_LIB_PATH = os.path.join(LIBTORCH_CPU_PATH, "lib")
    LIBTORCH_BIN_PATH = os.path.join(LIBTORCH_CPU_PATH, "bin")

    print(LIBTORCH_CPU_PATH)
    os.path.exists(LIBTORCH_CPU_PATH)
    if not os.path.exists(LIBTORCH_CPU_PATH):
        if os.path.exists(LIBTORCH_CPU_ZIP):
            os.remove(LIBTORCH_CPU_ZIP)

        response = requests.get(LIBTORCH_CPU_URL, stream=True)
        with open(LIBTORCH_CPU_ZIP, "wb") as file:
            for chunk in response.iter_content(chunk_size=8192):
                file.write(chunk)

        os.makedirs(LIBTORCH_DIR, exist_ok=True)

        if platform.system() == "Windows":
            with zipfile.ZipFile(LIBTORCH_CPU_ZIP, "r") as zip_ref:
                zip_ref.extractall(LIBTORCH_DIR)
        else:
            os.system(f"unzip {LIBTORCH_CPU_ZIP} -d {LIBTORCH_DIR}")

        shutil.move(os.path.join(LIBTORCH_DIR, "libtorch"), LIBTORCH_CPU_PATH)

    if platform.system() == "Windows":
        os.environ["PATH"] = LIBTORCH_BIN_PATH + \
            ";" + os.environ.get("PATH", "")
    else:
        os.environ["LD_LIBRARY_PATH"] = LIBTORCH_LIB_PATH + \
            ":" + os.environ.get("LD_LIBRARY_PATH", "")

    try:
        if platform.system() == "Windows":
            for dll_file in os.listdir(LIBTORCH_BIN_PATH):
                if dll_file.endswith(".dll"):
                    ctypes.CDLL(os.path.join(LIBTORCH_BIN_PATH, dll_file))
        else:
            print("C")
            for so_file in os.listdir(LIBTORCH_LIB_PATH):
                if so_file.endswith(".so"):
                    ctypes.CDLL(os.path.join(LIBTORCH_LIB_PATH, so_file))
    except OSError as e:
        print(e)
        print(f"❌ Error to load libtorch")
        sys.exit(1)
