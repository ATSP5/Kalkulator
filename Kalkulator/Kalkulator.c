/*
 * Kalkulator.c
 *
 * Created: 2018-07-14 12:48:47
 *  Author: Adam
 */ 


#include <avr/io.h>
#define F_CPU 8000000UL
#include "avr/eeprom.h"
#include <avr/io.h>
#include <stddef.h>
#include <util/delay.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
//zdefiniuj odpowiednie sta�e port�w:
#define DDR_LCD DDRD
#define PORT_LCD PORTD
#define RS_LCD 2// Dla linii RS bit nr 2
#define RW_LCD 0// Dla linii RW bit nr 0
#define EN_LCD 3// Dla linii EN bit nr 3
#define DD4_LCD 4// Dla Linii danych bitu 4 bit nr 4
#define DD5_LCD 5// Dla Linii danych bitu 5 bit nr 5
#define DD6_LCD 6// Dla Linii danych bitu 6 bit nr 6
#define DD7_LCD 7// Dla Linii danych bitu 7 bit nr 7
#define A 1// Dla Anody diody wy�wietlacza bit nr 1
/////////////////////////////////////////////////////////Definicje klawiatury
#define percent 10
#define sqrroot 11
#define DP 12
#define CCE 13
#define dotbutton 14
#define plusminus 15
#define mul 16
#define div 17
#define substr 18
#define add 19
#define MC 20
#define MR 21
#define MPlus 22
#define equal 23
void WyslijLCD(int8_t);
void WlaczLCD(void);
void CzyscLCD(void);
void WyswietlLCD(char[],int8_t);
int8_t SprawdzKlawisz(void);
void Dzialanie(int8_t);
int8_t Insert(int8_t dat,bool FirstHalfBajt)//Ustaw odpowiednie bity linii danych (tryb 4'ro bitowy) pozostawi�j�c reszt� jako 0, dzi�ki czemu mo�na u�y� operacji sumy logicznej.
{
	int8_t prt=0,maska=1;
	if(FirstHalfBajt==true)//je�eli ma by� przesy�any m�odszy p�bajt
	{
		if((dat&maska)==maska)//je�eli pierwszy bit zmiennej dat jest ustawiony
		prt|=(1<<DD4_LCD);//ustaw bit odpowiadaj�cy lini DD4_LCD
		maska=maska<<1;//przesu� zawarto�� zmiennej maska (tu 1) bitowo w lewo o jeden
		if((dat&maska)==maska)//powta�aj procedur� dla wszystkich czterech bit�w.
		prt|=(1<<DD5_LCD);
		maska=maska<<1;
		if((dat&maska)==maska)
		prt|=(1<<DD6_LCD);
		maska=maska<<1;
		if((dat&maska)==maska)
		prt|=(1<<DD7_LCD);
	}
	else//je�eli ma by� ustawiony starszy p�bajt
	{
		dat=dat>>4;//przesu� bitowo w prawo zawarto�� zmiennej dat o 4 bity w celu ustawienia starszego p�bajta jako m�odszy
		if((dat&maska)==maska)//powt�rz procedur� j.w.
		prt|=(1<<DD4_LCD);
		maska=maska<<1;
		if((dat&maska)==maska)
		prt|=(1<<DD5_LCD);
		maska=maska<<1;
		if((dat&maska)==maska)
		prt|=(1<<DD6_LCD);
		maska=maska<<1;
		if((dat&maska)==maska)
		prt|=(1<<DD7_LCD);
	}
	return prt;//port LCD
}
void Init(void)
{
	DDRC=39;//WY: PC 0 1 2 5
	DDRB=0;
	PORTB=0xFF;
	PORTC=31;//Zapalone wszystkie opr�cz PC 7 6 5
}
int8_t CheckBuf(char txt[],bool dotbut)
{
	int8_t q=0;//pomocniczy licznik
	for(int8_t i=0; i<16; i++)
	{
		if(txt[i]==0||(dotbut==false&&txt[i]=='.'))//je�eli w tablicy txt jest 0 lub kropka kt�rej ma nie by�
		break;//przerwij p�tl�
		q=i;//ustaw pomocniczy, g��wnym licznikiem p�tli
		q++;//zwi�ksz go o 1
	}
	if(q==0)//Zawsze zwr�� przynajmniej 1 �eby wy�wietlacz wy�wietli� pierwszy znak
	return 1;
	else
	return q;//Zwr�� liczb� wanych danych w buforze
}
int8_t klawisz=255, operacja=0;
bool druga=false,kropka=false;
double LiczbaA=0.0, LiczbaB=0.0, Wynik=0.0;
double LicznikKropki=1.0;
char bufor [16]={0};
int main(void)
{
	_delay_ms(500);
	Init();
	WlaczLCD();
	char hellotxt[5]="Witaj";
	WyswietlLCD(hellotxt,5);
	_delay_ms(1000);
	CzyscLCD();
	//int licznik=0;//Do usuni�cia po zdebbugowaniu
    while(1)
    {
		klawisz=SprawdzKlawisz();
        Dzialanie(klawisz); 
    }
}
void WyslijLCD(int8_t bajtmask)
{
	int8_t maska=255;//Chcemy mie� mask� ze wszystkimi ustawionymi bitami
	maska &= ~(1<<DD4_LCD);//zerujemy bity odpowiadaj�ce liniom danych
	maska &= ~(1<<DD5_LCD);//
	maska &= ~(1<<DD6_LCD);//
	maska &= ~(1<<DD7_LCD);//
	PORT_LCD=PORT_LCD&maska;//Operacja AND powoduje wyzerowanie bit�w portu tam gdzie w masce by�y zera a pozostawienie bez zmian
	//tam gdzie by�y jedynki (w masce)
	PORT_LCD |= (1<<EN_LCD);//Ustawiamy bit EN w celu przes�ania wa�nych danych do wy�wietlacza
	PORT_LCD|= Insert(bajtmask,false);//Ustawiamy pierwszy p�bajt (starszy) na porcie LCD (operacja sumy logicznej dla tego musz� tam by�
	//same zera.)
	_delay_us(10);//Op�nienie linii
	PORT_LCD &= ~(_BV(EN_LCD));// Ko�czymy transmisj� pierwszego p�bajtu
	_delay_us(1);// Op�nienie lini
	PORT_LCD=PORT_LCD&maska;//Ponownie zerujemy lini� danych
	PORT_LCD |= (1<<EN_LCD);//Ustawiamy EN w celu rozpocz�cia wa�nej transmisji danych
	PORT_LCD|= Insert(bajtmask,true);// Ustawiamy pierwszy p�bajt (m�odszy) na porcie LCD (operacja sumy logicznej dla tego musz� tam by�
	//same zera.)
	_delay_us(10);//Op�nienie linii
	PORT_LCD &=~ (_BV(EN_LCD));// Koniec transmisji zerujemy bit EN
	_delay_us(40);// op�nienie linii
	
	
}
void CzyscLCD()
{
	
	PORT_LCD &= ~(_BV(RS_LCD));//Zerujemy RS aby mie� dost�p do polece� wy�wietlacza (transmisja nie wysy�a danych do wy�wietlenia)
	WyslijLCD(1);//Czy��
	PORT_LCD |= (_BV(RS_LCD));// Wr�� do trybu transmisji w celu wy�wietlania
	_delay_ms(2);
	
}
void SetLine(int8_t line)
{
	PORT_LCD &= ~(_BV(RS_LCD));//Zerujemy RS aby mie� dost�p do polece� wy�wietlacza (transmisja nie wysy?a danych do wy�wietlenia)
	switch (line)
	{
		case 1:
		WyslijLCD(0xC0);
		break;
		case 2:
		WyslijLCD(0x94);
		break;
		case 3:
		WyslijLCD(0xD4);
		break;
		default:
		WyslijLCD(0x80);//Linia pierwsza
		break;
	}
	PORT_LCD |= (_BV(RS_LCD));// Wr�� do trybu transmisji w celu wy�wietlania
	_delay_ms(2);
}

void WyswietlLCD(char napis[],int8_t ile)
{
	int8_t k=0,i=0;
	while(k<ile)// Wy�wietl po kolei kazdy znak znajduj�cy si� w tablicy
	{
		i=(int8_t)napis[k];//rzutuj typ znakowy na jego reprezentacj� w formie liczby
		WyslijLCD(i);//Wy�lij do LCD
		k++;
	}
	
}
void WlaczLCD()
{
	DDR_LCD=255;//Port LCD jako wyj�cie
	PORT_LCD=0;//Pocz�tkowy stan zerowy linii
	_delay_ms(100);//op�nienie linii
	PORT_LCD|=(1<<A);//w��cz LED
	PORT_LCD&=~(1<<RW_LCD);
	PORT_LCD&=~(_BV(RS_LCD));//Tryb polece� wy�wietlacza
	//Dalej procedura inicjalizacji wy�wietlacza (patrz nota katalogowa HD 44780 wy�wietlanie w trybie 4'ro bitowym)
	//Wy�lij sekwencj� 0011
	for(int i=0;i<3;i=i+1)
	{
		PORT_LCD |= (1<<EN_LCD);
		
		PORT_LCD |= (1<<DD4_LCD)|(1<<DD5_LCD);
		_delay_us(10);//Op�nienie linii===
		PORT_LCD &= ~(_BV(EN_LCD));
		
		_delay_us(100);
	}
	//4 bitowy interfejs
	PORT_LCD |= (1<<EN_LCD);
	PORT_LCD &= ~(_BV(DD4_LCD));
	_delay_us(10);//Op�nienie linii===
	PORT_LCD &= ~(_BV(EN_LCD));
	//Parametry Wy�wietlacza
	PORT_LCD&=~(_BV(RS_LCD));
	WyslijLCD(0b00101000);
	PORT_LCD|=_BV(RS_LCD);
	_delay_us(1);
	//Parametry Wy�wietlacza
	PORT_LCD&=~(_BV(RS_LCD));
	WyslijLCD(0b00000110);
	PORT_LCD|=_BV(RS_LCD);
	_delay_us(1);
	//W��cz wy�wietlacz
	PORT_LCD&=~(_BV(RS_LCD));
	WyslijLCD(0b00001110);
	PORT_LCD|=_BV(RS_LCD);
	_delay_ms(1);
	//Czy�� wy�wietlacz
	PORT_LCD&=~(_BV(RS_LCD));
	WyslijLCD(0b00000001);
	PORT_LCD|=_BV(RS_LCD);
	_delay_ms(2);
	
}
void ZerujKalkulator()
{
	klawisz=255; operacja=0;
	druga=false;kropka=false;
	LiczbaA=0.0; LiczbaB=0.0; Wynik=0.0;
	LicznikKropki=1.0;
	for(int i=0; i<16; i++)
	bufor[i]=0;
}
int8_t SprawdzKlawisz(void)
{
	while(true)//p�tla czekaj�ca na wci�ni�cie przycisku
	{
		PORTC|=(1<<PC0);
		PORTC&=~(1<<PC2);
		_delay_us(10);
		if(bit_is_clear(PINB,6))
		{
			_delay_ms(120);//Poczekaj 120 ms na ustalenie si� stanu nieustalonego
			if(bit_is_clear(PINB,6))// czy na pewno przycisk zosta� wci�ni�ty? Czy tylko drgania styk�w.
			{
				return plusminus;
			}
		}
		if(bit_is_clear(PINC,3))
		{
			_delay_ms(120);//Poczekaj 120 ms na ustalenie si� stanu nieustalonego
			if(bit_is_clear(PINC,3))// czy na pewno przycisk zosta� wci�ni�ty? Czy tylko drgania styk�w.
			{
				return dotbutton;
			}
		}
		if(bit_is_clear(PINC,4))
		{
			_delay_ms(120);//Poczekaj 120 ms na ustalenie si� stanu nieustalonego
			if(bit_is_clear(PINC,4))// czy na pewno przycisk zosta� wci�ni�ty? Czy tylko drgania styk�w.
			{
				return CCE;
			}
		}
		if(bit_is_clear(PINB,7))
		{
			_delay_ms(120);//Poczekaj 120 ms na ustalenie si� stanu nieustalonego
			if(bit_is_clear(PINB,7))// czy na pewno przycisk zosta� wci�ni�ty? Czy tylko drgania styk�w.
			{
				return DP;
			}
		}
		if(bit_is_clear(PINB,3))
		{
			_delay_ms(120);//Poczekaj 120 ms na ustalenie si� stanu nieustalonego
			if(bit_is_clear(PINB,3))// czy na pewno przycisk zosta� wci�ni�ty? Czy tylko drgania styk�w.
			{
				return MR;
			}
		}
		if(bit_is_clear(PINB,1))
		{
			_delay_ms(120);//Poczekaj 120 ms na ustalenie si� stanu nieustalonego
			if(bit_is_clear(PINB,1))// czy na pewno przycisk zosta� wci�ni�ty? Czy tylko drgania styk�w.
			{
				return MPlus;
			}
		}
		if(bit_is_clear(PINB,4))
		{
			_delay_ms(120);//Poczekaj 120 ms na ustalenie si� stanu nieustalonego
			if(bit_is_clear(PINB,4))// czy na pewno przycisk zosta� wci�ni�ty? Czy tylko drgania styk�w.
			{
				return equal;
			}
		}
		if(bit_is_clear(PINB,0))
		{
			_delay_ms(120);//Poczekaj 120 ms na ustalenie si� stanu nieustalonego
			if(bit_is_clear(PINB,0))// czy na pewno przycisk zosta� wci�ni�ty? Czy tylko drgania styk�w.
			{
				return MC;
			}
		}
		PORTC|=(1<<PC2);
		PORTC&=~(1<<PC1);
		_delay_us(10);
		if(bit_is_clear(PINC,3))
		{
			_delay_ms(120);//Poczekaj 120 ms na ustalenie si� stanu nieustalonego
			if(bit_is_clear(PINC,3))// czy na pewno przycisk zosta� wci�ni�ty? Czy tylko drgania styk�w.
			{
				return 0;
			}
		}
		if(bit_is_clear(PINB,6))
		{
			_delay_ms(120);//Poczekaj 120 ms na ustalenie si� stanu nieustalonego
			if(bit_is_clear(PINB,6))// czy na pewno przycisk zosta� wci�ni�ty? Czy tylko drgania styk�w.
			{
				return 1;
			}
		}
		if(bit_is_clear(PINB,7))
		{
			_delay_ms(120);//Poczekaj 120 ms na ustalenie si� stanu nieustalonego
			if(bit_is_clear(PINB,7))// czy na pewno przycisk zosta� wci�ni�ty? Czy tylko drgania styk�w.
			{
				return 2;
			}
		}
		if(bit_is_clear(PINB,4))
		{
			_delay_ms(120);//Poczekaj 120 ms na ustalenie si� stanu nieustalonego
			if(bit_is_clear(PINB,4))// czy na pewno przycisk zosta� wci�ni�ty? Czy tylko drgania styk�w.
			{
				return 3;
			}
		}
		if(bit_is_clear(PINC,4))
		{
			_delay_ms(120);//Poczekaj 120 ms na ustalenie si� stanu nieustalonego
			if(bit_is_clear(PINC,4))// czy na pewno przycisk zosta� wci�ni�ty? Czy tylko drgania styk�w.
			{
				return 4;
			}
		}
		if(bit_is_clear(PINB,5))
		{
			_delay_ms(120);//Poczekaj 120 ms na ustalenie si� stanu nieustalonego
			if(bit_is_clear(PINB,5))// czy na pewno przycisk zosta� wci�ni�ty? Czy tylko drgania styk�w.
			{
				return 5;
			}
		}
		if(bit_is_clear(PINB,3))
		{
			_delay_ms(120);//Poczekaj 120 ms na ustalenie si� stanu nieustalonego
			if(bit_is_clear(PINB,3))// czy na pewno przycisk zosta� wci�ni�ty? Czy tylko drgania styk�w.
			{
				return 6;
			}
		}
		if(bit_is_clear(PINB,1))
		{
			_delay_ms(120);//Poczekaj 120 ms na ustalenie si� stanu nieustalonego
			if(bit_is_clear(PINB,1))// czy na pewno przycisk zosta� wci�ni�ty? Czy tylko drgania styk�w.
			{
				return 7;
			}
		}
		if(bit_is_clear(PINB,2))
		{
			_delay_ms(120);//Poczekaj 120 ms na ustalenie si� stanu nieustalonego
			if(bit_is_clear(PINB,2))// czy na pewno przycisk zosta� wci�ni�ty? Czy tylko drgania styk�w.
			{
				return 8;
			}
		}
		if(bit_is_clear(PINB,0))
		{
			_delay_ms(120);//Poczekaj 120 ms na ustalenie si� stanu nieustalonego
			if(bit_is_clear(PINB,0))// czy na pewno przycisk zosta� wci�ni�ty? Czy tylko drgania styk�w.
			{
				return 9;
			}
		}
		PORTC|=(1<<PC1);
		PORTC&=~(1<<PC0);
		_delay_us(10);
		if(bit_is_clear(PINB,5))
		{
			_delay_ms(120);//Poczekaj 120 ms na ustalenie si� stanu nieustalonego
			if(bit_is_clear(PINB,5))// czy na pewno przycisk zosta� wci�ni�ty? Czy tylko drgania styk�w.
			{
				return add;
			}
		}
		if(bit_is_clear(PINB,3))
		{
			_delay_ms(120);//Poczekaj 120 ms na ustalenie si� stanu nieustalonego
			if(bit_is_clear(PINB,3))// czy na pewno przycisk zosta� wci�ni�ty? Czy tylko drgania styk�w.
			{
				return substr;
			}
		}
		if(bit_is_clear(PINB,0))
		{
			_delay_ms(120);//Poczekaj 120 ms na ustalenie si� stanu nieustalonego
			if(bit_is_clear(PINB,0))// czy na pewno przycisk zosta� wci�ni�ty? Czy tylko drgania styk�w.
			{
				return percent;
			}
		}
		if(bit_is_clear(PINB,6))
		{
			_delay_ms(120);//Poczekaj 120 ms na ustalenie si� stanu nieustalonego
			if(bit_is_clear(PINB,6))// czy na pewno przycisk zosta� wci�ni�ty? Czy tylko drgania styk�w.
			{
				return mul;
			}
		}
		if(bit_is_clear(PINC,3))
		{
			_delay_ms(120);//Poczekaj 120 ms na ustalenie si� stanu nieustalonego
			if(bit_is_clear(PINC,3))// czy na pewno przycisk zosta� wci�ni�ty? Czy tylko drgania styk�w.
			{
				return sqrroot;
			}
		}
		if(bit_is_clear(PINB,7))
		{
			_delay_ms(120);//Poczekaj 120 ms na ustalenie si� stanu nieustalonego
			if(bit_is_clear(PINB,7))// czy na pewno przycisk zosta� wci�ni�ty? Czy tylko drgania styk�w.
			{
				return div;
			}
		}
	}
	return 255;
}
void SecretMode()
{
	uint8_t znak=0xFF;
	int klaw=0;
	static unsigned int poz=0;
	char DataStr[16];
    for(poz=0; poz<32; poz++)
    {
	    znak=eeprom_read_byte((uint8_t*)poz);
	    WyslijLCD(znak);
	    if(poz==15)
	    {
		    SetLine(1);
	    }
    }
	while(klaw!=CCE)
	{
		klaw=SprawdzKlawisz();
		if(klaw==2&&poz<(512-32))
		{
			SetLine(0);
			CzyscLCD();
			eeprom_read_block((void*)&DataStr, (const void*)poz,16);
			poz+=16;
			WyswietlLCD(DataStr,16);
	 
			SetLine(1);
			eeprom_read_block((void*)&DataStr, (const void*)poz,16);
			poz+=16;
			WyswietlLCD(DataStr,16);
			
		}
		if(klaw==8&&poz>=32)
		{
			SetLine(0);
			CzyscLCD();
			poz-=32;
			eeprom_read_block((void*)&DataStr, (const void*)poz,16);
			WyswietlLCD(DataStr,16);
			
			SetLine(1);
			poz+=16;
			eeprom_read_block((void*)&DataStr, (const void*)poz,16);
			WyswietlLCD(DataStr,16);
			
		}
	}
	ZerujKalkulator();
	CzyscLCD();
}
void Dzialanie(int8_t klaw)
{
	if(klaw!=10&&klaw!=11&&klaw!=12&&klaw!=13&&klaw!=14&&klaw!=15&&klaw!=16&&klaw!=17&&klaw!=18&&klaw!=19&&klaw!=20&&klaw!=21&&klaw!=22&&klaw!=23)
		{
			if(druga==false)//Je�eli wpisujemy pierwsz� liczb�
			{
				
				if(kropka==false)// Je�eli nie jest to liczba po kropce
				{
					LiczbaA=LiczbaA*10;//Przesu� liczb� w lewo (wymn� przez 10)
					LiczbaA+=klaw;// Dodaj liczb� odpowiadaj�c� wci�ni�temu klawiszowi tu 7.
				}
				else//A je�eli jest to liczba po kropce
				{
					LiczbaA+=(double)klaw/pow(10.0,LicznikKropki);//Dodaj liczb� odpowiadaj�c� wci�ni�temu klawiszowi tu 1, przesuni�t� i tyle miejsc w prawo
					//od przecinka ile odnotowano wci�ni�� danego klawisza
				}
				dtostrf(LiczbaA,10,8,bufor);//Zamie� liczb� typu double na ci�g znak�w do wy�wietlenia
				CzyscLCD();
				WyswietlLCD(bufor,CheckBuf(bufor,kropka));//wy�wietl ci�g znak�w
			}
			else// analogicznie jak wy�ej dla drugiej liczby...
			{
				
				if(kropka==false)
				{
					LiczbaB=LiczbaB*10;
					LiczbaB+=klaw;
				}
				else
				{
					LiczbaB+=(double)klaw/pow(10.0,LicznikKropki);
				}
				dtostrf(LiczbaB,10,8,bufor);
				CzyscLCD();
				WyswietlLCD(bufor,CheckBuf(bufor,kropka));
			}
			if(kropka==true)
			LicznikKropki++;
		}
		switch(klaw)
		{
		case MC:
		PORT_LCD^=0b00000010;
		break;
		case MPlus:
		druga=true;//Zaznacz �e od teraz wpisujemy drug� liczb�
		LicznikKropki=1;//licznik kropki znowu od 1
		kropka=false;// wpisujemy now� liczb� nie musi to by� liczba po przecinku
		CzyscLCD();
		dtostrf(LiczbaB,10,8,bufor);
		CzyscLCD();
		WyswietlLCD(bufor,CheckBuf(bufor,kropka));
		break;
		case DP:
		if(LiczbaA==72469&&LiczbaB==116225)
		{
		operacja=5;
		CzyscLCD();	
		}
		break;
		case plusminus:
		if(druga==false)
		{
			LiczbaA=LiczbaA*(-1);
			dtostrf(LiczbaA,10,8,bufor);
			CzyscLCD();
			WyswietlLCD(bufor,CheckBuf(bufor,kropka));
		}
		else
		{
			LiczbaB=LiczbaB*(-1);
			dtostrf(LiczbaB,10,8,bufor);
			CzyscLCD();
			WyswietlLCD(bufor,CheckBuf(bufor,kropka));
		}
		break;	
		case sqrroot:
		Wynik=sqrt(LiczbaA);
		CzyscLCD();
		dtostrf(Wynik,7,5,bufor);
		WyswietlLCD(bufor,CheckBuf(bufor,true));
		ZerujKalkulator();
		break;
		case mul: //operacja mno�enia nr 12
		operacja=1;//Operacja nr 1 ty mno�enie
		druga=true;//Zaznacz �e od teraz wpisujemy drug� liczb�
		LicznikKropki=1;//licznik kropki znowu od 1
		kropka=false;// wpisujemy now� liczb� nie musi to by� liczba po przecinku
		CzyscLCD();
		dtostrf(LiczbaB,10,8,bufor);
		CzyscLCD();
		WyswietlLCD(bufor,CheckBuf(bufor,kropka));
		break;
		case substr://odejmowanie
		operacja=2;//operacja nr 2
		druga=true;//zaznacz �e od teraz wpisujemy druga liczb�
		LicznikKropki=1;//w przypadku liczby po przecinku wpisuj od pierwszego miejsca
		kropka=false;//zacznij od liczb za�kowitych
		CzyscLCD();
		dtostrf(LiczbaB,10,8,bufor);
		CzyscLCD();
		WyswietlLCD(bufor,CheckBuf(bufor,kropka));
		break;
		case div:
		operacja=3;//dzielenie
		druga=true;
		LicznikKropki=1;
		kropka=false;
		CzyscLCD();
		dtostrf(LiczbaB,10,8,bufor);
		CzyscLCD();
		WyswietlLCD(bufor,CheckBuf(bufor,kropka));
		break;
		case dotbutton://obs�uga kropki (liczby zmienoprzecinkowe
		kropka=true;//zaznacz �e kolejno wpisane liczby maj� by� po przecinku
		break;
		case equal://(r�wna si�) wy�wietl wynik operacji
		Wynik=0.0;//zeruj wynik
		switch(operacja)
		{
			case 1:
			Wynik=LiczbaA*LiczbaB;
			break;
			case 2:
			Wynik=LiczbaA-LiczbaB;
			break;
			case 3:
			Wynik=LiczbaA/LiczbaB;
			break;
			case 4:
			Wynik=LiczbaA+LiczbaB;
			break;
			case 5:
			SecretMode();
			break;
		}
		CzyscLCD();
		dtostrf(Wynik,7,5,bufor);
		WyswietlLCD(bufor,CheckBuf(bufor,true));
		ZerujKalkulator();
		break;
		case add://dodawanie
		operacja=4;
		druga=true;
		LicznikKropki=1;
		kropka=false;
		CzyscLCD();
		dtostrf(LiczbaB,10,8,bufor);
		CzyscLCD();
		WyswietlLCD(bufor,CheckBuf(bufor,kropka));
		break;
		case CCE:
		ZerujKalkulator();
		CzyscLCD();
		dtostrf(LiczbaB,10,8,bufor);
		CzyscLCD();
		WyswietlLCD(bufor,CheckBuf(bufor,kropka));
		break;
		case MR:
		CzyscLCD();
		_delay_ms(500);
		WlaczLCD();
		CzyscLCD();
		break;
		           default:
		           break;
	}
	
}