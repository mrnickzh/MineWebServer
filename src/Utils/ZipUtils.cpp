#include "ZipUtils.hpp"

#include "../lib/miniz/miniz.h"
#include "../lib/miniz/miniz.c"

void ZipUtils::extract_all(std::string src, std::string dst) {
    std::filesystem::create_directory(dst);

    mz_zip_archive zip_archive;
    memset(&zip_archive, 0, sizeof(zip_archive));

    if (!mz_zip_reader_init_file(&zip_archive, src.c_str(), 0)) {
        printf("Failed to open zip archive\n");
        return;
    }

    for (int i = 0; i < (int)mz_zip_reader_get_num_files(&zip_archive); i++) {
        mz_zip_archive_file_stat file_stat;
        mz_zip_reader_file_stat(&zip_archive, i, &file_stat);

        // Construct target path (e.g., using std::filesystem or similar)
        std::string destFile = dst + "/" + std::string(file_stat.m_filename);

        if (mz_zip_reader_is_file_a_directory(&zip_archive, i)) {
            std::filesystem::create_directory(destFile);
            continue;
        }

        mz_zip_reader_extract_to_file(&zip_archive, i, destFile.c_str(), 0);
    }

    mz_zip_reader_end(&zip_archive);
}