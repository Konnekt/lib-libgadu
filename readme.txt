
  LibGadu dla Win32 jako DLL

  (C)2003 Rafa� Lindemann <stamina@kliper.eu.org>
				  + autorzy LibGadu


  Biblioteka powsta�a na potrzeby wtyczki GG do programu Konnekt. Tworzy dynamicznego
  DLL'a eksportujacego wszystkie funkcje z LibGadu. Ten kod w kilku miejscach
  r�ni si� od "orygina�u" i raz na jaki� czas jest uaktualniany do najnowszej
  wydanej wersji. Zmiany maj� na celu lepsze przystosowanie kodu do kompilacji
  na platformie Windows i w �rodowisku MSVC. Jest te� kilka obej�� starych b��d�w
  w niesynchronicznej cz�ci kodu libgadu, kt�re by� mo�e ju� zosta�y naprawione,
  by� mo�e nie, s� i dzia�aj�...
  Ciekawsz� "modyfikacj�" jest mo�liwo�� dostarczenia w�asnej funkcji loguj�cej
  podstawiaj�c stosowny wska�nik pod zewn�trzn� zmienn� gg_debug.
  
  Niekt�re zmiany, kt�re tutaj wprowadzi�em, po pewnych modyfikacjach
  pojawi�y si� w oryginalnym LibGadu. Mo�e si� wi�c zdarzy�, �e nie
  wszystko w tej bibliotece b�dzie tak samo jak w oryginalnym LG!
  
  Moje zmiany w kodzie oznaczone s� zazwyczaj komentarzem "RL" albo "HAO"
  
  Zgodnie z licencj� LGPL (w naszym jej rozumieniu) publikujemy kod biblioteki
  na zasadach tej�e licencji.


  O autorach poszczeg�lnych cz�ci libgadu przeczytasz w pozosta�ych
  plikach �r�d�owych.

  Miejsca modyfikowane przeze mnie bywaj� oznaczone, ale pewnie wi�kszo��
  nie jest...
  
  UWAGA! DCC i obs�uga konferencji mog� nie dzia�a� jak nale�y. DCC najpewniej
         w og�le nie zadzia�a, bo jak dot�d jeszcze nie wykorzystywa�em tamtego
         kodu...
         NIE MO�NA u�ywa� po��cze� asynchronicznych. Fragmenty kodu, kt�re za to
         odpowiadaj� s� w du�ej mierze wyci�te. Na Win32 bardzo �adnie mo�na to
         "omin��" u�ywaj�c po��czenia w wydzielonym w�tku...
  
  -----------------------------------------
  
  Podczas kompilowania biblioteki musi byc zdefiniowane
  LIBGADU_EXPORTS
  dla calego projektu.

  -----------------------------------------                    

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License Version
  2.1 as published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.


