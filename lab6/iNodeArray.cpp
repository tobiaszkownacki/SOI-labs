#include <vector>
#include <fstream>
#include "structures.h"


class INodeArray {
private:
    std::vector<iNode> iNodes;
    unsigned int maxDirectDataBlocks = 8;
public:

    INodeArray(unsigned int numberOfINodes)
    {
        iNodes.resize(numberOfINodes);
        for (unsigned int i = 0; i < numberOfINodes; ++i) {
            iNodes[i].indexNode = i;
        }
    }

    INodeArray(std::ifstream& disk, unsigned int numberOfINodes)
    {
        iNodes.resize(numberOfINodes);
        for (unsigned int i = 0; i < numberOfINodes; i++) {
            if (!disk.read(reinterpret_cast<char*>(&iNodes[i]), sizeof(iNode))) {
                throw std::runtime_error("Error reading iNode");
            }
        }
    }

    INodeArray() = default;

    void write(std::ofstream& disk) const
    {
        for (const auto& iNode : iNodes) {
            if (!disk.write(reinterpret_cast<const char*>(&iNode), sizeof(iNode))) {
                throw std::runtime_error("Error writing iNode");
            }
        }
    }

    iNode& operator[](unsigned int index)
    {
        return iNodes[index];
    }

};