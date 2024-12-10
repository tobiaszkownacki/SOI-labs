#include <iostream>
#include "monitor.h"
#include <deque>
#include <unistd.h>

class Buffer : public std::deque<int>
{
public:
    std::string show()
    {
        std::string result = "[";
        for (int i = 0; i < size(); ++i)
        {
            result += std::to_string(at(i));
            if (i != size() - 1)
            {
                result += ", ";
            }
        }
        result += "]";
        return result;
    }
};

bool canPutEven(Buffer buffer) {
    int evenCount = 0;
    for (int i = 0; i < buffer.size(); ++i) {
        if(buffer[i] % 2 == 0) {
            evenCount++;
        }
    }

    return evenCount < 10;
}

bool canPutOdd(Buffer buffer) {
    int evenCount = 0;
    for(int i = 0; i < buffer.size(); ++i) {
        if(buffer[i] % 2 == 0) {
            evenCount++;
        }
    }

    return evenCount > buffer.size() - evenCount;
}

bool canConsEven(Buffer buffer) {
    int front = buffer.front();
    return front % 2 == 0 && buffer.size() >= 3;
}

bool canConsOdd(Buffer buffer) {
    int front = buffer.front();
    return front % 2 == 1 && buffer.size() >= 7;
}


class MyMonitor : public Monitor
{
public:
    MyMonitor();
    void putEven(int elem);
    void putOdd(int elem);
    void getEven();
    void getOdd();
    void fillBuffer();
    void showBuffer();

private:
    Buffer buffer;
    Condition prodEvenCond, prodOddCond, consEvenCond, consOddCond;
};

MyMonitor::MyMonitor() {}

void MyMonitor::fillBuffer() {
    srand(time(NULL));
    for(int i = 0; i < 12; ++i) {
        buffer.push_back(rand() % 50);
    }
}

void MyMonitor::showBuffer() {
    std::cout<<"Buffer: "<< buffer.size() << ", " << buffer.show()<<std::endl;
}

void MyMonitor::putEven(int elem) {
    enter();
    if(!canPutEven(buffer)) {
        std::cout << "ProdEven waiting" << std::endl;
        wait(prodEvenCond);
        std::cout << "ProdEven woke up" << std::endl;
    }
    buffer.push_back(elem);
    std::cout << "Produced even: " << elem <<  ", ";
    showBuffer();

    if(prodOddCond.getWaitingCount() > 0 && canPutOdd(buffer)) {
        signal(prodOddCond);
    } else if(consEvenCond.getWaitingCount() > 0 && canConsEven(buffer)) {
        signal(consEvenCond);
    } else if(consOddCond.getWaitingCount() > 0 && canConsOdd(buffer)) {
        signal(consOddCond);
    }
    leave();
}

void MyMonitor::putOdd(int elem) {
    enter();
    if(!canPutOdd(buffer)) {
        std::cout<<"ProdOdd waiting" << std::endl;
        wait(prodOddCond);
        std::cout << "ProdOdd woke up" << std::endl;
    }
    buffer.push_back(elem);
    std::cout << "Produced odd: " << elem <<  ", ";
    showBuffer();

    if(prodEvenCond.getWaitingCount() > 0 && canPutEven(buffer)) {
        signal(prodEvenCond);
    } else if(consEvenCond.getWaitingCount() > 0 && canConsEven(buffer)) {
        signal(consEvenCond);
    } else if(consOddCond.getWaitingCount() > 0 && canConsOdd(buffer)) {
        signal(consOddCond);
    }
    leave();
}

void MyMonitor::getEven() {
    enter();
    if(!canConsEven(buffer)) {
        std::cout << "ConsEven waiting" << std::endl;
        wait(consEvenCond);
        std::cout << "ConsEven woke up" << std::endl;
    }
    int elem = buffer.front();
    buffer.pop_front();
    std::cout << "Consumed even: " << elem << ", ";
    showBuffer();

    if(prodEvenCond.getWaitingCount() > 0 && canPutEven(buffer)) {
        signal(prodEvenCond);
    } else if(prodOddCond.getWaitingCount() > 0 && canPutOdd(buffer)) {
        signal(prodOddCond);
    } else if(consOddCond.getWaitingCount() > 0 && canConsOdd(buffer)) {
        signal(consOddCond);
    }
    leave();
}

void MyMonitor::getOdd() {
    enter();
    if(!canConsOdd(buffer)) {
        std::cout << "ConsOdd waiting" << std::endl;
        wait(consOddCond);
        std::cout << "ConsOdd woke up" << std::endl;
    }
    int elem = buffer.front();
    buffer.pop_front();
    std::cout << "Consumed odd: " << elem <<  ", ";
    showBuffer();

    if(prodEvenCond.getWaitingCount() > 0 && canPutEven(buffer)) {
        signal(prodEvenCond);
    } else if(prodOddCond.getWaitingCount() > 0 && canPutOdd(buffer)) {
        signal(prodOddCond);
    } else if(consEvenCond.getWaitingCount() > 0 && canConsEven(buffer)) {
        signal(consEvenCond);
    }
    leave();
}

MyMonitor myMonitor;

void *prodEven(void* ptr) {
    int i = 0;
    while (true) {
        myMonitor.putEven(i % 50);
        i += 2;
        sleep(1);
    }
    return NULL;
}

void *prodOdd(void* ptr) {
    int i = 1;
    while (true) {
        myMonitor.putOdd(i % 50);
        i += 2;
        sleep(1);
    }
    return NULL;
}

void *consEven(void *ptr) {
    while (true) {
        myMonitor.getEven();
        sleep(1);
    }
    return NULL;
}

void *consOdd(void* ptr) {
    while (true) {
        myMonitor.getOdd();
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
    int ifFillBuffer = std::stoi(argv[2]);

    pthread_t threads[8];
    void* (*functions[4])(void*) = {prodEven, prodOdd, consEven, consOdd};

    if(ifFillBuffer == 1) {
        myMonitor.fillBuffer();
    }
    else if(ifFillBuffer != 0) {
        std::cout << "Invalid second argument" << std::endl;
        return 1;
    }

    myMonitor.showBuffer();
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




