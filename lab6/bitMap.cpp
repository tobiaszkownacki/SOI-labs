#include <vector>
#include <fstream>

class bitMap {
private:
    std::vector<bool> map;
public:

    bitMap(unsigned int size)
    {
        map.resize(size);
    }

    bitMap(std::ifstream& disk, unsigned int size)
    {
        map.resize(size);
        char byte;
        for (unsigned int i = 0; i < size; i++) {
            if (!disk.read(reinterpret_cast<char*>(&byte), sizeof(char))) {
                throw std::runtime_error("Error reading bitMap");
            }
            map[i] = byte;
        }
    }

    bitMap() = default;

    void write(std::ofstream& disk) const
    {
        for (const auto& bit : map) {
            if (!disk.write(reinterpret_cast<const char*>(&bit), sizeof(bool))) {
                throw std::runtime_error("Error writing bitMap");
            }
        }
    }

    bool operator[](unsigned int index) const
    {
        return map[index];
    }

    void set(unsigned int index, bool value)
    {
        map[index] = value;
    }

    unsigned int size() const
    {
        return map.size();
    }

    unsigned int findFirstFree() const
    {
        for (unsigned int i = 0; i < map.size(); i++) {
            if (!map[i]) {
                return i;
            }
        }
        return -1;
    }

    unsigned int countFree() const
    {
        unsigned int count = 0;
        for (const auto& bit : map) {
            if (!bit) {
                count++;
            }
        }
        return count;
    }
};