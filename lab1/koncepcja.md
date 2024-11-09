### Lista plików do modyfikacji:
- /usr/include/minix/config.h - zwiększenie rozmiaru tablicy deskryptorów procesów do 64.
- /usr/include/minix/callnr.h - zwiększyć liczbę wywołań systemowych o 4 oraz dodać stałe identyfikujące wywołania systemowe.
- /usr/src/mm/proto.h - dodanie prototypów funkcji realizujących wywołania systemowe. Do 1 zadania będzie to np do_maxChildren i do_whoMaxChildren, a do drugiego do_maxChildrenWithinLevels i do_whoMaxChildrenWithinLevels.
- /usr/src/mm/misc.c - dodanie definicji funkcji realizującej wywołanie systemowe.
- /usr/src/fs/table.c - dodanie wpisów dla dodawanych wywołań systemowych oznaczając je jako nie używane przez serwer FS.
- /usr/src/mm/table.c - dodać wpisy dla dodawanych wywołań systemowych da serwera MM.

### Zadania

1. Zwrócić pid procesu mającego największą liczbę potomków (dzieci, wnuków itd) mieszczących się w przedziale <A, B> (A i B podawane jako parametry), zwrócić liczbę potomków dla tego procesu.   

Będziemy tutaj potrzebować 2 funkcji realizujących wywołanie systemowe np maxChildren i childrenCount.    
W funkcji maxChildren będzie znajdować się zewnętrzna pętla programu. W tej pętli będziemy przeszukiwać tablice deskryptorów procesów w poszukiwaniu aktywnych procesów. Następnie dla każdego takiego procesu będziemy za pomocą funkcji childrenCount zliczać liczbę dzieci. Mając tą liczbę dzieci będziemy sprawdzać czy mieści się miedzy <A, B> i jeśli jest nowym maximum to będziemy zapisywać numer pid oraz liczbę potomków   
Funkcja childrenCount będzie funkcją rekurencyjną. Będzie do niej przekazywany numer procesu rodzica w tablicy deskryptorów jako parametr. Będzie również potrzebna zmienna lokalna children, która będzie zliczać liczbe potomków, ta zmienna będzie również zwracana. Zawarta w funkcji pętla for będzie przechodzić po wszystkich procesach i sprawdzać czy proces jest aktywny oraz czy numer rodzica aktualnie sprawdzanego procesu jest zgodny z numerem rodzica podanym jako parametr funkcji Jeśli tak to zwiększamy liczbe dzieci o jeden oraz wywołujemy childrenCount rekurencyjnie dla kolejnych dzieci (children += childrenCount(children_proc_nr)).

2. Zwrócić pid procesu mającego największą liczbę potomków na kolejnych N (N podawane jako parametr) poziomach (dla N równego 3 będą to: dzieci, wnukowie, prawnukowie), zwrócić liczbą potomków dla tego procesu.   

Tutaj również będziemy potrzebować 2 funkcji realizujących wywołanie systemowe np maxChildrenWithinLevels i countChildrenAtLevels.   
W funkcji maxChildrenWithinLevels będzie znajdować się zewnętrzna pętla programu. W tej pętli będziemy przeszukiwać tablice deskryptorów procesów w poszukiwaniu aktywnych procesów. Następnie dla każdego takiego procesu będziemy zliczać liczbę dzieci za pomocą funkcji countChildrenAtLevels do której będziemy przekazywać numer aktualnie badanego procesu oraz parametr N. Mając liczbę dzieci będziemy sprawdzać czy jest ona nowym maximum i jeśli tak to zapiszemy liczbę dzieci oraz numer pid procesu.   
Funkcja countChildrenAtLevels będzie funkcją rekurencyjną. Jako argument będzie przyjmować numer procesu rodzica oraz liczbę pokoleń (np x)jeszcze do zbadania. Ta funkcja będzie działać tak samo jak childrenCount z poprzedniego zadania z wyjątkiem dodania jednego if'a który będzie sprawdzał czy liczba pozostałych pokoleń jest większa od 1. Jeśli tak to uruchomi rekurencje funkcji z argumentem liczby pokoleń x-1 (np children+=countChildrenAtLevels(children_proc_nr, x-1)).

### Testowanie

1. Zadanie 1

Testująca funkcja będzie przyjmować dodatkowy jeden argument, nazwijmy go x. W funkcji stworzę 2 procesy potomne nazwijmy je dziecko_1 i dziecko_2. Dziecko_1 stworzy kolejne 2*x dzieci. Natomiast dziecko_2 stworzy x dzieci, a następnie każde dziecko stworzy kolejne x dzieci. W ten sposób dziecko_1 będzie miał ogólnie 2*x potomków a dziecko_2 x+x^2. Funkcja na samym początku wypisuję aktualną liczbę procesów na podstawie funkcji a następnie wypisuję wszystkie procesy czyli sume (aktualne+2+2*x+x+x^2).

2. Zadanie 2
Testująca funkcja będzie przyjmować tylko argumenty z zadania. W funkcji stworzę taki schemat procesów jak widać poniżej:

rodzic     
├── dziecko_1       
│    └── wnuk_1   
│        ├── prawnuk_1   
│        ├── prawnuk_2   
│        ├── prawnuk_3   
│        ├── prawnuk_4   
│        ├── prawnuk_5   
│        ├── prawnuk_6
│        ├── prawnuk_7   
|        └── prawnuk_8
└── dziecko_2   
    ├── wnuk_2   
    │   ├── prawnuk_2_1   
    │   ├── prawnuk_2_2   
    │   ├── prawnuk_2_3
    |   ├── prawnuk_2_4   
    |   └── prawnuk_2_5
    └── wnuk_3   
    │   ├── prawnuk_3_1   
    │   ├── prawnuk_3_2   
    │   ├── prawnuk_3_3
    |   ├── prawnuk_3_4   
    |   └── prawnuk_3_4   

Minix na samym rozpoczęciu systemu ma 7 procesów, także dałem dużo nowych procesów aby być pewnym, że funkcja zwróciła pid nowego procesu a nie systemowego minixa.
Dla N = 1, funkcja powinna zwrócić 8 i pid wnuka_1 bo wnuk_1 ma 8 potomków na 1 pokoleniu w dół. Dla N = 2, funkcja powinna zwrócić 12 i pid dziecka_2, bo ma 2 potomków w pierwszym pokoleniu i 8 w drugim co daje w sumie 8. Dla N = 3 powinna zwrócić 23 i pid rodzica.