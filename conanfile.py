from conan import ConanFile
from conan.tools.gnu import PkgConfigDeps
from conan.tools.cmake import CMakeToolchain, CMakeDeps, cmake_layout


class RagPureAIConan(ConanFile):
    name = "RagPureAI"
    version = "1.0"
    settings = "os", "compiler", "build_type", "arch"
    options = {"shared": [True, False], "fPIC": [
        True, False], "CURL_STATIC_LINKING": [True, False]}
    default_options = {"shared": True,
                       "fPIC": True, "CURL_STATIC_LINKING": False}
    generators = "CMakeDeps", "CMakeToolchain", "PkgConfigDeps"

    def layout(self):
        cmake_layout(self)

    def requirements(self):
        self.requires("protobuf/3.21.12")
        self.requires("pdfium/95.0.4629")
        self.requires("icu/74.1")
        self.requires("miniz/3.0.2")
        self.requires("rapidxml/1.13")
        self.requires("pybind11/2.13.6")
        self.requires("beauty/1.0.4")
        self.requires("lexbor/2.3.0")
        self.requires("re2/20231101")
        self.requires("boost/1.85.0", force=True)
        self.requires("onnxruntime/1.18.1")
        self.requires("nlohmann_json/3.11.3")
        self.requires("libcurl/8.10.1")

    def configure(self):
        if self.options.CURL_STATIC_LINKING:
            self.options["libcurl"].shared = False
