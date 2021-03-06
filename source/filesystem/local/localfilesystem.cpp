#include "localfilesystem.h"

#include "localfile.h"

#include <filesystem>

LocalFileSystem::LocalFileSystem(const std::string mountPoint) :
    FileSystem(),
    mMountPoint(mountPoint) {
}

LocalFileSystem::~LocalFileSystem() {
}

bool LocalFileSystem::setup() {
    return true;
}

void LocalFileSystem::cleanup() {
}

std::string LocalFileSystem::getMountPoint() const {
    return mMountPoint;
}

bool LocalFileSystem::navigate(const std::string path, std::vector<Entry>& list) {
    // clear previous listing
    std::error_code errorCode;
    if (!std::filesystem::exists(path, errorCode) || !std::filesystem::is_directory(path, errorCode)) {
        mError = path;
        return false;
    }

    // add all items to listing
    const auto iterator = std::filesystem::directory_iterator(path, errorCode);
    if (errorCode) {
        mError = errorCode.message();
        return false;
    }

    // ensure empty list
    list.clear();
    for(const auto& p: iterator) {
        // Hide hidden file, maybe an user option
        if (const auto filename = std::string(p.path().filename());
            filename[0] != '.') {

            if (p.is_regular_file() || p.is_directory()) {
                list.push_back((FileSystem::Entry) {
                    .folder = p.is_directory(),
                    .name = filename,
                    .size = p.is_directory() ? 0 : p.file_size()
                });
            }
        }
    }

    return true;
}

std::shared_ptr<File> LocalFileSystem::getFile(const std::string path) const {
    return std::shared_ptr<File>(new LocalFile(path));
}
