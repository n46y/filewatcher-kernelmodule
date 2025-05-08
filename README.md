# Filewatcher Kernel Module
## Leiras
Ez egy egyszeru kernelmodul, amivel egy character device-on keresztul kommunikalhatunk.
Celja, hogy a megadott eleresi utvonalu file-okat figyeli es ha lekerdezzuk, kiirja hogy az utolso lekerdezes ota volt a valtozas a file tartalmaban.

## Mukodes
Ez a modul letrehoz egy chardev-et amibe iraskor megkeressuk hogy letezik-e az adott file es ha igen hozzaadja egy listahoz, vagy ha mas parancsot adunk meg akkor eltavolitja onnan. Ha pedig olvasunk a filebol akkor lekerdezi a `vfs_getattr` metodus segitsegevel az utolso modositast es ha az elter az utolso eltarolt ertektol akkor kiirja hogy mododult a file egyebkent oedig hogy nem.

## Build
`make all` utan a C file melle fordul a tobbi file. (Az en kernel releasem: `6.14.2-arch1-1`)

## Hasznalat
- `sudo insmod filewatch.ko` utan letrejon egy `/dev/filewatch_chrdev` chardev, ezzel lehet kommunikalni
- `printf "ADD:/full/path/to/your/file" > /dev/filewatch_chrdev` a teljes, abszolut eleresi ut megadasa `ADD:` elotaggal hozzaad egy file-t a watchlisthez (amiben max 8 elem lehet)
- `printf "RMV:/full/path/to/your/file" > /dev/filewatch_chrdev` a teljes, abszolut eleresi ut megadasa `RMV:` elotaggal eltavolit egy file-t a watchlistbol (ha az letezik benne)
- `cat /dev/filewatch_chrdev` soronk kiirja a watchlisten levo file-okrol, hogy az utolso lekerdezes ota volt e bennuk modositas (hozzaadas utan ha nem modositunk akkor elso lekerdezesnel nem szamit modositottnak)
- `sudo rmmod filewatch` a module eltavolitasa utan a chardev file is torlodik.


### Nepun kod:
C06R3L
