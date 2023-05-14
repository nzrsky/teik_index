import os
import magic
import hashlib
from filehash import FileHash
from zipfile import ZipFile
import sys

SIZE = 8192
MIME_APPLICATION_SUBTYPES = {"epub+zip", "msword", "octet-stream", "postscript", "pdf", "vnd.ms-htmlhelp",
                             "vnd.ms-excel", "vnd.openxmlformats-officedocument.wordprocessingml.document",
                             "vnd.ms-powerpoint", "vnd.oasis.opendocument.presentation",
                             "vnd.openxmlformats-officedocument.spreadsheetml.sheet", "vnd.oasis.opendocument.text",
                             "vnd.oasis.opendocument.spreadsheet"}

ARCHIVE_SUBTYPES = {"gzip", "x-7z-compressed", "x-bzip2", "x-iso9660-image", "x-lzip", "x-mobipocket-ebook",
                    "x-ms-reader", "x-ms-wim", "x-rar", "x-ustar", "x-xz", "zip", "zlib", "zstd"}

def eprint(*args, **kwargs):
    print(*args, file=sys.stderr, **kwargs)
    
def file_md5(filename):
    hasher = FileHash('md5')
    md5 = hasher.hash_file(filename)
    print(f"{filename}: {md5}")


def file_content_md5(filename):
    try:
        with ZipFile(filename, 'r') as archive:
            for file in archive.namelist():
                with archive.open(file) as f:
                    hasher = hashlib.md5()
                    for chunk in iter(lambda: f.read(SIZE), b''):
                        hasher.update(chunk)
                    print(f"{filename}/{file}: {hasher.hexdigest()}")
    except:
        eprint(f"error: {filename} is not a valid zip file")


def file_is_doc(filename):
    mime = magic.from_file(filename, mime=True)
    prefix, subtype = mime.split('/')
    if prefix == "application" and (subtype in MIME_APPLICATION_SUBTYPES or filename[-3:] in ['fb3', 'fb2']):
        return True
    if mime in ["image/vnd.djvu", "text/rtf", "text/csv"]:
        return True
    return False


def file_is_archive(filename):
    mime = magic.from_file(filename, mime=True)
    prefix, subtype = mime.split('/')
    if prefix == "application" and subtype in ARCHIVE_SUBTYPES:
        return True
    return False


def enumerate_files(basePath):
    eprint(basePath)
    for root, dirs, files in os.walk(basePath):
        for file in files:
            path = os.path.join(root, file)
            if file == '.DS_Store':
                continue
            elif file_is_doc(path):
                file_md5(path)
            elif file_is_archive(path):
                file_md5(path)
                file_content_md5(path)
            # else:
            #     eprint(f"info: skip '{path}'")


if __name__ == "__main__":
    enumerate_files(sys.argv[1] or '.')
