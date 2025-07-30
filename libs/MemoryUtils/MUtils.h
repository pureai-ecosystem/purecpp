#pragma once

#include <tuple>
#include <string>
#include <format>
#include <iostream> 
#ifdef _WIN32
    #include <windows.h>
#elif __linux__
    #include <unistd.h>
    #include <sys/sysinfo.h>
#else
    #error "Unsupported OS"
#endif

namespace RAGLibrary{
    
    struct RamInfo {
        size_t total_bytes;
        size_t free_bytes;
    };

    inline RamInfo get_ram_info() {
        RamInfo info = {0, 0};

    #ifdef _WIN32
        MEMORYSTATUSEX status;
        status.dwLength = sizeof(status);
        GlobalMemoryStatusEx(&status);
        info.total_bytes = static_cast<size_t>(status.ullTotalPhys);
        info.free_bytes = static_cast<size_t>(status.ullAvailPhys);

    #elif __linux__
        struct sysinfo sys_info;
        if (sysinfo(&sys_info) == 0) {
            info.total_bytes = static_cast<size_t>(sys_info.totalram) * sys_info.mem_unit;
            info.free_bytes = static_cast<size_t>(sys_info.freeram) * sys_info.mem_unit;
        }

    #endif
        return info;
    }

    inline size_t get_total_system_ram_bytes() {
        return get_ram_info().total_bytes;
    }

    inline std::tuple<double, std::string> convert_bytes(size_t bytes) {
        const char* units[] = { "B", "KB", "MB", "GB", "TB", "PB" };
        int unit_index = 0;
        double size = static_cast<double>(bytes);

        while (size >= 1024.0 && unit_index < 5) {
            size /= 1024.0;
            ++unit_index;
        }

        return std::make_tuple(size, std::string(units[unit_index]));
    }

    inline size_t print_memory() {
        auto info = RAGLibrary::get_ram_info();

        auto [total, unit1] = RAGLibrary::convert_bytes(info.total_bytes);
        auto [free, unit2] = RAGLibrary::convert_bytes(info.free_bytes);

        std::cout << std::format("📦 Total Memory: {:.2f} {}\n", total, unit1);
        std::cout << std::format("🟢 Memory Free: {:.2f} {}\n", free, unit2);

        return 0;
    }
}
