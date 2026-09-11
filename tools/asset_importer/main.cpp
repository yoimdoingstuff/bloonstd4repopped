#include "AssetImporter.hpp"
#include <iostream>
#include <string>

void printUsage(const char* progName) {
    std::cout << "Bloons TD 4 Repopped - Native Asset Importer CLI\n"
              << "Usage:\n"
              << "  " << progName << " <source.swf> [options]\n\n"
              << "Options:\n"
              << "  --out <dir>   Output directory for extracted game_data (default: game_data)\n"
              << "  --ipa <file>  Optional mobile IPA package path\n"
              << "  --help        Display this help message\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }

    std::string swfPath;
    std::string outDir = "game_data";
    std::string ipaPath;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            printUsage(argv[0]);
            return 0;
        } else if (arg == "--out" && i + 1 < argc) {
            outDir = argv[++i];
        } else if (arg == "--ipa" && i + 1 < argc) {
            ipaPath = argv[++i];
        } else if (arg.rfind("--", 0) != 0 && swfPath.empty()) {
            swfPath = arg;
        }
    }

    if (swfPath.empty()) {
        std::cerr << "Error: No SWF input file specified.\n";
        printUsage(argv[0]);
        return 1;
    }

    btd4::tools::ImportOptions options;
    options.sourceSwf = swfPath;
    options.sourceIpa = ipaPath;
    options.outputDir = outDir;

    std::cout << "========================================\n"
              << " BTD4 Asset Importer\n"
              << " Source: " << swfPath << "\n"
              << " Output: " << outDir << "\n"
              << "========================================\n";

    auto logCallback = [](const std::string& msg) {
        std::cout << msg << std::endl;
    };

    auto progressCallback = [](float p, const std::string& status) {
        int barWidth = 30;
        int pos = static_cast<int>(barWidth * p);
        std::cout << "\r[";
        for (int i = 0; i < barWidth; ++i) {
            if (i < pos) std::cout << "=";
            else if (i == pos) std::cout << ">";
            else std::cout << " ";
        }
        std::cout << "] " << static_cast<int>(p * 100.0f) << "% " << status << std::flush;
    };

    btd4::tools::ImportReport report = btd4::tools::AssetImporter::run(options, logCallback, progressCallback);
    std::cout << "\n";

    if (!report.success) {
        std::cerr << "\nImport failed: " << report.errorMessage << std::endl;
        return 1;
    }

    std::cout << "\n========================================\n"
              << " Import Summary:\n"
              << " Textures extracted: " << report.texturesExtracted << "\n"
              << " Audio cues extracted: " << report.soundsExtracted << "\n"
              << " Symbols mapped: " << report.symbolsMapped << "\n"
              << " Manifest: " << report.manifestPath << "\n"
              << "========================================\n";

    return 0;
}
