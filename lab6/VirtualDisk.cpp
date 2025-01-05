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
    unsigned int currentINodeNumber = 0;
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

    void updateMetaData(unsigned int newUsedBlocks) {
        std::ofstream disk(diskPath, std::ios::binary | std::ios::in | std::ios::out);
        if (!disk.is_open()) {
            throw std::runtime_error("Error opening disk file");
        }

        superBlok.set_numberOfFiles(superBlok.get_numberOfFiles() + 1);
        superBlok.set_lastModification();
        superBlok.set_usedBytesSize(superBlok.get_usedBytesSize() + newUsedBlocks * DataBlockSize);
        superBlok.write(disk);

        iNodeArray.write(disk);
        iNodeBitmap.write(disk);
        dataBlockBitmap.write(disk);
        disk.close();
    }

    void updateParentsDirectory(unsigned int newUsedBlocks) {
        unsigned int parentINodeNumber = currentINodeNumber;
        while(parentINodeNumber != 0)
        {
            iNode& parentINode = iNodeArray[parentINodeNumber];
            parentINode.sizeOfFile += newUsedBlocks * DataBlockSize;
            parentINodeNumber = parentINode.parentDirectoryIndexNode;
            parentINode.modificationTime = std::time(nullptr);
        }

        iNode& rootINode = iNodeArray[0];
        rootINode.sizeOfFile += newUsedBlocks * DataBlockSize;
        rootINode.modificationTime = std::time(nullptr);
    }


public:
    VirtualDisk(unsigned int totalBytes, const std::string& diskPath) {
        this->diskPath = diskPath;
        std::ifstream disk(diskPath, std::ios::binary);
        if (disk.is_open()) {
            superBlok = SuperBlok(disk);
            iNodeArray = INodeArray(disk, superBlok.get_numberOfINodes());
            iNodeBitmap = bitMap(disk, superBlok.get_numberOfINodes());
            dataBlockBitmap = bitMap(disk, superBlok.get_numberOfDataBlocks());
            std::cout << "Wczytano dysk wirtualny z pliku " << diskPath << std::endl;
            superBlok.printSuperBlock();

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
            createDirectory("root");

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
        iNode.sizeOfFile = iNode.usedBlocksOfData * DataBlockSize;
        iNode.directDataBlocks[0] = dataBlockBitmap.findFirstFreeBlock();
        dataBlockBitmap.set(iNode.directDataBlocks[0], true);
        iNode.countHardLinks = 1;
        iNode.parentDirectoryIndexNode = currentINodeNumber;
        iNode.fileType = FileType::directory;

        // update current directory
        // don't update current directory if we create root directory
        if(currentINodeNumber != 0 || iNodeBitmap.countFree() != superBlok.get_numberOfINodes() - 1)
        {
            std::map<std::string, unsigned int> directoryEntries = readDirectoryDataBlock(iNodeArray[currentINodeNumber].directDataBlocks[0]);
            directoryEntries[directoryName] = iNodeNumber;
            writeDirectoryDataBlock(iNodeArray[currentINodeNumber].directDataBlocks[0], directoryEntries);
            updateParentsDirectory(iNode.usedBlocksOfData);
        }

        updateMetaData(iNode.usedBlocksOfData);

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
        iNode.usedBlocksOfData = 0;
        iNode.sizeOfFile = 0;
        iNode.countHardLinks = 1;
        iNode.parentDirectoryIndexNode = currentINodeNumber;
        iNode.fileType = FileType::file;

        char buffer[DataBlockSize];
        unsigned int blockIndex = 0;

        std::ofstream disk(diskPath, std::ios::binary | std::ios::in | std::ios::out);
        if (!disk.is_open()) {
            throw std::runtime_error("Error opening disk file");
        }

        // Warning - to działa tylko jeśli plik < 8 * DataBlockSize
        while(sourceFile.read(buffer, DataBlockSize) || sourceFile.gcount() > 0) {
            std::cout << "lol" << std::endl;
            unsigned int dataBlockNumber = dataBlockBitmap.findFirstFreeBlock();
            std::cout << "dataBlockNumber: " << dataBlockNumber << std::endl;
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
        std::cout << "po lol" << std::endl;
        disk.close();

        iNode.usedBlocksOfData = blockIndex;
        iNode.sizeOfFile = iNode.usedBlocksOfData * DataBlockSize;

        // update current directory
        std::map<std::string, unsigned int> directoryEntries = readDirectoryDataBlock(iNodeArray[currentINodeNumber].directDataBlocks[0]);
        directoryEntries[fileName] = iNodeNumber;
        writeDirectoryDataBlock(iNodeArray[currentINodeNumber].directDataBlocks[0], directoryEntries);

        updateParentsDirectory(iNode.usedBlocksOfData);
        updateMetaData(iNode.usedBlocksOfData);
    }

    void printDirectory() {
        std::map<std::string, unsigned int> directoryEntries = readDirectoryDataBlock(iNodeArray[currentINodeNumber].directDataBlocks[0]);

        const int colWidthName = 15;
        const int colWidthType = 13;
        const int colWidthSize = 10;
        const int colWidthTime = 30;

        std::cout << "Zawartość katalogu:" << std::endl;
        std::cout << std::left
                << std::setw(colWidthName) << "Nazwa pliku"
                << std::setw(colWidthType) << "Typ pliku"
                << std::setw(colWidthSize) << "Rozmiar B"
                << std::setw(colWidthTime) << "Czas utworzenia"
                << "Czas ostatniej modyfikacji"
                << std::endl;

        std::cout << std::string(colWidthName + colWidthType + colWidthSize + 2 * colWidthTime, '-') << std::endl;

        unsigned int totalSizeLevel0 = 0;
        unsigned int totalSize = 0;
        for (const auto& [fileName, iNodeNumber] : directoryEntries) {

            iNode iNode = iNodeArray[iNodeNumber];
            totalSizeLevel0 += iNode.usedBlocksOfData * DataBlockSize;
            totalSize += iNode.sizeOfFile;

            // Usunięcie '\n' z końca stringa
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
        std::cout << std::endl;
        std::cout << "Rozmiar plików w samym katalogu: " << totalSizeLevel0 << " B" << std::endl;
        std::cout << "Rozmiar plików wraz z podkatalogami: " << totalSize << " B" << std::endl;
        std::cout << "Wolne miejsce na dysku: " << superBlok.get_totalBytesSize() - superBlok.get_usedBytesSize() << " B" << std::endl;
    }

};


int main() {

    VirtualDisk virtualDisk(4857600, "disk.bin");
    // TODO: przyjmować ścieżkę do dysku jako argument programu, jeżeli nie istnieje
    //to pytamy czy tworzyć nowy o podanym rozmiarze

    while(true) {
        std::cout<<std::endl << std::endl;
        std::cout<<"------------------------------------"<<std::endl;
        std::cout << "1. Skopiuj plik z dysku systemu na dysk wirtualny" << std::endl;
        std::cout << "2. Utwórz katalog" << std::endl;
        std::cout << "3. Usuń katalog z dysku wirtualnego" << std::endl;
        std::cout << "4. Skopiuj plik z dysku wirtualnego na dysk systemu" << std::endl;
        std::cout << "5. Wyświetl informacje o aktualnym katalogu" << std::endl;
        std::cout << "6. Stwórz twarde dowiązanie do pliku lub katalogu" << std::endl;
        std::cout << "7. Usuwanie pliku lub dowiązania z wirtualnego dysku" << std::endl;
        std::cout << "8. Dodaj do pliku o zadanej nazwie N bajtów" << std::endl;
        std::cout << "9. Skrócenie pliku o zadanej nazwie o N bajtów" << std::endl;
        std::cout << "10. Wyświetl informacje o zajętości dysku" << std::endl;
        std::cout << "11. EXIT" << std::endl;

        int choice;
        std::cin >> choice;

        switch(choice)
        {
            case 1: {
                // TODO:
                std::string path, fileName;
                std::cout << "Podaj ścieżkę do pliku: ";
                std::cin >> path;
                std::cout << "Podaj nazwę pliku: ";
                std::cin >> fileName;
                virtualDisk.copyFileFromSystemDisk(path, fileName);
                break;
            }
            case 2: {
                std::string directoryName;
                std::cout << "Podaj nazwę katalogu: ";
                std::cin >> directoryName;
                virtualDisk.createDirectory(directoryName);
                break;
            }
            case 3: {
                // TODO:
                break;
            }
            case 4: {
                // TODO:
                break;
            }
            case 5: {
                virtualDisk.printDirectory();
                break;
            }
            case 6: {
                // TODO:
                break;
            }
            case 7: {
                // TODO:
                break;
            }
            case 8: {
                // TODO:
                break;
            }
            case 9: {
                // TODO:
                break;
            }
            case 10: {
                // TODO:
                break;
            }
            case 11: {
                return 0;
            }
            default: {
                std::cout << "Niepoprawny wybór" << std::endl;
                break;
            }

        }
    }
    return 0;
}