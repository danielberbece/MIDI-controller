Proiect PM - MIDI Controller
Student: Daniel Berbece
Grupa: 335CA

I) Descriere cod
	Codul pentru microcontroller a fost creat pentru a coordona urmatoarele
evenimente:

	1. Sa faca switch intre cele doua moduri de operare, folosind butonul PB2 de
pe placuta. Cand acesta genereaza o intrerupere, schimbam modul de operare si
semnalam modul curent prin aprinderea ledului PD7.
	2. Sa itereze prin fiecare linie si fiecare coloana a matricei de butoane
pentru a verifica daca exista input de la utilizator. Fiecare buton este asociat
unui canal MIDI. Avem, astfel, 16 canale in total.
	3. Sa updateze banda de leduri in concordanta cu butoanele apasate/secventa
de sunete din loop. Pentru acest lucru am folosit https://github.com/cpldcpu/light_ws2812
	4. Daca este in modul looping, trebuie sa tina cont de semnelele MIDI pe
care trebuie sa le trimita, la ce interval de timp. Pentru asta, am setat timer
1 sa genereze o intrerupere la o frecventa de 16Hz, iar la fiecare intrerupere
verificam daca trebuie sa trimitem un semnal MIDI.
	5. Sa trimita pe USART semnale MIDI catre calculator

II) Compilare si rulare - Linux
	Pentru a compila aveti nevoie de biblioteca avr-gcc pentru a produce
fisierul hex. Dupa aceea tot ce aveti de facut este sa rulati comanda
		> make
	Acesta va genera mai multe fisiere obiect pe care le va pune in directorul
Objects, iar programul hex va avea numele main.hex
	Pentru a scrie fisierul .hex pe placuta este nevoie de a porni placuta in
modul bootloader (la pornire tinem apasat pe butonul PB2) si sa folosim
programul auxiliar bootloadHID, aflat in fisierul cu acelasi nume. Pentru a urca
codul pe placuta rulam comanda
		> sudo ./bootloadHID -r main.hex
	Flagul -r transmite programului sa reporneasca placuta.

III) Software auxiliar

	Deoarece calculatorul nu stie ca placuta e un device MIDI, trebuie sa
instalam un program auxiliar care sa faca transformarea Serial ↔ MIDI. Un astfel
de program poate fi gasit aici: http://projectgus.github.io/hairless-midiserial/
	Acum calculatorul va vedea un port MIDI pe care se vor transmite semnalele.

IV) Altele

	In radacina proiectului sunt deja compilate 3 variante ale programului:
	* midi-controller-1s.hex - loop cu durata de 1 secunda
	* midi-controller-2s.hex - loop cu durata de 2 secunda
	* half-midi-controller-2s.hex - loop cu durata de 2 secunde, doar pe primele 8 butoane