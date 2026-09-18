#include "AssetImporter.hpp"
#include <iostream>
#include <string>
#include <exception>

void printUsage(const char* progName) {
    std::cout << "Bloons TD 4 Repopped - Native Asset Importer CLI\n"
              << "Usage:\n"
              << "  " << progName << " <source.swf> [options]\n\n"
              << "Options:\n"
              << "  --out <dir>        Output directory for extracted game_data (default: game_data)\n"
              << "  --ipa <file>       Optional mobile IPA package path\n"
              << "  --platform <name>  Target build platform: Windows, Linux, PSP, Xbox 360, or auto\n"
              << "  --help             Display this help message\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }

    std::string swfPath;
    std::string outDir = "game_data";
    std::string ipaPath;
    std::string platform = "auto";

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            printUsage(argv[0]);
            return 0;
        } else if (arg == "--out" && i + 1 < argc) {
            outDir = argv[++i];
        } else if (arg == "--ipa" && i + 1 < argc) {
            ipaPath = argv[++i];
        } else if (arg == "--platform" && i + 1 < argc) {
            platform = argv[++i];
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
    options.targetPlatform = platform;

    std::cout << "========================================\n"
              << " BTD4 Asset Importer\n"
              << " Source: " << swfPath << "\n"
              << " Output: " << outDir << "\n"
              << " Platform: " << platform << "\n"
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

    btd4::tools::ImportReport report;
    try {
        report = btd4::tools::AssetImporter::run(options, logCallback, progressCallback);
    } catch (const std::bad_alloc&) {
        std::cerr << "\nImport failed: importer ran out of memory." << std::endl;
        return 2;
    } catch (const std::exception& e) {
        std::cerr << "\nImport failed with exception: " << e.what() << std::endl;
        return 2;
    } catch (...) {
        std::cerr << "\nImport failed with an unknown exception." << std::endl;
        return 2;
    }
    std::cout << "\n";

    if (!report.success) {
        std::cerr << "\nImport failed: " << report.errorMessage << std::endl;
        return 1;
    }

    std::cout << "\n========================================\n"
              << " Import Summary:\n"
              << " Target platform: " << report.targetPlatform << "\n"
              << " Source family: " << report.sourceFamily << "\n"
              << " BTD4 detected: " << (report.btd4Detected ? "yes" : "no") << "\n"
              << " IPA detected: " << (report.ipaDetected ? "yes" : "no") << "\n"
              << " IPA archive detected: " << (report.ipaArchiveDetected ? "yes" : "no") << "\n"
              << " IPA files extracted: " << report.ipaFilesExtracted << "\n"
              << " IPA bytes extracted: " << report.ipaBytesExtracted << "\n"
              << " IPA output directory: " << report.ipaOutputDirectory << "\n"              << " Textures extracted: " << report.texturesExtracted << "\n"
              << " Audio cues extracted: " << report.soundsExtracted << "\n"
              << " Symbols mapped: " << report.symbolsMapped << "\n"
              << " Manifest: " << report.manifestPath << "\n"
              << "========================================\n";

    return 0;
}
