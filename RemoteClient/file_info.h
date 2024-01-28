#pragma once
#include <windows.h>
#include <string>
#include <vector>
#include <io.h>
#include <optional>


struct FileInfo {
    std::string name;
    bool is_dir;
};

struct DriveInfo {
    char drive_letter;
    std::vector<FileInfo> files;
};

static std::string CStringToStdString(const CString& cString) {
    CT2CA pszConvertedAnsiString(cString);
    std::string stdString(pszConvertedAnsiString);
    return stdString;
}

static std::optional<std::vector<FileInfo>> GatherDirInfo(const std::string& directory) {
    std::vector<FileInfo> files;

    // _finddata_t is used with _findfirst and _findnext
    struct _finddata_t file_info;
    intptr_t handle;

    // Prepare the directory search pattern
    std::string search_pattern = directory + "\\*";

    // Start the file search
    handle = _findfirst(search_pattern.c_str(), &file_info);

    if (handle == -1L) 
        return std::nullopt;

    do {
        if (strcmp(file_info.name, ".") != 0 && strcmp(file_info.name, "..") != 0) {
            FileInfo fileInfo;
            fileInfo.name = file_info.name;
            fileInfo.is_dir = (file_info.attrib & _A_SUBDIR) != 0;
            files.push_back(fileInfo);
        }
    } while (_findnext(handle, &file_info) == 0);

    _findclose(handle);
    return files;
}


static std::vector<DriveInfo> GetDrivesInfo() {
    std::vector<DriveInfo> drives;

    DWORD driveMask = GetLogicalDrives();
    for (char letter = 'A'; letter <= 'Z'; letter++) {
        if (!(driveMask & 1)) {
            driveMask >>= 1;
            continue;
        }

        DriveInfo drive;
        drive.drive_letter = letter;

        drives.push_back(drive);
        driveMask >>= 1;
    }

    /*
    for (auto& drive : drives) {
        auto files = GatherDirInfo(std::string("") + drive.drive_letter + ":");
        if (files.has_value())
			drive.files = files.value();
    }
    */

    return drives;
}

static std::pair<size_t, std::shared_ptr<BYTE[]>> SerializeDirInfo(const std::vector<FileInfo>& files) {
    std::string data;
    for (const auto& file : files) {
        data += file.name + (file.is_dir ? "D" : "F") + ";";
    }

    size_t buffer_size = data.size();
    std::shared_ptr<BYTE[]> buffer(new BYTE[buffer_size]);
    memcpy(buffer.get(), data.c_str(), buffer_size);

    return { buffer_size, buffer };
}

static std::vector<FileInfo> DeserializeDirInfo(size_t buffer_size, const std::shared_ptr<BYTE[]>& buffer) {
    std::vector<FileInfo> files;

    // Convert BYTE[] to std::string for easier processing
    std::string data(reinterpret_cast<char*>(buffer.get()), buffer_size);

    size_t start = 0;
    size_t end = data.find(';');
    while (end != std::string::npos) {
        std::string fileEntry = data.substr(start, end - start);

        FileInfo fileInfo;
        if (!fileEntry.empty()) {
            assert(fileEntry.back() == 'D' || fileEntry.back() == 'F');
            fileInfo.name = fileEntry.substr(0, fileEntry.size() - 1);
            fileInfo.is_dir = fileEntry.back() == 'D';
            files.push_back(fileInfo);
        }

        start = end + 1;
        end = data.find(';', start);
    }

    return files;
}

static std::pair<size_t, std::shared_ptr<BYTE[]>> SerializeDriveInfo(const std::vector<DriveInfo>& drives) {
    std::string data;
    for (const auto& drive : drives) {
        data += drive.drive_letter + std::string(":\\");
        for (const auto& file : drive.files) {
            data += file.name + (file.is_dir ? "D" : "F") + ";";
        }
        data += "|"; // Separator between drives
    }

    size_t buffer_size = data.size();
    std::shared_ptr<BYTE[]> buffer(new BYTE[buffer_size]);
    memcpy(buffer.get(), data.c_str(), buffer_size);

    return { buffer_size, buffer };
}


static std::vector<DriveInfo> DeserializeDriveInfo(size_t data_size, std::shared_ptr<BYTE[]> data) {
    std::vector<DriveInfo> drives;

    // Convert BYTE[] to std::string
    std::string strData(reinterpret_cast<char*>(data.get()), data_size);

    size_t start = 0;
    while (start < strData.size()) {
        size_t end = strData.find('|', start);
        std::string driveBlock = strData.substr(start, end - start);

        DriveInfo driveInfo;
        driveInfo.drive_letter = driveBlock[0];

        size_t fileStart = 3; // Starting after "X:\\"
        while (fileStart < driveBlock.size()) {
            size_t fileEnd = driveBlock.find(';', fileStart);
            std::string fileBlock = driveBlock.substr(fileStart, fileEnd - fileStart);

            FileInfo fileInfo;
            fileInfo.name = fileBlock.substr(0, fileBlock.size() - 1);
            assert(fileBlock.back() == 'D' || fileBlock.back() == 'F');
            fileInfo.is_dir = fileBlock.back() == 'D';

            driveInfo.files.push_back(fileInfo);
            fileStart = fileEnd + 1;
        }

        drives.push_back(driveInfo);
        start = end + 1;
    }

    return drives;
}
