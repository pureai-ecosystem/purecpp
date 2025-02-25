from setuptools import setup, find_packages
from setuptools.command.install import install
import sys
import os


class PostInstall(install):
    def run(self):
        install.run(self)
        lib_path = os.path.abspath(os.path.join(
            self.install_lib, "pureai"))
        if "LD_LIBRARY_PATH" in os.environ:
            os.environ["LD_LIBRARY_PATH"] += f":{lib_path}"
        else:
            os.environ["LD_LIBRARY_PATH"] = lib_path


package_data = {}

if sys.platform.startswith("win"):
    package_data["pureai"] = [
        "RagPUREAI.cp312-win_amd64.pyd",
        "asmjit.dll",
        "c10.dll",
        "fbgemm.dll",
        "libiomp5md.dll",
        "libiompstubs5md.dll",
        "torch_cpu.dll",
        "torch_global_deps.dll",
        "torch.dll",
        "uv.dll"
    ]

elif sys.platform.startswith("linux"):
    package_data["pureai"] = [
        "*.so",
        "*.so.6"
    ]

setup(
    name="PureAI",
    version="0.0.11",
    description="All-in-one solution for building Retrieval-Augmented Generation (RAG) pipelines",
    author="PureAI",
    author_email="contato@pureai.com.br",
    packages=find_packages(),
    package_data=package_data,
    include_package_data=True,
    # cmdclass={"install": PostInstall},
    license="MIT",
    install_requires=["requests"],
    classifiers=[
        "Programming Language :: Python :: 3",
        "License :: OSI Approved :: MIT License",
        "Operating System :: Microsoft :: Windows",
        "Operating System :: POSIX :: Linux",
    ],
    python_requires=">=3.6",
)
