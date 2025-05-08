# Filewatcher Kernel Module
## Leírás
Ez egy egyszerű kernelmodul, amivel egy character device-on keresztül kommunikálhatunk.
Célja, hogy a megadott elérési utvonalú file-okat figyeli és ha lekérdezzük, kiírja hogy az utolsó lekérdezés óta volt-e változás a file tartalmában.

## Mőködés
Ez a modul létrehoz egy chardev-et, amibe íráskor megkeressük, hogy létezik-e az adott file és, ha igen hozzáadja egy listához, vagy ha más parancsot adunk meg, akkor eltávolítja onnan. Ha pedig olvasunk a file-ból, akkor lekérdezi a `vfs_getattr` metódus segítségével az utolsó módosítást és ha az eltér az utolsó eltárolt értéktől, akkor kiírja, hogy módosult a file, egyébként pedig, hogy nem.

## Build
`make all` után a C file mellé fordul a többi file. (Az én kernel releasem: `6.14.2-arch1-1`)

## Használat
- `sudo insmod filewatch.ko` után létrejön egy `/dev/filewatch_chrdev` chardev, ezzel lehet kommunikalni
- `printf "ADD:/full/path/to/your/file" > /dev/filewatch_chrdev` a teljes, abszolút elérési út megadása `ADD:` előtaggal hozzáad egy file-t a watchlisthez (amiben max 8 elem lehet)
- `printf "RMV:/full/path/to/your/file" > /dev/filewatch_chrdev` a teljes, abszolút elérési út megadása `RMV:` előtaggal eltávolít egy file-t a watchlist-ből (ha az létezik benne)
- `cat /dev/filewatch_chrdev` soronként kiírja a watchlist-en lévő file-okról, hogy az utolsó lekérdezés óta volt-e bennük módosítás (hozzáadas után, ha nem módosítunk, akkor első lekérdezésnél nem szamít módosítottnak)
- `sudo rmmod filewatch` a module eltávolítása után a chardev file is törlődik.


### Nepun kód:
C06R3L
