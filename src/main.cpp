#include "app.h"

#ifdef _WIN32
#include <windows.h>
#endif

#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>

namespace {

void print_version() { std::cout << "hashchecker v1.0.0\n"; }

void print_help(const char* prog) {
    std::cout << "Usage: " << prog
              << " --target <dir> --reference <dir> [options]\n\n"
                 "Options:\n"
                 "  -t, --target <dir>       Target/copy directory (required)\n"
                 "  -r, --reference <dir>    Reference/original directory (required)\n"
                 "  -a, --algorithm <alg>    Hash algorithm: md5, sha1, sha256 (default: "
                 "md5)\n"
                 "  -j, --threads <n>        Total threads (default: 80%% CPU, by file count)\n"
                 "  -T, --target-threads <n> Threads for -t drive (overrides -j split)\n"
                 "  -R, --ref-threads <n>    Threads for -r drive (overrides -j split)\n"
                 "  -o, --output <file>      Save report to file\n"
                 "  -v, --verbose            Show per-file progress\n"
                 "  -q, --quiet              Minimal output\n"
                 "      --json               JSON output format\n"
                 "      --save-logs          Save per-file hash logs\n"
                 "  -h, --help               Show this help\n"
                 "      --version            Show version\n\n"
                 "Exit codes: 0 = identical, 1 = differences found, 2 = error\n";
}

AppConfig parse_args(int argc, char* argv[]) {
    AppConfig config;

    for (int i = 1; i < argc; ++i) {
        std::string_view arg(argv[i]);

        if (arg == "-h" || arg == "--help") {
            print_help(argv[0]);
            std::exit(EXIT_SUCCESS);
        } else if (arg == "--version") {
            print_version();
            std::exit(EXIT_SUCCESS);
        } else if (arg == "-t" || arg == "--target") {
            if (++i >= argc) {
                std::cerr << "Error: --target requires a path argument\n";
                std::exit(2);
            }
            config.target_dir = argv[i];
        } else if (arg == "-r" || arg == "--reference") {
            if (++i >= argc) {
                std::cerr << "Error: --reference requires a path argument\n";
                std::exit(2);
            }
            config.reference_dir = argv[i];
        } else if (arg == "-a" || arg == "--algorithm") {
            if (++i >= argc) {
                std::cerr << "Error: --algorithm requires a value\n";
                std::exit(2);
            }
            config.algorithm = argv[i];
        } else if (arg == "-j" || arg == "--threads") {
            if (++i >= argc) {
                std::cerr << "Error: --threads requires a number\n";
                std::exit(2);
            }
            config.num_threads = static_cast<std::size_t>(std::stoul(argv[i]));
        } else if (arg == "-T" || arg == "--target-threads") {
            if (++i >= argc) {
                std::cerr << "Error: --target-threads requires a number\n";
                std::exit(2);
            }
            config.target_threads = static_cast<std::size_t>(std::stoul(argv[i]));
        } else if (arg == "-R" || arg == "--ref-threads") {
            if (++i >= argc) {
                std::cerr << "Error: --ref-threads requires a number\n";
                std::exit(2);
            }
            config.reference_threads = static_cast<std::size_t>(std::stoul(argv[i]));
        } else if (arg == "-o" || arg == "--output") {
            if (++i >= argc) {
                std::cerr << "Error: --output requires a file path\n";
                std::exit(2);
            }
            config.output_file = argv[i];
        } else if (arg == "-v" || arg == "--verbose") {
            config.verbose = true;
        } else if (arg == "-q" || arg == "--quiet") {
            config.quiet = true;
            config.report_format = "quiet";
        } else if (arg == "--json") {
            config.report_format = "json";
        } else if (arg == "--save-logs") {
            config.save_logs = true;
        } else {
            std::cerr << "Error: unknown option '" << arg << "'\n";
            std::cerr << "Try '" << argv[0] << " --help' for usage.\n";
            std::exit(2);
        }
    }

    return config;
}

} // namespace

int main(int argc, char* argv[]) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    AppConfig config = parse_args(argc, argv);

    if (config.target_dir.empty()) {
        std::cerr << "Error: --target is required.\n";
        return 2;
    }
    if (config.reference_dir.empty()) {
        std::cerr << "Error: --reference is required.\n";
        return 2;
    }

    return run(config);
}
