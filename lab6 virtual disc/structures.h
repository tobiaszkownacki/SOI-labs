#pragma once
#include <ctime>

enum FileType {
    file,
    directory
};

std::string fileTypeToString(FileType fileType)
{
    switch (fileType)
    {
        case file:
            return "file";
        case directory:
            return "directory";
        default:
            return "unknown";
    }
}

struct iNode {
    FileType fileType;
    std::time_t creationTime;
    std::time_t modificationTime;
    unsigned int indexNode;
    unsigned int usedBlocksOfData;
    unsigned int sizeOfFile;
    unsigned int directDataBlocks[8];
    unsigned int indirectDataBlocks;
    unsigned int countHardLinks;
    unsigned int parentDirectoryIndexNode;
};