#include <iostream>
#include <fstream>
#include <string>
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
};


int main() {
    try {
        VirtualDisc virtualDisc(104857600, "virtual_disk.bin");
    } catch (const std::runtime_error& e) {
        std::cerr << "Błąd: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}