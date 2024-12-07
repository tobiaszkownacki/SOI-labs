#include <iostream>
#include "monitor.h"
#include <deque>
#include <unistd.h>

class Buffer: public std::deque<int> {
public:
    std::string show() {
        std::string result = "[";
        for(int i = 0; i < size(); ++i) {
            result += std::to_string(at(i));
            if(i != size() - 1) {
                result += ", ";
            }
        }
        result += "]";
        return result;
    }
};


Buffer buffer;
Semaphore mutex(1), prodEvenMutex(0), prodOddMutex(0), consEvenMutex(0), consOddMutex(0);
unsigned int numOfProdEvenWaiting = 0, numOfProdOddWaiting = 0, numOfConsEvenWaiting = 0, numOfConsOddWaiting = 0;

bool canProdEven() {
    int evenCount = 0;
    for (int i = 0; i < buffer.size(); ++i) {
        if(buffer[i] % 2 == 0) {
            evenCount++;
        }
    }

    return evenCount < 10;
}

bool canProdOdd() {
    int evenCount = 0;
    for(int i = 0; i < buffer.size(); ++i) {
        if(buffer[i] % 2 == 0) {
            evenCount++;
        }
    }

    return evenCount > buffer.size() - evenCount;
}

bool canConsEven() {
    int front = buffer.front();
    return front % 2 == 0 && buffer.size() >= 3;
}

bool canConsOdd() {
    int front = buffer.front();
    return front % 2 == 1 && buffer.size() >= 7;
}

void *prodEven(void* ptr) {
    int i = 0;
    while (true) {
        mutex.p();
        if(!canProdEven()){
            std::cout << "ProdEven waiting" << std::endl;
            numOfProdEvenWaiting++;
            mutex.v();
            prodEvenMutex.p();
            std::cout << "ProdEven woke up" << std::endl;
            --numOfProdEvenWaiting;
        }
        int element = i % 50;
        i += 2;
        buffer.push_back(element);
        std::cout << "Produced even: " << element <<  ", " << buffer.size();
        std::cout << " " << buffer.show() << std::endl;

        if(numOfProdOddWaiting > 0U && canProdOdd()) {
            prodOddMutex.v();
        } else if(numOfConsEvenWaiting > 0U && canConsEven()) {
            consEvenMutex.v();
        } else if(numOfConsOddWaiting > 0U && canConsOdd()) {
            consOddMutex.v();
        } else {
            mutex.v();
        }
        sleep(1);
    }
    return NULL;
}

void *prodOdd(void *ptr) {
    int i = 1;
    while (true) {
        mutex.p();
        if(!canProdOdd()){
            std::cout << "ProdOdd waiting" << std::endl;
            numOfProdOddWaiting++;
            mutex.v();
            prodOddMutex.p();
            std::cout << "ProdOdd woke up" << std::endl;
            --numOfProdOddWaiting;
        }
        int element = i % 50;
        i += 2;
        buffer.push_back(element);
        std::cout << "Produced odd: " << element <<  ", " << buffer.size();
        std::cout << " " << buffer.show() << std::endl;

        if(numOfProdEvenWaiting > 0U && canProdEven()) {
            prodEvenMutex.v();
        } else if(numOfConsEvenWaiting > 0U && canConsEven()) {
            consEvenMutex.v();
        } else if(numOfConsOddWaiting > 0U && canConsOdd()) {
            consOddMutex.v();
        } else {
            mutex.v();
        }
        sleep(1);
    }
    return NULL;
}

void *consEven(void *ptr) {
    while (true) {
        mutex.p();
        if(!canConsEven()){
            std::cout << "ConsEven waiting" << std::endl;
            numOfConsEvenWaiting++;
            mutex.v();
            consEvenMutex.p();
            std::cout << "ConsEven woke up" << std::endl;
            --numOfConsEvenWaiting;
        }
        int element = buffer.front();
        buffer.pop_front();
        std::cout << "Consumed Even: " << element <<  ", " << buffer.size();
        std::cout << " " << buffer.show() << std::endl;

        if(numOfProdEvenWaiting > 0U && canProdEven()) {
            prodEvenMutex.v();
        } else if(numOfProdOddWaiting > 0U && canProdOdd()) {
            prodOddMutex.v();
        } else if(numOfConsOddWaiting > 0U && canConsOdd()) {
            consOddMutex.v();
        } else {
            mutex.v();
        }
        sleep(1);
    }
    return NULL;
}

void *consOdd(void* ptr) {
    while (true) {
        mutex.p();
        if(!canConsOdd()){
            std::cout << "ConsOdd waiting" << std::endl;
            numOfConsOddWaiting++;
            mutex.v();
            consOddMutex.p();
            std::cout << "ConsOdd woke up" << std::endl;
            --numOfConsOddWaiting;
        }
        int element = buffer.front();
        buffer.pop_front();
        std::cout << "Consumed Odd: " << element <<  ", " << buffer.size();
        std::cout << " " << buffer.show() << std::endl;

        if(numOfProdEvenWaiting > 0U && canProdEven()) {
            prodEvenMutex.v();
        } else if(numOfProdOddWaiting > 0U && canProdOdd()) {
            prodOddMutex.v();
        } else if(numOfConsEvenWaiting > 0U && canConsEven()) {
            consEvenMutex.v();
        } else {
            mutex.v();
        }
        sleep(1);
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    if(argc != 3) {
        std::cout << "Invalid number of arguments" << std::endl;
        return 1;
    }

    int testCase = std::stoi(argv[1]);
    int ifFullBuffer = std::stoi(argv[2]);

    pthread_t threads[8];
    void* (*functions[4])(void*) = {prodEven, prodOdd, consEven, consOdd};
    srand(time(NULL));
    if(ifFullBuffer == 1) {
        for(int i = 0; i < 12; ++i){
            buffer.push_back(rand() % 50);
        }
    }
    else if(ifFullBuffer != 0) {
        std::cout << "Invalid second argument" << std::endl;
        return 1;
    }

    std::cout<<"Buffer: "<< buffer.size() << ", " << buffer.show()<<std::endl;

    switch (testCase) {
        case 1:
            std::cout << "1. 1-A1" << std::endl;
            pthread_create(&threads[0], NULL, functions[0], NULL);
            pthread_join(threads[0], NULL);
            break;
        case 2:
            std::cout << "2. 1-A2" << std::endl;
            pthread_create(&threads[1], NULL, functions[1], NULL);
            pthread_join(threads[1], NULL);
            break;
        case 3:
            std::cout << "3. 1-B1" << std::endl;
            pthread_create(&threads[2], NULL, functions[2], NULL);
            pthread_join(threads[2], NULL);
            break;
        case 4:;
            std::cout << "4. 1-B2" << std::endl;
            pthread_create(&threads[3], NULL, functions[3], NULL);
            pthread_join(threads[3], NULL);
            break;
        case 5:
            std::cout << "5. 1-A1, 1-A2" << std::endl;
            pthread_create(&threads[0], NULL, functions[0], NULL);
            pthread_create(&threads[1], NULL, functions[1], NULL);
            pthread_join(threads[0], NULL);
            pthread_join(threads[1], NULL);
            break;
        case 6:
            std::cout << "6. 1-B1, 1-B2" << std::endl;
            pthread_create(&threads[2], NULL, functions[2], NULL);
            pthread_create(&threads[3], NULL, functions[3], NULL);
            pthread_join(threads[2], NULL);
            pthread_join(threads[3], NULL);
            break;
        case 7:
            std::cout << "7. 1-A1, 1-B1" << std::endl;
            pthread_create(&threads[0], NULL, functions[0], NULL);
            pthread_create(&threads[2], NULL, functions[2], NULL);
            pthread_join(threads[0], NULL);
            pthread_join(threads[2], NULL);
            break;
        case 8:
            std::cout << "8. 1-A2, 1-B2" << std::endl;
            pthread_create(&threads[1], NULL, functions[1], NULL);
            pthread_create(&threads[3], NULL, functions[3], NULL);
            pthread_join(threads[1], NULL);
            pthread_join(threads[3], NULL);
            break;
        case 9:
            std::cout << "9. 1-A1, 1-A2, 1-B1, 1-B2" << std::endl;
            for(int i = 0; i < 4; ++i) {
                pthread_create(&threads[i], NULL, functions[i], NULL);
            }
            for(int i = 0; i < 4; ++i) {
                pthread_join(threads[i], NULL);
            }
            break;
        case 10:
            std::cout << "10. 2-A1, 2-A2, 2-B1, 2-B2" << std::endl;
            for(int i = 0; i < 8; ++i) {
                pthread_create(&threads[i], NULL, functions[i % 4], NULL);
            }
            for(int i = 0; i < 8; ++i) {
                pthread_join(threads[i], NULL);
            }
            break;
        default:
            std::cout << "Invalid test case number" << std::endl;
            return 1;
    }

    return 0;
}