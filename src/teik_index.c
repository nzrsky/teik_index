#include <stdio.h>
#include <string.h>
#include <zlib.h>
#include <magic.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>
#include <stdbool.h>
#include <zip.h>

#if defined(__APPLE__)
    #include <CommonCrypto/CommonDigest.h>
    #define TK_MD5_DIGEST_LENGTH CC_MD5_DIGEST_LENGTH
    #define TK_MD5_CTX CC_MD5_CTX
    #define TK_MD5_Init CC_MD5_Init
    #define TK_MD5_Update CC_MD5_Update
    #define TK_MD5_Final CC_MD5_Final
#else
    #include <openssl/md5.h>
    #define TK_MD5_DIGEST_LENGTH MD5_DIGEST_LENGTH
    #define TK_MD5_CTX MD5_CTX
    #define TK_MD5_Init MD5_Init
    #define TK_MD5_Update MD5_Update
    #define TK_MD5_Final MD5_Final
#endif

static const int SIZE = 8192;

void file_md5(const char *filename) {
    unsigned char c[TK_MD5_DIGEST_LENGTH];
    char buffer[SIZE];
    TK_MD5_CTX md5_ctx;
    FILE *file = fopen(filename, "rb");

    if (file == NULL) {
        fprintf(stderr, "error: %s can't be opened.\n", filename);
        return;
    }

    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wdeprecated-declarations"
    TK_MD5_Init(&md5_ctx);
    while (!feof(file) && !ferror(file)) {
        size_t bytes = fread(buffer, 1, SIZE, file);
        TK_MD5_Update(&md5_ctx, buffer, bytes);
    }
    TK_MD5_Final(c, &md5_ctx);
    #pragma clang diagnostic pop

    printf("%s: ", filename);

    for (int i = 0; i < TK_MD5_DIGEST_LENGTH; ++i) {
        printf("%02x", c[i]);
    }

    printf("\n");
    fclose(file);
}

void file_content_md5(const char *filename) {
    int error;
    struct zip *archive = zip_open(filename, ZIP_RDONLY, &error);

    if (archive == NULL) {
        fprintf(stderr, "error: %s can't be opened.\n", filename);
        return;
    }

    // Loop over all files in the archive
    zip_int64_t num_entries = zip_get_num_entries(archive, 0);

    for (zip_int64_t i = 0; i < num_entries; i++) {
        struct zip_stat stat;
        zip_stat_init(&stat);
        zip_stat_index(archive, i, 0, &stat);

        // Open the current file in the archive
        struct zip_file *file = zip_fopen_index(archive, i, 0);
        if (file != NULL) {
            // Calculate the MD5 hash of the file
            unsigned char c[TK_MD5_DIGEST_LENGTH];
            char buffer[SIZE];

            #pragma clang diagnostic push
            #pragma clang diagnostic ignored "-Wdeprecated-declarations"
            TK_MD5_CTX md5_ctx;
            TK_MD5_Init(&md5_ctx);

            zip_int64_t bytesRead;
            while ((bytesRead = zip_fread(file, buffer, SIZE)) > 0) {
                TK_MD5_Update(&md5_ctx, buffer, bytesRead);
            }
            TK_MD5_Final(c, &md5_ctx);
            #pragma clang diagnostic pop

            printf("%s/%s: ", filename, stat.name);
            for (int j = 0; j < TK_MD5_DIGEST_LENGTH; ++j) {
                printf("%02x", c[j]);
            }
            printf("\n");

            // Close the current file in the archive
            zip_fclose(file);
        }
    }

    // Close the archive
    zip_close(archive);
}

static magic_t magic_cookie;

void cleanup_magic() {
    magic_close(magic_cookie);
}

magic_t setup_magic() {
    magic_t magic_cookie;

    magic_cookie = magic_open(MAGIC_MIME_TYPE | MAGIC_NO_CHECK_BUILTIN);
    if (magic_cookie == NULL) {
        fprintf(stderr, "error: Unable to initialize magic library\n");
        return false;
    }

    if (magic_load(magic_cookie, NULL) != 0) {
        fprintf(stderr, "error: Cannot load magic database - %s\n", magic_error(magic_cookie));
        cleanup_magic();
        return false;
    }

    return magic_cookie;
}

const char* file_mgc(const char* filename) {
    const char *mgc = magic_file(magic_cookie, filename);
    if (mgc == NULL) {
        fprintf(stderr, "error: Cannot find magic file - %s\n", magic_error(magic_cookie));
        return NULL;
    }
    return mgc;
}

_Bool file_cmp_ext3(const char *str, unsigned long len, const char *ext) {
    return strncmp(str + len - 3, ext, 3) == 0;
}

_Bool file_is_doc(const char *filename) {
    const char *magic_full = file_mgc(filename);
    if (magic_full == NULL) { return false; }

    char *sep = strchr(magic_full, '/');
    if (sep == NULL) { return false; }

    if (strncmp(magic_full, "application", sep - magic_full) == 0) {
        const char *subtype = sep + 1;
        if (
            strcmp(subtype, "epub+zip") == 0 ||
            strcmp(subtype, "msword") == 0 ||
            strcmp(subtype, "octet-stream") == 0 ||
            strcmp(subtype, "postscript") == 0 ||
            strcmp(subtype, "pdf") == 0 ||
            strcmp(subtype, "vnd.ms-htmlhelp") == 0 ||
            strcmp(subtype, "vnd.ms-excel") == 0 ||
            strcmp(subtype, "vnd.openxmlformats-officedocument.wordprocessingml.document") == 0 ||
            strcmp(subtype, "vnd.ms-powerpoint") == 0 ||
            strcmp(subtype, "vnd.oasis.opendocument.presentation") == 0 ||
            strcmp(subtype, "vnd.openxmlformats-officedocument.spreadsheetml.sheet") == 0 ||
            strcmp(subtype, "vnd.oasis.opendocument.text") == 0 ||
            strcmp(subtype, "vnd.oasis.opendocument.spreadsheet") == 0
        ) {
            return true;
        } else {
            unsigned long len = strlen(filename);
            if (file_cmp_ext3(filename, len, "fb3") || file_cmp_ext3(filename, len, "fb2")) {
                return true;
            }
        }
    } else if (strcmp(magic_full, "image/vnd.djvu") == 0 || strcmp(magic_full, "text/rtf") == 0) {
        return true;
    }

    return false;
}

_Bool file_is_archive(const char *filename) {
    const char *magic_full = file_mgc(filename);
    if (magic_full == NULL) { return false; }

    char *sep = strchr(magic_full, '/');
    if (sep == NULL) { return false; }

    if (strncmp(magic_full, "application", sep - magic_full) == 0) {
        const char *subtype = sep + 1;
        if (
            strcmp(subtype, "gzip") == 0 ||
            strcmp(subtype, "x-7z-compressed") == 0 ||
            strcmp(subtype, "x-bzip2") == 0 ||
            strcmp(subtype, "x-iso9660-image") == 0 ||
            strcmp(subtype, "x-lzip") == 0 ||
            strcmp(subtype, "x-mobipocket-ebook") == 0 ||
            strcmp(subtype, "x-ms-reader") == 0 ||
            strcmp(subtype, "x-ms-wim") == 0 ||
            strcmp(subtype, "x-rar") == 0 ||
            strcmp(subtype, "x-ustar") == 0 ||
            strcmp(subtype, "x-xz") == 0 ||
            strcmp(subtype, "zip") == 0 ||
            strcmp(subtype, "zlib") == 0 ||
            strcmp(subtype, "zstd") == 0
        ) {
            return true;
        }
    }

    return false;
}

void enumerate_files(const char *basePath) {
    char path[PATH_MAX];
    struct dirent *dp;
    DIR *dir = opendir(basePath);

    if (!dir) {
        fprintf(stderr, "error: Unable to open dir '%s'. ", basePath);
        perror(NULL);
        return;
    }

    while ((dp = readdir(dir)) != NULL) {
        if (strcmp(dp->d_name, ".") != 0 && strcmp(dp->d_name, "..") != 0) {
            strcpy(path, basePath);
            strcat(path, "/");
            strcat(path, dp->d_name);

            if(dp->d_type == DT_DIR) {
                enumerate_files(path);
            } else if (file_is_doc(path)) {
                file_md5(path);
            } else if (file_is_archive(path)) {
                file_md5(path);
                file_content_md5(path);
            } else {
                #if !NDEBUG
                fprintf(stderr, "info: skip '%s'\n", path);
                #endif
            }
        }
    }

    closedir(dir);
}

int main(int argc, char *argv[]) {
    magic_cookie = setup_magic();
    enumerate_files(argc > 1 ? argv[1] : ".");
    cleanup_magic();
    return 0;
}
