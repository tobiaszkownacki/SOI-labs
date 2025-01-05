#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <iomanip>
#include "SuperBlok.cpp"
#include "iNodeArray.cpp"
#include "bitMap.cpp"


class VirtualDisk {
private:
    std::string diskPath;
    SuperBlok superBlok;
    INodeArray iNodeArray;
    bitMap iNodeBitmap;
    bitMap dataBlockBitmap;
    const unsigned int DataBlockSize = 2048u;

    void writeEmptyDataBlocks(std::ofstream& disk, unsigned int numberOfDataBlocks) {
        std::vector<char> emptyBlock(DataBlockSize, 0);
        for (unsigned int i = 0; i < numberOfDataBlocks; i++) {
            if (!disk.write(emptyBlock.data(), DataBlockSize)) {
                throw std::runtime_error("Error writing empty data block");
            }
        }
    }

    void writeDirectoryDataBlock(unsigned int blockNumber, const std::map<std::string, unsigned int>& directoryEntries) {
        std::ofstream disk(diskPath, std::ios::binary | std::ios::in | std::ios::out);
        if (!disk.is_open()) {
            throw std::runtime_error("Error opening disk file");
        }

        disk.seekp(superBlok.get_firstDataBlockOffset() + blockNumber * DataBlockSize, std::ios::beg);
        size_t mapSize = directoryEntries.size();
        disk.write(reinterpret_cast<const char*>(&mapSize), sizeof(mapSize));
        for (const auto& [fileName, iNodeNumber] : directoryEntries) {
            size_t fileNameSize = fileName.size();
            disk.write(reinterpret_cast<const char*>(&fileNameSize), sizeof(fileNameSize));
            disk.write(fileName.c_str(), fileNameSize);
            disk.write(reinterpret_cast<const char*>(&iNodeNumber), sizeof(iNodeNumber));
        }
        disk.close();
    }

    std::map<std::string, unsigned int> readDirectoryDataBlock(unsigned int blockNumber) {
        std::map<std::string, unsigned int> directoryEntries;
        std::ifstream disk(diskPath, std::ios::binary);
        if (!disk.is_open()) {
            throw std::runtime_error("Error opening disk file");
        }

        disk.seekg(superBlok.get_firstDataBlockOffset() + blockNumber * DataBlockSize, std::ios::beg);
        size_t mapSize;
        disk.read(reinterpret_cast<char*>(&mapSize), sizeof(mapSize));
        for (size_t i = 0; i < mapSize; ++i) {
            size_t fileNameSize;
            disk.read(reinterpret_cast<char*>(&fileNameSize), sizeof(fileNameSize));
            std::string fileName(fileNameSize, 0);
            disk.read(fileName.data(), fileNameSize);
            unsigned int iNodeNumber;
            disk.read(reinterpret_cast<char*>(&iNodeNumber), sizeof(iNodeNumber));
            directoryEntries[fileName] = iNodeNumber;
        }
        disk.close();
        return directoryEntries;
    }

    void updateMetaData(int fileSize, unsigned int numberofNewFiles = 1) {
        std::ofstream disk(diskPath, std::ios::binary | std::ios::in | std::ios::out);
        if (!disk.is_open()) {
            throw std::runtime_error("Error opening disk file");
        }

        superBlok.set_numberOfFiles(superBlok.get_numberOfFiles() + numberofNewFiles);
        superBlok.set_lastModification();
        superBlok.set_usedBytesSize(superBlok.get_usedBytesSize() + fileSize);
        superBlok.write(disk);

        iNodeArray.write(disk);
        iNodeBitmap.write(disk);
        dataBlockBitmap.write(disk);
        disk.close();
    }

    void updateParentsDirectory(unsigned int fileSize) {
        unsigned int parentINodeNumber = currentINodeNumber;
        while(parentINodeNumber != 0)
        {
            iNode& parentINode = iNodeArray[parentINodeNumber];
            parentINode.sizeOfFile += fileSize;
            parentINodeNumber = parentINode.parentDirectoryIndexNode;
            parentINode.modificationTime = std::time(nullptr);
        }

        iNode& rootINode = iNodeArray[0];
        rootINode.sizeOfFile += fileSize;
        rootINode.modificationTime = std::time(nullptr);
    }

    unsigned int sumDataBlocksInDescendantsRecursive(unsigned int iNodeNumber) {
        iNode iNode = iNodeArray[iNodeNumber];
        if(iNode.fileType == FileType::file) {
            return iNode.usedBlocksOfData;
        }
        else {
            unsigned int totalSize = 1;
            std::map<std::string, unsigned int> directoryEntries = readDirectoryDataBlock(iNode.directDataBlocks[0]);
            for (const auto& [fileName, iNodeNumber] : directoryEntries) {
                totalSize += sumDataBlocksInDescendantsRecursive(iNodeNumber);
            }
            return totalSize;
        }

    }

    unsigned int sumDataBlocksInDescendants(unsigned int iNodeNumber) {
        unsigned int usedBlocksOfData = 0;
        iNode iNode = iNodeArray[iNodeNumber];
        std::map<std::string, unsigned int> directoryEntries = readDirectoryDataBlock(iNode.directDataBlocks[0]);
        for (const auto& [fileName, iNodeChildNumber] : directoryEntries) {
            usedBlocksOfData += sumDataBlocksInDescendantsRecursive(iNodeChildNumber);
        }
        return usedBlocksOfData;
    }


public:
    unsigned int currentINodeNumber = 0;

    VirtualDisk(unsigned int totalBytes, const std::string& diskPath) {
        this->diskPath = diskPath;
        std::ifstream disk(diskPath, std::ios::binary);
        if (disk.is_open()) {
            superBlok = SuperBlok(disk);
            iNodeArray = INodeArray(disk, superBlok.get_numberOfINodes());
            iNodeBitmap = bitMap(disk, superBlok.get_numberOfINodes());
            dataBlockBitmap = bitMap(disk, superBlok.get_numberOfDataBlocks());
            std::cout << "Wczytano dysk wirtualny z pliku " << diskPath << std::endl;
        } else {
            std::ofstream disk(diskPath, std::ios::binary);
            unsigned int numberOfINodes = 100;
            unsigned int numberOfDataBlocks = (totalBytes - sizeof(SuperBlok) - numberOfINodes * sizeof(iNode) - numberOfINodes) / DataBlockSize;
            numberOfDataBlocks -= 1; // Ostatni blok na bitmapę bloków danych
            unsigned int revisedTotalBytes = sizeof(SuperBlok) + numberOfINodes * sizeof(iNode) + numberOfINodes + numberOfDataBlocks * (DataBlockSize + 1);

            superBlok = SuperBlok(revisedTotalBytes, DataBlockSize, numberOfINodes, numberOfDataBlocks);
            iNodeArray = INodeArray(superBlok.get_numberOfINodes());
            iNodeBitmap = bitMap(superBlok.get_numberOfINodes());
            dataBlockBitmap = bitMap(superBlok.get_numberOfDataBlocks());

            superBlok.write(disk);
            iNodeArray.write(disk);
            iNodeBitmap.write(disk);
            dataBlockBitmap.write(disk);
            writeEmptyDataBlocks(disk, numberOfDataBlocks);
            createDirectory("home");

        }

        disk.close();
    }

    void createDirectory(const std::string& directoryName) {
        unsigned int iNodeNumber = iNodeBitmap.findFirstFreeBlock();
        iNodeBitmap.set(iNodeNumber, true);
        iNode &iNode = iNodeArray[iNodeNumber];
        iNode.creationTime = std::time(nullptr);
        iNode.modificationTime = std::time(nullptr);
        iNode.usedBlocksOfData = 1;
        iNode.sizeOfFile = DataBlockSize;
        iNode.directDataBlocks[0] = dataBlockBitmap.findFirstFreeBlock();
        dataBlockBitmap.set(iNode.directDataBlocks[0], true);
        iNode.countHardLinks = 1;
        iNode.parentDirectoryIndexNode = currentINodeNumber;
        iNode.fileType = FileType::directory;

        // update current directory
        // don't update current directory if we create home directory
        if(currentINodeNumber != 0 || iNodeBitmap.countFree() != superBlok.get_numberOfINodes() - 1)
        {
            std::map<std::string, unsigned int> directoryEntries = readDirectoryDataBlock(iNodeArray[currentINodeNumber].directDataBlocks[0]);
            directoryEntries[directoryName] = iNodeNumber;
            writeDirectoryDataBlock(iNodeArray[currentINodeNumber].directDataBlocks[0], directoryEntries);
            updateParentsDirectory(iNode.sizeOfFile);
        }

        updateMetaData(iNode.sizeOfFile);

    }

    void copyFileFromSystemDisk(std::string sourcePath, std::string fileName) {
        std::ifstream sourceFile(sourcePath, std::ios::binary);

        if(!sourceFile.is_open()){
            throw std::runtime_error("Error opening source file");
        }

        unsigned int iNodeNumber = iNodeBitmap.findFirstFreeBlock();
        iNodeBitmap.set(iNodeNumber, true);
        iNode &iNode = iNodeArray[iNodeNumber];
        iNode.creationTime = std::time(nullptr);
        iNode.modificationTime = std::time(nullptr);

        sourceFile.seekg(0, std::ios::end);
        iNode.sizeOfFile = sourceFile.tellg();
        sourceFile.seekg(0, std::ios::beg);

        iNode.usedBlocksOfData = (iNode.sizeOfFile + DataBlockSize - 1) / DataBlockSize;
        iNode.countHardLinks = 1;
        iNode.parentDirectoryIndexNode = currentINodeNumber;
        iNode.fileType = FileType::file;

        char buffer[DataBlockSize];
        unsigned int blockIndex = 0;

        std::ofstream disk(diskPath, std::ios::binary | std::ios::in | std::ios::out);
        if (!disk.is_open()) {
            throw std::runtime_error("Error opening disk file");
        }

        // !!Warning!! - Works if only file < 8 * DataBlockSize
        while(sourceFile.read(buffer, DataBlockSize) || sourceFile.gcount() > 0) {
            unsigned int dataBlockNumber = dataBlockBitmap.findFirstFreeBlock();
            dataBlockBitmap.set(dataBlockNumber, true);
            iNode.directDataBlocks[blockIndex] = dataBlockNumber;
            ++blockIndex;

            disk.seekp(superBlok.get_firstDataBlockOffset() + dataBlockNumber * DataBlockSize, std::ios::beg);

            if(sourceFile.gcount() > 0) {
                disk.write(buffer, sourceFile.gcount());
            }
            else {
                disk.write(buffer, DataBlockSize);
            }
        }
        disk.close();

        // update current directory
        std::map<std::string, unsigned int> directoryEntries = readDirectoryDataBlock(iNodeArray[currentINodeNumber].directDataBlocks[0]);
        directoryEntries[fileName] = iNodeNumber;
        writeDirectoryDataBlock(iNodeArray[currentINodeNumber].directDataBlocks[0], directoryEntries);

        updateParentsDirectory(iNode.sizeOfFile);
        updateMetaData(iNode.sizeOfFile);
    }

    void deleteDirectoryRecursively(const std::string& directoryName) {

        std::map<std::string, unsigned int> directoryEntries = readDirectoryDataBlock(iNodeArray[currentINodeNumber].directDataBlocks[0]);
        if(directoryEntries.find(directoryName) == directoryEntries.end()) {
            std::cerr << "Directory not found" << std::endl;
            return;
        }

        unsigned int directoryINodeNumber = directoryEntries[directoryName];
        iNode& directoryINode = iNodeArray[directoryINodeNumber];

        if(directoryINode.fileType != FileType::directory) {
            std::cerr << "Not a directory" << std::endl;
            return;
        }

        directoryINode.countHardLinks -= 1;
        if(directoryINode.countHardLinks == 0) {

            std::map<std::string, unsigned int> childEntriesToDelete = readDirectoryDataBlock(directoryINode.directDataBlocks[0]);
            for (const auto& [fileName, iNodeNumber] : childEntriesToDelete) {
                iNode& iNode = iNodeArray[iNodeNumber];
                if(iNode.fileType == FileType::directory) {
                    deleteDirectoryRecursively(fileName);
                }
                else {
                    deleteFile(fileName, directoryINodeNumber);
                }
            }

            dataBlockBitmap.set(directoryINode.directDataBlocks[0], false);
            iNodeBitmap.set(directoryINodeNumber, false);
        }

        directoryEntries.erase(directoryName);
        writeDirectoryDataBlock(iNodeArray[currentINodeNumber].directDataBlocks[0], directoryEntries);

        if(directoryINode.countHardLinks == 0) {
            updateParentsDirectory(-directoryINode.sizeOfFile);
            updateMetaData(-directoryINode.sizeOfFile, -1);
        }
        else {
            updateParentsDirectory(0);
            updateMetaData(0, -1);
        }

    }

    void copyFileToSystemDisk(std::string destinationPath, std::string fileName) {
        std::map<std::string, unsigned int> directoryEntries = readDirectoryDataBlock(iNodeArray[currentINodeNumber].directDataBlocks[0]);
        if(directoryEntries.find(fileName) == directoryEntries.end()) {
            std::cerr << "File not found" << std::endl;
            return;
        }

        std::ofstream destinationFile(destinationPath + "/" + fileName, std::ios::binary);
        if(!destinationFile.is_open()){
            throw std::runtime_error("Error opening destination file");
        }

        iNode iNode = iNodeArray[directoryEntries[fileName]];

        std::ifstream disk(diskPath, std::ios::binary);
        if (!disk.is_open()) {
            throw std::runtime_error("Error opening disk file");
        }

        char buffer[DataBlockSize];
        for (unsigned int i = 0; i < iNode.usedBlocksOfData; ++i) {
            disk.seekg(superBlok.get_firstDataBlockOffset() + iNode.directDataBlocks[i] * DataBlockSize, std::ios::beg);
            disk.read(buffer, DataBlockSize);
            if(i == iNode.usedBlocksOfData - 1) {
                destinationFile.write(buffer, iNode.sizeOfFile % DataBlockSize);
            }
            else {
                destinationFile.write(buffer, DataBlockSize);
            }
        }
        disk.close();
        destinationFile.close();
    }

    void printDirectory() {
        std::map<std::string, unsigned int> directoryEntries = readDirectoryDataBlock(iNodeArray[currentINodeNumber].directDataBlocks[0]);

        const int colWidthName = 14;
        const int colWidthType = 12;
        const int colWidthSize = 23;
        const int colWidthTime = 27;

        std::cout << "Directory contents:" << std::endl;
        std::cout << std::left
                << std::setw(colWidthName) << "File Name"
                << std::setw(colWidthType) << "File Type"
                << std::setw(colWidthSize) << "Size in Bytes"
                << std::setw(colWidthTime) << "Creation Time"
                << "Last Modification Time"
                << std::endl;


        std::cout << std::string(colWidthName + colWidthType + colWidthSize + 2 * colWidthTime, '-') << std::endl;

        unsigned int totalSizeLevel0 = 0;
        unsigned int usedBlocksOfDataLevel0 = 0;
        for (const auto& [fileName, iNodeNumber] : directoryEntries) {

            iNode iNode = iNodeArray[iNodeNumber];
            if(iNode.fileType == FileType::file) {
                totalSizeLevel0 += iNode.sizeOfFile;
            }
            else {
                // Level 0 directory size is the size of one data block
                totalSizeLevel0 += DataBlockSize;
            }

            usedBlocksOfDataLevel0 += iNode.usedBlocksOfData;
            // delete '\n' from ctime
            std::string creationTimeStr = std::ctime(&iNode.creationTime);
            creationTimeStr.pop_back();

            std::string modificationTimeStr = std::ctime(&iNode.modificationTime);

            std::cout << std::left
                    << std::setw(colWidthName) << fileName
                    << std::setw(colWidthType) << fileTypeToString(iNode.fileType)
                    << std::setw(colWidthSize) << iNode.sizeOfFile
                    << std::setw(colWidthTime) << creationTimeStr
                    << modificationTimeStr;
        }
        std::cout << std::string(colWidthName + colWidthType + colWidthSize + 2 * colWidthTime, '-') << std::endl;

        unsigned int logicalTotalSize = sumDataBlocksInDescendants(currentINodeNumber) * DataBlockSize;
        unsigned int logicalFreeSpace = dataBlockBitmap.countFree() * DataBlockSize;
        // subtract size of current directory
        unsigned int physicalSizeSubdirectories = iNodeArray[currentINodeNumber].sizeOfFile - DataBlockSize;
        std::cout << std::endl;
        std::cout << "Physical size of files in the directory: " << totalSizeLevel0 << " B" << std::endl;
        std::cout << "Physical size of files including subdirectories: " << physicalSizeSubdirectories << " B" << std::endl << std::endl;
        std::cout << "Logical size of files in the directory: " << usedBlocksOfDataLevel0 * DataBlockSize << " B" << std::endl;
        std::cout << "Logical size of files including subdirectories: " << logicalTotalSize << " B" << std::endl << std::endl;
        std::cout << "Physical free space on the disk: " << superBlok.get_totalBytesSize() - superBlok.get_usedBytesSize() << " B" << std::endl;
        std::cout << "Logical free space on the disk: " << logicalFreeSpace << " B" << std::endl;

    }

    void createHardLink(std::string& sourcePath, std::string& newFileName) {
        // we accept sourcePath line: home/folder1/podfolder2 and also home/folder1/plik1
        // !!Warning!! - new hardlink is not counted in the size of the current directory

        std::vector<std::string> partsOfPath;
        std::string part;
        std::istringstream pathStream(sourcePath);
        while(std::getline(pathStream, part, '/')) {
            partsOfPath.push_back(part);
        }


        unsigned int iNodeNumber = 0;
        for(std::size_t i = 1; i < partsOfPath.size(); ++i) {

            std::map<std::string, unsigned int> directoryEntries = readDirectoryDataBlock(iNodeArray[iNodeNumber].directDataBlocks[0]);
            if(directoryEntries.find(partsOfPath[i]) == directoryEntries.end()) {
                std::cerr << "File not found" << std::endl;
                return;
            }
            iNodeNumber = directoryEntries[partsOfPath[i]];
        }

        iNode &sourceINode = iNodeArray[iNodeNumber];
        sourceINode.countHardLinks += 1;
        std::map<std::string, unsigned int> directoryEntries = readDirectoryDataBlock(iNodeArray[currentINodeNumber].directDataBlocks[0]);
        directoryEntries[newFileName] = iNodeNumber;
        writeDirectoryDataBlock(iNodeArray[currentINodeNumber].directDataBlocks[0], directoryEntries);

        updateMetaData(0, 1);
        updateParentsDirectory(0);

    }

    void deleteFile(const std::string& fileName, unsigned int baseINodeNumber) {
        std::map<std::string, unsigned int> directoryEntries = readDirectoryDataBlock(iNodeArray[baseINodeNumber].directDataBlocks[0]);
        if(directoryEntries.find(fileName) == directoryEntries.end()) {
            std::cerr << "File not found" << std::endl;
            return;
        }
        iNode& iNode = iNodeArray[directoryEntries[fileName]];
        if(iNode.fileType == FileType::directory) {
            std::cerr << "Delete directory is a different switch option" << std::endl;
            return;
        }

        iNode.countHardLinks -= 1;
        if(iNode.countHardLinks == 0) {
            for(unsigned int i = 0; i < iNode.usedBlocksOfData; ++i) {
                dataBlockBitmap.set(iNode.directDataBlocks[i], false);
            }
            iNodeBitmap.set(iNode.indexNode, false);
        }
        directoryEntries.erase(fileName);
        writeDirectoryDataBlock(iNodeArray[baseINodeNumber].directDataBlocks[0], directoryEntries);

        if(iNode.countHardLinks == 0) {
            updateParentsDirectory(-iNode.sizeOfFile);
            updateMetaData(-iNode.sizeOfFile, -1);
        }
        else {
            updateParentsDirectory(0);
            updateMetaData(0, -1);
        }
    }

    void addNBytes(const std::string& fileName, unsigned int bytes) {
        std::map<std::string, unsigned int> directoryEntries = readDirectoryDataBlock(iNodeArray[currentINodeNumber].directDataBlocks[0]);
        if(directoryEntries.find(fileName) == directoryEntries.end()) {
            std::cerr << "File not found" << std::endl;
            return;
        }

        iNode& iNode = iNodeArray[directoryEntries[fileName]];
        unsigned int oldSize = iNode.sizeOfFile;
        unsigned int newBlockIndex = iNode.usedBlocksOfData;
        iNode.sizeOfFile += bytes;
        int newUsedBlocksOfData = (iNode.sizeOfFile / DataBlockSize) - (oldSize / DataBlockSize);

        for(int i = 0; i < newUsedBlocksOfData; ++i) {
            unsigned int dataBlockNumber = dataBlockBitmap.findFirstFreeBlock();
            dataBlockBitmap.set(dataBlockNumber, true);
            iNode.directDataBlocks[newBlockIndex + i] = dataBlockNumber;
        }

        iNode.usedBlocksOfData += newUsedBlocksOfData;
        updateParentsDirectory(bytes);
        updateMetaData(bytes, 0);
    }

    void takeAwayNBytes(const std::string& fileName, unsigned int bytes) {
        std::map<std::string, unsigned int> directoryEntries = readDirectoryDataBlock(iNodeArray[currentINodeNumber].directDataBlocks[0]);
        if(directoryEntries.find(fileName) == directoryEntries.end()) {
            std::cerr << "File not found" << std::endl;
            return;
        }

        iNode& iNode = iNodeArray[directoryEntries[fileName]];
        unsigned int oldSize = iNode.sizeOfFile;
        iNode.sizeOfFile -= bytes;
        int BlocksToFree = (oldSize / DataBlockSize) - (iNode.sizeOfFile / DataBlockSize);
        for(int i = 0; i < BlocksToFree; ++i) {
            dataBlockBitmap.set(iNode.directDataBlocks[iNode.usedBlocksOfData - 1 - i], false);
        }

        iNode.usedBlocksOfData -= BlocksToFree;
        updateParentsDirectory(-bytes);
        updateMetaData(-bytes, 0);
    }

    void showDiskUsage() {
        std::cout << iNodeBitmap.countFree() << std::endl << dataBlockBitmap.countFree() << std::endl;
        unsigned int totalSize = superBlok.get_totalBytesSize();
        unsigned int physicalUsedSize = iNodeArray[0].sizeOfFile;
        unsigned int metaDataSize = superBlok.get_usedBytesSize() - iNodeArray[0].sizeOfFile;
        unsigned int logicalUsedSize = (sumDataBlocksInDescendants(0) + 1)* (DataBlockSize);
        unsigned int physicalFreeSize = totalSize - physicalUsedSize;
        unsigned int logicalFreeSize = dataBlockBitmap.countFree() * DataBlockSize;

        std::cout << "Total disk size: " << totalSize << " B" << std::endl;
        std::cout << "Metadata size: " << metaDataSize << " B" << std::endl;
        std::cout << "Physical disk usage: " << physicalUsedSize << " B" << std::endl;
        std::cout << "Logical disk usage: " << logicalUsedSize << " B" << std::endl;
        std::cout << "Physical free space: " << physicalFreeSize << " B" << std::endl;
        std::cout << "Logical free space: " << logicalFreeSize << " B" << std::endl;

    }

    void changeDirectory(const std::string& directoryName) {
        std::map<std::string, unsigned int> directoryEntries = readDirectoryDataBlock(iNodeArray[currentINodeNumber].directDataBlocks[0]);
        if(directoryEntries.find(directoryName) == directoryEntries.end()) {
            std::cerr << "Directory not found" << std::endl;
        }
        if(iNodeArray[directoryEntries[directoryName]].fileType != FileType::directory) {
            std::cerr << "Not a directory" << std::endl;
        }
        currentINodeNumber = directoryEntries[directoryName];
        std::cout << "Changed to directory: " << directoryName << std::endl;
    }

    void changeToParentDirectory() {
        if(currentINodeNumber == 0) {
            std::cerr << "Already in home directory" << std::endl;
        }
        currentINodeNumber = iNodeArray[currentINodeNumber].parentDirectoryIndexNode;
    }

};


int main(int argc, char *argv[]) {

    //VirtualDisk virtualDisk(4857600, "disk.bin");
    if(argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <disk_path>" << std::endl;
        return 1;
    }

    std::string diskPath = argv[1];
    std::ifstream disk(diskPath, std::ios::binary);
    unsigned int totalBytes = 0;
    if(!disk.is_open()) {
        std::cout << "Disk not found. Do you want to create a new one? [y/n]" << std::endl;
        char choice;
        std::cin >> choice;
        if(choice != 'y') {
            return 0;
        }
        std::cout << "Enter the total size of the disk in bytes: ";
        std::cin >> totalBytes;
    }
    VirtualDisk virtualDisk(totalBytes, diskPath);
    disk.close();


    while(true) {
        std::cout<<"------------------------------------"<<std::endl;
        std::cout << "1. Copy a file from the system disk to the virtual disk" << std::endl;
        std::cout << "2. Create a directory" << std::endl;
        std::cout << "3. Delete a directory from the virtual disk" << std::endl;
        std::cout << "4. Copy a file from the virtual disk to the system disk" << std::endl;
        std::cout << "5. Display information about the current directory" << std::endl;
        std::cout << "6. Create a hard link to a file or directory" << std::endl;
        std::cout << "7. Delete a file from the virtual disk" << std::endl;
        std::cout << "8. Add N bytes to a file with the specified name" << std::endl;
        std::cout << "9. Take away N bytes from a file with the specified name." << std::endl;
        std::cout << "10. Display disk usage information" << std::endl;
        std::cout << "11. Change to the selected directory" << std::endl;
        std::cout << "12. Go to the parent directory" << std::endl;
        std::cout << "13. EXIT" << std::endl;

        std::cout<<"------------------------------------"<<std::endl;

        int choice;
        std::cin >> choice;

        switch(choice)
        {
            case 1: {
                std::string sourcePath, fileName;
                std::cout << "Enter the file path: ";
                std::cin >> sourcePath;
                std::cout << "Enter the file name: ";
                std::cin >> fileName;
                virtualDisk.copyFileFromSystemDisk(sourcePath, fileName);

                break;
            }
            case 2: {
                std::string directoryName;
                std::cout << "Enter the directory name: ";
                std::cin >> directoryName;
                virtualDisk.createDirectory(directoryName);
                break;
            }
            case 3: {
                std::string directoryName;
                std::cout << "Enter the directory name: ";
                std::cin >> directoryName;
                virtualDisk.deleteDirectoryRecursively(directoryName);
                break;
            }
            case 4: {
                std::string destinationPath, fileName;
                std::cout << "Enter the file name: ";
                std::cin >> fileName;
                std::cout << "Enter destination path: ";
                std::cin >> destinationPath;
                virtualDisk.copyFileToSystemDisk(destinationPath, fileName);
                break;
            }
            case 5: {
                virtualDisk.printDirectory();
                break;
            }
            case 6: {
                std::string sourcePath, newFileName;
                std::cout << "Enter the file path: ";
                std::cin >> sourcePath;
                std::cout << "Enter the new file name: ";
                std::cin >> newFileName;
                virtualDisk.createHardLink(sourcePath, newFileName);
                break;
            }
            case 7: {
                std::string fileName;
                std::cout << "Enter the file name: ";
                std::cin >> fileName;
                virtualDisk.deleteFile(fileName, virtualDisk.currentINodeNumber);
                break;
            }
            case 8: {
                unsigned int bytes;
                std::string fileName;
                std::cout << "Enter the file name: ";
                std::cin >> fileName;
                std::cout << "Enter the number of bytes: ";
                std::cin >> bytes;
                virtualDisk.addNBytes(fileName, bytes);
                break;
            }
            case 9: {
                unsigned int bytes;
                std::string fileName;
                std::cout << "Enter the file name: ";
                std::cin >> fileName;
                std::cout << "Enter the number of bytes: ";
                std::cin >> bytes;
                virtualDisk.takeAwayNBytes(fileName, bytes);
                break;
            }
            case 10: {
                virtualDisk.showDiskUsage();
                break;
            }
            case 11: {
                std::string directoryName;
                std::cout << "Enter the directory name: ";
                std::cin >> directoryName;
                virtualDisk.changeDirectory(directoryName);
                break;
            }
            case 12: {
                virtualDisk.changeToParentDirectory();
                break;
            }
            case 13: {
                return 0;
            }
            default: {
                std::cout << "Wrong command" << std::endl;
                break;
            }

        }
    }
    return 0;
}