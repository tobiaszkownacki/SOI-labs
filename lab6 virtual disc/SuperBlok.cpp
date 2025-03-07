#include <string>
#include <fstream>
#include <iostream>
#include "structures.h"

class SuperBlok {
private:
    unsigned int numberOfFiles;
    std::time_t lastModification;
    unsigned int totalBytesSize;
    unsigned int usedBytesSize;
    unsigned int iNodeSize;
    unsigned int numberOfINodes;
    unsigned int dataBlockSize;
    unsigned int numberOfDataBlocks;
    unsigned int firstINodeOffset;
    unsigned int iNodeBitmapOffset;
    unsigned int dataBlockBitmapOffset;
    unsigned int firstDataBlockOffset;
public:

    SuperBlok(unsigned int totalBytes, unsigned int dataBlockSize, unsigned int numberOfINodes, unsigned int numberOfDataBlocks)
    {
        this->numberOfFiles = 0;
        this->lastModification = std::time(nullptr);
        this->totalBytesSize = totalBytes;
        this->iNodeSize = sizeof(iNode);
        this->usedBytesSize = sizeof(SuperBlok) + numberOfINodes * iNodeSize + numberOfINodes + numberOfDataBlocks;
        this->numberOfINodes = numberOfINodes;
        this->dataBlockSize = dataBlockSize;
        this->numberOfDataBlocks = numberOfDataBlocks;
        this->firstINodeOffset = sizeof(SuperBlok);
        this->iNodeBitmapOffset = this->firstINodeOffset + numberOfINodes * iNodeSize;
        this->dataBlockBitmapOffset = this->iNodeBitmapOffset + numberOfINodes;
        this->firstDataBlockOffset = this->dataBlockBitmapOffset + numberOfDataBlocks;
    }

    SuperBlok(std::ifstream& disk)
    {
        if (!disk.read(reinterpret_cast<char*>(this), sizeof(SuperBlok))) {
            throw std::runtime_error("Error reading SuperBlok");
        }
    }

    SuperBlok() = default;

    unsigned int get_numberOfFiles() const { return numberOfFiles; }
    std::time_t get_lastModification() const { return lastModification; }
    unsigned int get_totalBytesSize() const { return totalBytesSize; }
    unsigned int get_usedBytesSize() const { return usedBytesSize; }
    unsigned int get_iNodeSize() const { return iNodeSize; }
    unsigned int get_numberOfINodes() const { return numberOfINodes; }
    unsigned int get_dataBlockSize() const { return dataBlockSize; }
    unsigned int get_numberOfDataBlocks() const { return numberOfDataBlocks; }
    unsigned int get_firstINodeOffset() const { return firstINodeOffset; }
    unsigned int get_iNodeBitmapOffset() const { return iNodeBitmapOffset; }
    unsigned int get_dataBlockBitmapOffset() const { return dataBlockBitmapOffset; }
    unsigned int get_firstDataBlockOffset() const { return firstDataBlockOffset; }

    void set_numberOfFiles(unsigned int numberOfFiles) { this->numberOfFiles = numberOfFiles; }
    void set_lastModification() { lastModification = std::time(nullptr); }
    void set_usedBytesSize(unsigned int usedBytesSize) { this->usedBytesSize = usedBytesSize; }


    void write(std::ofstream& disk) const
    {
        if (!disk.write(reinterpret_cast<const char*>(this), sizeof(SuperBlok))) {
            throw std::runtime_error("Error writing SuperBlok");
        }
    }

    void printSuperBlock() {

        std::cout << "Number of files: " << numberOfFiles << std::endl;
        std::cout << "Last modification: " << std::ctime(&lastModification);
        std::cout << "Total bytes size: " << totalBytesSize << std::endl;
        std::cout << "Used bytes size: " << usedBytesSize << std::endl;
        std::cout << "iNode size: " << iNodeSize << std::endl;
        std::cout << "Number of iNodes: " << numberOfINodes << std::endl;
        std::cout << "Data block size: " << dataBlockSize << std::endl;
        std::cout << "Number of data blocks: " << numberOfDataBlocks << std::endl;
        std::cout << "First iNode offset: " << firstINodeOffset << std::endl;
        std::cout << "iNode bitmap offset: " << iNodeBitmapOffset << std::endl;
        std::cout << "Data block bitmap offset: " << dataBlockBitmapOffset << std::endl;
        std::cout << "First data block offset: " << firstDataBlockOffset << std::endl;
    }

};
