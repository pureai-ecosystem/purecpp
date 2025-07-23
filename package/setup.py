from setuptools import setup, find_packages
import sys

package_data = {}

if sys.platform.startswith("win"):
    package_data["purecpp"] = [
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
    package_data["purecpp"] = [
        "*.so",
        "*.so.6"
    ]

setup(
    name="purecpp",
    version="0.0.18",
    description="All-in-one solution for building Retrieval-Augmented Generation (RAG) pipelines",
    author="PureAI",
    author_email="contato@pureai.com.br",
    packages=find_packages(),
    package_data=package_data,
    include_package_data=True,
    license="MIT",
    classifiers=[
        'Programming Language :: Python :: 3',
        'Operating System :: POSIX :: Linux',
    ],
    install_requires=[
        'purecpp_libs',
        'auditwheel',
        'build',
        'requests',
        'wheel',
        'pybind11',
        'setuptools',
    ],
)