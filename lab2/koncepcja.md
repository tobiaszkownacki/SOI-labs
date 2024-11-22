### Zadanie

Proszę zrealizować algorytm szeregowania dzielący procesy użytkownika na trzy grupy: A, B i C. Dodatkowo, proszę opracować funkcję systemową umożliwiającą przenoszenie procesów pomiędzy grupami. Procesy w grupie C otrzymują dwa razy więcej czasu niż procesy z grupy B. Zakładamy, że nowy proces domyślnie znajduje się w grupie A oraz że w grupie A znajduje się co najmniej 1 proces. Opracować również łatwą metodę weryfikacji poprawności rozwiązania.

Aby wykonać te zadanie będziemy potrzebowali nowego pola w strukturze proc, które będzie określało do której grupy należy proces. Trzeba pamiętać aby ustawić grupe A dla wszystkich nowych procesów. Za pomocą syscalla set_group będzie możliwość zmienienia przynależności procesu do poszczególnej grupy. Ten syscall przez to, że musi pracować na samym jądrze, będzie przekazywać zapytanie do niego jako taskcall. Implementacja tego syscalla będzie polegała na przejściu po wszystkich procesach i znalezienie procesu o numerze pid przekazanym do syscalla i zmianie grupy. Szeregowanie procesów będzie polegało na tym, że jeśli dany proces skończy prace to na szczyt kolejki wejdzie proces, który jest w następnej grupie i jest najbliżej szczytu. Proces, który skończył prace idzie na sam koniec kolejki. Będzie to wymagało przepinania wskaźników na liście jednokierunkowej.

---------------------------
### Lista plików do modyfikacji
- **include**
    - callnr.h - zwiększenie rozmiaru tablicy deskryptorów oraz dodanie numerów dla syscalla get_group.
    - com.h - zdefiniowanie systaska dla set_group.
    - config.h - zwiększenie rozmiaru tablicy deskryptorów procesów do 64.
- **fs**
    - table.c - dodanie wpisów dla dodanego wywołania systemowego.
- **mm**
    - table.c - dodanie definicji funkcji realizującej wywołanie systemowe.
    - proto.h - dodanie prototypów funkcji realizujących wywołanie systemowe.
    - misc.c - dodanie funkcji, które przekazują wywołanie SET_SETGROUP do jądra.
- **kernel**
    - proc.h - dodanie pola opisujące numer grupy do struktury proc.
    - dmp.c - modyfikacja wyświetlania tablicy procesów aby było widać grupe.
    - main.c - dodanie przypisania grupy A dla procesu.
    - system.c - dodanie do_setgroup do FORWARD PROTYPE oraz obsługi tego wywołania. Dodanie tablicy, która zawiera współczynniki czasu dla każdej grupy. Napisanie funkcji, która realizuje do_setgroup(). Przypisanie w do_fork() grupy A do nowego procesu.
    - clock.c - w funkcjach do_clocktick() oraz clock_handler() dodanie warunków, które dadzą odpowiedni czas dla procesu w zależności od grupy.
    - proc.c - modyfikacja funkcji sched.
------------------------------
### Testowanie
Test będzie polegał na stworzeniu programu, który przyjmie trzy parametry. Każdy parametr określi liczbę procesów do utworzenia i przypisania do odpowiedniej grupy. Każdy z utworzonych procesów będzie wykonywać kod nieskończonej pętli. Następnie za pomocą F1 zobaczymy zmodyfikowaną tabele procesów. Dzięki widoczności grupy procesu będzie można porównać czy proporcje między czasem wykonywania procesów z grup B i C są zachowane.

