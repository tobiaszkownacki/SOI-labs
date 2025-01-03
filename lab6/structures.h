#pragma once
#include <ctime>

enum FileType {
    file,
    directory
};

struct iNode {
    unsigned int indexNode;
    time_t creationTime;
    time_t modificationTime;
    unsigned int usedBlocksOfData;
    unsigned int sizeOfFile;
    unsigned int directDataBlocks[8];
    unsigned int indirectDataBlocks;
    unsigned int countHardLinks;
    unsigned int parentDirectoryIndexNode;
    FileType fileType;
};