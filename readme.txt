
  LibGadu dla Win32 jako DLL

  (C)2003 Rafa³ Lindemann <stamina@kliper.eu.org>
				  + autorzy LibGadu


  Biblioteka powsta³a na potrzeby wtyczki GG do programu Konnekt. Tworzy dynamicznego
  DLL'a eksportujacego wszystkie funkcje z LibGadu. Ten kod w kilku miejscach
  ró¿ni siê od "orygina³u" i raz na jakiœ czas jest uaktualniany do najnowszej
  wydanej wersji. Zmiany maj¹ na celu lepsze przystosowanie kodu do kompilacji
  na platformie Windows i w œrodowisku MSVC. Jest te¿ kilka obejœæ starych b³êdów
  w niesynchronicznej czêœci kodu libgadu, które byæ mo¿e ju¿ zosta³y naprawione,
  byæ mo¿e nie, s¹ i dzia³aj¹...
  Ciekawsz¹ "modyfikacj¹" jest mo¿liwoœæ dostarczenia w³asnej funkcji loguj¹cej
  podstawiaj¹c stosowny wskaŸnik pod zewnêtrzn¹ zmienn¹ gg_debug.
  
  Niektóre zmiany, które tutaj wprowadzi³em, po pewnych modyfikacjach
  pojawi³y siê w oryginalnym LibGadu. Mo¿e siê wiêc zdarzyæ, ¿e nie
  wszystko w tej bibliotece bêdzie tak samo jak w oryginalnym LG!
  
  Moje zmiany w kodzie oznaczone s¹ zazwyczaj komentarzem "RL" albo "HAO"
  
  Zgodnie z licencj¹ LGPL (w naszym jej rozumieniu) publikujemy kod biblioteki
  na zasadach tej¿e licencji.


  O autorach poszczególnych czêœci libgadu przeczytasz w pozosta³ych
  plikach Ÿród³owych.

  Miejsca modyfikowane przeze mnie bywaj¹ oznaczone, ale pewnie wiêkszoœæ
  nie jest...
  
  UWAGA! DCC i obs³uga konferencji mog¹ nie dzia³aæ jak nale¿y. DCC najpewniej
         w ogóle nie zadzia³a, bo jak dot¹d jeszcze nie wykorzystywa³em tamtego
         kodu...
         NIE MO¯NA u¿ywaæ po³¹czeñ asynchronicznych. Fragmenty kodu, które za to
         odpowiadaj¹ s¹ w du¿ej mierze wyciête. Na Win32 bardzo ³adnie mo¿na to
         "omin¹æ" u¿ywaj¹c po³¹czenia w wydzielonym w¹tku...
  
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


