#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <iomanip>
#include "SuperBlok.cpp"
#include "iNodeArray.cpp"
#include "bitMap.cpp"

const unsigned int DataBlockSize = 2048;

class VirtualDisc {
public:
    std::string path;
    SuperBlok superBlok;
    INodeArray iNodeArray;
    bitMap iNodeBitmap;
    bitMap dataBlockBitmap;
    unsigned int currentINodeNumber = 0;

    VirtualDisc(unsigned int totalBytes, const std::string& path)
    {
        this->path = path;
        std::ifstream disk(path, std::ios::binary);
        if (disk.is_open()) {
            superBlok = SuperBlok(disk);
            iNodeArray = INodeArray(disk, superBlok.get_numberOfINodes());
            iNodeBitmap = bitMap(disk, superBlok.get_numberOfINodes());
            dataBlockBitmap = bitMap(disk, superBlok.get_numberOfDataBlocks());
            std::cout << "Wczytano dysk wirtualny z pliku " << path << std::endl;
            superBlok.printSuperBlock();

        } else {
            std::ofstream disk(path, std::ios::binary);
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

    void writeEmptyDataBlocks(std::ofstream& disk, unsigned int numberOfDataBlocks)
    {
        std::vector<char> emptyBlock(DataBlockSize, 0);
        for (unsigned int i = 0; i < numberOfDataBlocks; i++) {
            if (!disk.write(emptyBlock.data(), DataBlockSize)) {
                throw std::runtime_error("Error writing empty data block");
            }
        }
    }

    void createDirectory(const std::string& directoryName)
    {
        unsigned int iNodeNumber = iNodeBitmap.findFirstFreeBlock();
        iNodeBitmap.set(iNodeNumber, true);
        iNode &iNode = iNodeArray[iNodeNumber];
        iNode.creationTime = std::time(nullptr);
        iNode.modificationTime = std::time(nullptr);
        iNode.usedBlocksOfData = 1;
        iNode.sizeOfFile = 0;
        iNode.directDataBlocks[0] = dataBlockBitmap.findFirstFreeBlock();
        dataBlockBitmap.set(iNode.directDataBlocks[0], true);
        iNode.fileType = FileType::directory;
        iNode.countHardLinks = 1;

        iNode.parentDirectoryIndexNode = currentINodeNumber;

        // update parent directory
        if(currentINodeNumber != 0 || iNodeBitmap.countFree() != superBlok.get_numberOfINodes() - 1)
        {
            std::map<std::string, unsigned int> directoryEntries = readDirectoryDataBlock(iNodeArray[currentINodeNumber].directDataBlocks[0]);
            directoryEntries[directoryName] = iNodeNumber;
            writeDirectoryDataBlock(iNodeArray[currentINodeNumber].directDataBlocks[0], directoryEntries);
        }

        updateBitmaps();
        writeINodes();
        updateSuperBlok(superBlok.get_dataBlockSize());

    }

    void writeDirectoryDataBlock(unsigned int blockNumber, const std::map<std::string, unsigned int>& directoryEntries)
    {
        std::ofstream disk(path, std::ios::binary | std::ios::in | std::ios::out);
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

    void writeINodes()
    {
        std::ofstream disk(path, std::ios::binary | std::ios::in | std::ios::out);
        if (!disk.is_open()) {
            throw std::runtime_error("Error opening disk file");
        }

        disk.seekp(superBlok.get_firstINodeOffset(), std::ios::beg);
        iNodeArray.write(disk);
        disk.close();
    }

    std::map<std::string, unsigned int> readDirectoryDataBlock(unsigned int blockNumber)
    {
        std::map<std::string, unsigned int> directoryEntries;
        std::ifstream disk(path, std::ios::binary);
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

    void updateSuperBlok(size_t size = 0)
    {
        std::ofstream disk(path, std::ios::binary | std::ios::in | std::ios::out);
        if (!disk.is_open()) {
            throw std::runtime_error("Error opening disk file");
        }

        superBlok.set_numberOfFiles(superBlok.get_numberOfFiles() + 1);
        superBlok.set_lastModification();
        superBlok.set_usedBytesSize(superBlok.get_usedBytesSize() + size);
        superBlok.write(disk);
        disk.close();
    }

    void updateBitmaps()
    {
        std::ofstream disk(path, std::ios::binary | std::ios::in | std::ios::out);
        if (!disk.is_open()) {
            throw std::runtime_error("Error opening disk file");
        }
        disk.seekp(superBlok.get_iNodeBitmapOffset(), std::ios::beg);
        iNodeBitmap.write(disk);
        disk.seekp(superBlok.get_dataBlockBitmapOffset(), std::ios::beg);
        dataBlockBitmap.write(disk);
        disk.close();
    }

    void printDirectory()
    {
        std::map<std::string, unsigned int> directoryEntries = readDirectoryDataBlock(iNodeArray[currentINodeNumber].directDataBlocks[0]);

        const int colWidthName = 15;
        const int colWidthType = 13;
        const int colWidthSize = 10;
        const int colWidthTime = 30;

        std::cout << "Zawartość katalogu:" << std::endl;
        std::cout << std::left
                << std::setw(colWidthName) << "Nazwa pliku"
                << std::setw(colWidthType) << "Typ pliku"
                << std::setw(colWidthSize) << "Rozmiar"
                << std::setw(colWidthTime) << "Czas utworzenia"
                << "Czas ostatniej modyfikacji"
                << std::endl;

        std::cout << std::string(colWidthName + colWidthType + colWidthSize + 2 * colWidthTime, '-') << std::endl;

        for (const auto& [fileName, iNodeNumber] : directoryEntries) {
            iNode iNode = iNodeArray[iNodeNumber];

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
    }

};


int main() {
    // napisz switcha z opcjami do wyboru
    // 1. włączenie dysku o podanej ścieżce
    // 2. Stworzenie nowego katalogu o podanej nazwie
    // 3. wyświetlenie zawartości katalogu
    // 4. wejście do katalogu o podanej nazwie
    // 5. wyjście z katalogu
    // 6. skopiowanie pliku z dysku do wirtualnego dysku
    VirtualDisc virtualDisc(4857600, "disk.bin");

    while(true)
    {
        std::cout<<std::endl << std::endl;
        std::cout << "1. Stwórz nowy katalog" << std::endl;
        std::cout << "2. Wyświetl zawartość katalogu" << std::endl;
        std::cout << "3. Wejdź do katalogu" << std::endl;
        std::cout << "4. Wyjdź z katalogu" << std::endl;
        std::cout << "5. Skopiuj plik z dysku do wirtualnego dysku" << std::endl;
        std::cout << "6. Wyjdź z programu" << std::endl;

        int choice;
        std::cin >> choice;

        switch(choice)
        {
            case 1:
            {
                std::string directoryName;
                std::cout << "Podaj nazwę nowego katalogu: ";
                std::cin >> directoryName;
                virtualDisc.createDirectory(directoryName);
                break;
            }
            case 2:
            {
                virtualDisc.printDirectory();
                break;
            }
            case 3:
            {
                std::string directoryName;
                std::cout << "Podaj nazwę katalogu do wejścia: ";
                std::cin >> directoryName;
                // virtualDisc.enterDirectory(directoryName);
                break;
            }
            case 4:
            {
                // virtualDisc.exitDirectory();
                break;
            }
            case 5:
            {
                std::string sourcePath, fileName;
                std::cout << "Podaj ścieżkę do pliku na dysku: ";
                std::cin >> sourcePath;
                std::cout << "Podaj nazwę pliku na dysku wirtualnym: ";
                std::cin >> fileName;
                // virtualDisc.copyFile(sourcePath, fileName);
                break;
            }
            case 6:
            {
                return 0;
            }
            default:
            {
                std::cout << "Niepoprawny wybór" << std::endl;
                break;
            }

        }
    }
    return 0;
}