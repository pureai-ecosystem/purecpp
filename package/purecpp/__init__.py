import os
import sys
import ctypes
from setuptools import setup, find_packages
from setuptools.command.install import install
import subprocess
import shutil

REQUIRED_FILES = [
    # "libaoti_custom_ops.so",
    # "libbackend_with_compiler.so",
    "libc10.so",
    # "libjitbackend_test.so",
    # "libnnapi_backend.so",
    # "libshm.so",
    "libtorch.so",
    "libtorch_cpu.so",
    # "libtorch_global_deps.so",
    # "libtorch_python.so",
    # "libtorchbind_test.so",
]

def download_libtorch():
    libtorch_cpu_zip = "libtorch-cxx11-abi-shared-with-deps-2.5.0+cpu.zip"
    libtorch_cpu_url = (
        "https://download.pytorch.org/libtorch/cpu/"
        "libtorch-cxx11-abi-shared-with-deps-2.5.0%2Bcpu.zip"
    )
    
    pkg_dir = os.path.join(os.path.dirname(__file__), "d_libs")
    libtorch_dir = os.path.join(pkg_dir, "libtorch")
    cpu_dir = os.path.join(libtorch_dir, "cpu")
    lib_path = os.path.join(cpu_dir, "lib")

    all_files_present = True
    if os.path.exists(lib_path):
        for f in REQUIRED_FILES:
            if not os.path.exists(os.path.join(lib_path, f)):
                all_files_present = False
                break
    else:
        all_files_present = False

    if all_files_present:
        return
    else:
        print("Not all files are present. Downloading libtorch...")

    if os.path.exists(pkg_dir):
        shutil.rmtree(pkg_dir)
    os.makedirs(libtorch_dir, exist_ok=True)

    subprocess.check_call(["wget", libtorch_cpu_url, "-O", libtorch_cpu_zip])

    subprocess.check_call(["unzip", "-o", libtorch_cpu_zip, "-d", libtorch_dir])

    extracted_dir = os.path.join(libtorch_dir, "libtorch")
    if os.path.exists(extracted_dir):
        os.rename(extracted_dir, cpu_dir)
    else:
        print("Error: extracted_dir does not exist")

    os.remove(libtorch_cpu_zip)
    print("Libtorch downloaded and extracted successfully.")

    result = subprocess.run(["ls", pkg_dir], capture_output=True, text=True)
    print("d_libs/:", result.stdout)

download_libtorch()

LIB_PATH = os.path.join(os.path.dirname(__file__), "d_libs", "libtorch", "cpu", "lib")

current_ld_library_path = os.environ.get("LD_LIBRARY_PATH", "")

new_ld_library_path = f"{LIB_PATH}:/usr/local/lib:{current_ld_library_path}".strip(":")

os.environ["LD_LIBRARY_PATH"] = new_ld_library_path
try:
    # ctypes.cdll.LoadLibrary(os.path.join(LIB_PATH, "libaoti_custom_ops.so"))
    # ctypes.cdll.LoadLibrary(os.path.join(LIB_PATH, "libbackend_with_compiler.so"))
    ctypes.cdll.LoadLibrary(os.path.join(LIB_PATH, "libc10.so"))
    # ctypes.cdll.LoadLibrary(os.path.join(LIB_PATH, "libjitbackend_test.so"))
    # ctypes.cdll.LoadLibrary(os.path.join(LIB_PATH, "libnnapi_backend.so"))
    # ctypes.cdll.LoadLibrary(os.path.join(LIB_PATH, "libshm.so"))
    ctypes.cdll.LoadLibrary(os.path.join(LIB_PATH, "libtorch.so"))
    ctypes.cdll.LoadLibrary(os.path.join(LIB_PATH, "libtorch_cpu.so"))
    # ctypes.cdll.LoadLibrary(os.path.join(LIB_PATH, "libtorch_global_deps.so"))
    # ctypes.cdll.LoadLibrary(os.path.join(LIB_PATH, "libtorch_python.so"))
    # ctypes.cdll.LoadLibrary(os.path.join(LIB_PATH, "libtorchbind_test.so"))
except OSError as e:
    raise ImportError(f"libtorch error: {e}")

from .RagPUREAI import *