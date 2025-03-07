# SOI lab6 koncepcja

Tobiasz Kownacki

--------------

## Zadanie

W pliku na dysku należy zorganizować system plików z wielopoziomowym katalogiem. Należy zrealizować aplikację konsolową, przyjmującą polecenia, wywoływaną z nazwą pliku implementującego dysk wirtualny. Należy zaimplementować następujące operacje, dostępne dla użytkownika tej aplikacji:

1. Tworzenie wirtualnego dysku (gdy plik wirtualnego dysku będący parametrem nie istnieje to pytamy się o utworzenie przed przejściem do interakcji) - jak odpowiedź negatywna to kończymy program. Parametrem polecenia powinien być rozmiar tworzonego systemu plików w bajtach. Dopuszcza się utworzenie systemu nieznacznie większego lub mniejszego, gdy wynika to z przyjętych założeń dotyczących budowy.
2. Kopiowanie pliku z dysku systemu na dysk wirtualny
3. Utworzenie katalogu na dysku wirtualnym
4. Usunięcie katalogu z dysku wirtualnego
5. Kopiowanie pliku z dysku wirtualnego na dysk systemu,
6. Wyświetlanie katalogu dysku wirtualnego z informacją o rozmiarze (sumie) plików w katalogu, rozmiarze plików w katalogu razem z podkatalogami (suma), oraz ilości wolnej pamięci na dysku wirtualnym
7. Tworzenie twardego dowiązania do pliku lub katalogu
8. Usuwanie pliku lub dowiązania z wirtualnego dysku
9. Dodanie do pliku o zadanej nazwie n bajtów
10. Skrócenie pliku o zadanej nazwie o n bajtów
11. Wyświetlenie informacji o zajętości dysku

--------------

## Koncepcja

System plików będzie uproszczona wersją systemu plików Ext2
Dysk wirtualny będzie składał sie z poniższej struktury.

1. Superblock. Będzie sie składał z następujących pól:
    - Liczba plików
    - Data ostatniej modyfikacji
    - Wielkość dysku wirtualnego
    - Ilość bajtów zajętych przez dane
    - Ilość bajtów wolnych na dane
    - Ilość bajtów pojedynczego iNode'a
    - Liczba iNode w systemie plików
    - Liczba wolnych iNode'ów
    - Ilość bajtów pojedynczego bloku danych
    - Liczba bloków danych
    - Liczba wolnych bloków danych
    - Wskaźnik na pierwszy iNode w tablicy
    - Wskaźnik na początek bitmapy zajętości iNode'ów
    - Wskaźnik na początek bitmapy zajętości bloków danych
    - Wskaźnik na pierwszy blok danych
2. Tablica iNode'ów. Pojedynczy iNode będzie się składał z następujących pól:
    - Numer iNode'a
    - Data utworzenia
    - Data ostatniej modyfikacji
    - Ilość zajętych bloków danych
    - Wielkość pliku w bajtach
    - Tablica 8 bezpośrednich wskaźników na bloki danych
    - Wskaźnik pośredni, który wskazuje na blok danych z kolejnymi wskaźnikami
    - Liczba twardych powiązań
    - Typ (plik, czy katalog)
    - Numer iNode'a katalogu rodzica w tablicy iNode'ów
3. Bitmapa zajętości iNode'ów.
4. Bitmapa zajętości bloków danych
5. Bloki danych o równej wielkości

iNode, będzie miał 8 wskaźników bezpośrednich do bloków danych(2Kib), oraz wskaźnik do bloku danych z kolejnymi wskaźnikami. Maksymalna wielkość pliku będzie wynosić około 1MiB.
Jeśli iNode ma tryb katalogu, to zajmuje on tylko jeden blok danych, gdzie jest mapowanie nazwy pliku/katalogu na numer iNode'a w tablicy. Numer iNode'a katalogu rodzica będzie bardzo przydatny przy poruszani się po systemie plików.

## Testy

Testy będą testować każdą pojedynczą funkcjonalność oraz ich kombinacje, czyli np. załadowanie pliku X z systemu na dysk wirtualny. Usunięcie go, a następnie dodanie pliku Y w tym samym katalogu o tej samej nazwie. Następnie przesłanie pliku z powrotem na komputer i sprawdzenie czy jest taki sam jak plik Y. Zostanie napisane sprawozdanie z testów z potrzebnymi screenami potwierdzającymi działanie programu.
