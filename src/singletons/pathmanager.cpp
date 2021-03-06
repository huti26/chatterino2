#include "singletons/pathmanager.hpp"

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QStandardPaths>

namespace chatterino {
namespace singletons {

PathManager::PathManager(int argc, char **argv)
{
    // hash of app path
    this->appPathHash = QCryptographicHash::hash(QCoreApplication::applicationFilePath().toUtf8(),
                                                 QCryptographicHash::Sha224)
                            .toBase64()
                            .mid(0, 32)
                            .replace("+", "-")
                            .replace("/", "x");

    // Options
    bool portable = false;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "portable") == 0) {
            portable = true;
        }
    }

    if (QFileInfo::exists(QCoreApplication::applicationDirPath() + "/portable")) {
        portable = true;
    }

    // Root path = %APPDATA%/Chatterino or the folder that the executable resides in
    QString rootPath;
    if (portable) {
        rootPath.append(QCoreApplication::applicationDirPath());
    } else {
        // Get settings path
        rootPath.append(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
        if (rootPath.isEmpty()) {
            throw std::runtime_error("Error finding writable location for settings");
        }
    }

    this->settingsFolderPath = rootPath;

    if (!QDir().mkpath(this->settingsFolderPath)) {
        throw std::runtime_error("Error creating settings folder");
    }

    this->customFolderPath = rootPath + "/Custom";

    if (!QDir().mkpath(this->customFolderPath)) {
        throw std::runtime_error("Error creating custom folder");
    }

    this->cacheFolderPath = rootPath + "/Cache";

    if (!QDir().mkpath(this->cacheFolderPath)) {
        throw std::runtime_error("Error creating cache folder");
    }

    this->logsFolderPath = rootPath + "/Logs";

    if (!QDir().mkpath(this->logsFolderPath)) {
        throw std::runtime_error("Error creating logs folder");
    }

    this->channelsLogsFolderPath = this->logsFolderPath + "/Channels";

    if (!QDir().mkpath(this->channelsLogsFolderPath)) {
        throw std::runtime_error("Error creating channel logs folder");
    }

    this->whispersLogsFolderPath = this->logsFolderPath + "/Whispers";

    if (!QDir().mkpath(this->whispersLogsFolderPath)) {
        throw std::runtime_error("Error creating whisper logs folder");
    }

    this->mentionsLogsFolderPath = this->logsFolderPath + "/Mentions";

    if (!QDir().mkpath(this->mentionsLogsFolderPath)) {
        throw std::runtime_error("Error creating mentions logs folder");
    }
}

bool PathManager::createFolder(const QString &folderPath)
{
    return QDir().mkpath(folderPath);
}

}  // namespace singletons
}  // namespace chatterino
