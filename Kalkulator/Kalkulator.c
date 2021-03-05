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
//zdefiniuj odpowiednie sta³e portów:
#define DDR_LCD DDRD
#define PORT_LCD PORTD
#define RS_LCD 2// Dla linii RS bit nr 2
#define RW_LCD 0// Dla linii RW bit nr 0
#define EN_LCD 3// Dla linii EN bit nr 3
#define DD4_LCD 4// Dla Linii danych bitu 4 bit nr 4
#define DD5_LCD 5// Dla Linii danych bitu 5 bit nr 5
#define DD6_LCD 6// Dla Linii danych bitu 6 bit nr 6
#define DD7_LCD 7// Dla Linii danych bitu 7 bit nr 7
#define A 1// Dla Anody diody wyœwietlacza bit nr 1
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
int8_t Insert(int8_t dat,bool FirstHalfBajt)//Ustaw odpowiednie bity linii danych (tryb 4'ro bitowy) pozostawi¹j¹c resztê jako 0, dziêki czemu mo¿na u¿yæ operacji sumy logicznej.
{
	int8_t prt=0,maska=1;
	if(FirstHalfBajt==true)//je¿eli ma byæ przesy³any m³odszy pó³bajt
	{
		if((dat&maska)==maska)//je¿eli pierwszy bit zmiennej dat jest ustawiony
		prt|=(1<<DD4_LCD);//ustaw bit odpowiadaj¹cy lini DD4_LCD
		maska=maska<<1;//przesuñ zawartoœæ zmiennej maska (tu 1) bitowo w lewo o jeden
		if((dat&maska)==maska)//powta¿aj procedurê dla wszystkich czterech bitów.
		prt|=(1<<DD5_LCD);
		maska=maska<<1;
		if((dat&maska)==maska)
		prt|=(1<<DD6_LCD);
		maska=maska<<1;
		if((dat&maska)==maska)
		prt|=(1<<DD7_LCD);
	}
	else//je¿eli ma byæ ustawiony starszy pó³bajt
	{
		dat=dat>>4;//przesuñ bitowo w prawo zawartoœæ zmiennej dat o 4 bity w celu ustawienia starszego pó³bajta jako m³odszy
		if((dat&maska)==maska)//powtórz procedurê j.w.
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
	PORTC=31;//Zapalone wszystkie oprócz PC 7 6 5
}
int8_t CheckBuf(char txt[],bool dotbut)
{
	int8_t q=0;//pomocniczy licznik
	for(int8_t i=0; i<16; i++)
	{
		if(txt[i]==0||(dotbut==false&&txt[i]=='.'))//je¿eli w tablicy txt jest 0 lub kropka której ma nie byæ
		break;//przerwij pêtlê
		q=i;//ustaw pomocniczy, g³ównym licznikiem pêtli
		q++;//zwiêksz go o 1
	}
	if(q==0)//Zawsze zwróæ przynajmniej 1 ¿eby wyœwietlacz wyœwietli³ pierwszy znak
	return 1;
	else
	return q;//Zwróæ liczbê wanych danych w buforze
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
	//int licznik=0;//Do usuniêcia po zdebbugowaniu
    while(1)
    {
		klawisz=SprawdzKlawisz();
        Dzialanie(klawisz); 
    }
}
void WyslijLCD(int8_t bajtmask)
{
	int8_t maska=255;//Chcemy mieæ maskê ze wszystkimi ustawionymi bitami
	maska &= ~(1<<DD4_LCD);//zerujemy bity odpowiadaj¹ce liniom danych
	maska &= ~(1<<DD5_LCD);//
	maska &= ~(1<<DD6_LCD);//
	maska &= ~(1<<DD7_LCD);//
	PORT_LCD=PORT_LCD&maska;//Operacja AND powoduje wyzerowanie bitów portu tam gdzie w masce by³y zera a pozostawienie bez zmian
	//tam gdzie by³y jedynki (w masce)
	PORT_LCD |= (1<<EN_LCD);//Ustawiamy bit EN w celu przes³ania wa¿nych danych do wyœwietlacza
	PORT_LCD|= Insert(bajtmask,false);//Ustawiamy pierwszy pó³bajt (starszy) na porcie LCD (operacja sumy logicznej dla tego musz¹ tam byæ
	//same zera.)
	_delay_us(10);//OpóŸnienie linii
	PORT_LCD &= ~(_BV(EN_LCD));// Koñczymy transmisjê pierwszego pó³bajtu
	_delay_us(1);// OpóŸnienie lini
	PORT_LCD=PORT_LCD&maska;//Ponownie zerujemy liniê danych
	PORT_LCD |= (1<<EN_LCD);//Ustawiamy EN w celu rozpoczêcia wa¿nej transmisji danych
	PORT_LCD|= Insert(bajtmask,true);// Ustawiamy pierwszy pó³bajt (m³odszy) na porcie LCD (operacja sumy logicznej dla tego musz¹ tam byæ
	//same zera.)
	_delay_us(10);//OpóŸnienie linii
	PORT_LCD &=~ (_BV(EN_LCD));// Koniec transmisji zerujemy bit EN
	_delay_us(40);// opóŸnienie linii
	
	
}
void CzyscLCD()
{
	
	PORT_LCD &= ~(_BV(RS_LCD));//Zerujemy RS aby mieæ dostêp do poleceñ wyœwietlacza (transmisja nie wysy³a danych do wyœwietlenia)
	WyslijLCD(1);//Czyœæ
	PORT_LCD |= (_BV(RS_LCD));// Wróæ do trybu transmisji w celu wyœwietlania
	_delay_ms(2);
	
}
void SetLine(int8_t line)
{
	PORT_LCD &= ~(_BV(RS_LCD));//Zerujemy RS aby mieŠ dostàp do polece½ wywietlacza (transmisja nie wysy?a danych do wywietlenia)
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
	PORT_LCD |= (_BV(RS_LCD));// Wr¡Š do trybu transmisji w celu wywietlania
	_delay_ms(2);
}

void WyswietlLCD(char napis[],int8_t ile)
{
	int8_t k=0,i=0;
	while(k<ile)// Wyœwietl po kolei kazdy znak znajduj¹cy siê w tablicy
	{
		i=(int8_t)napis[k];//rzutuj typ znakowy na jego reprezentacjê w formie liczby
		WyslijLCD(i);//Wyœlij do LCD
		k++;
	}
	
}
void WlaczLCD()
{
	DDR_LCD=255;//Port LCD jako wyjœcie
	PORT_LCD=0;//Pocz¹tkowy stan zerowy linii
	_delay_ms(100);//opóŸnienie linii
	PORT_LCD|=(1<<A);//w³¹cz LED
	PORT_LCD&=~(1<<RW_LCD);
	PORT_LCD&=~(_BV(RS_LCD));//Tryb poleceñ wyœwietlacza
	//Dalej procedura inicjalizacji wyœwietlacza (patrz nota katalogowa HD 44780 wyœwietlanie w trybie 4'ro bitowym)
	//Wyœlij sekwencjê 0011
	for(int i=0;i<3;i=i+1)
	{
		PORT_LCD |= (1<<EN_LCD);
		
		PORT_LCD |= (1<<DD4_LCD)|(1<<DD5_LCD);
		_delay_us(10);//OpóŸnienie linii===
		PORT_LCD &= ~(_BV(EN_LCD));
		
		_delay_us(100);
	}
	//4 bitowy interfejs
	PORT_LCD |= (1<<EN_LCD);
	PORT_LCD &= ~(_BV(DD4_LCD));
	_delay_us(10);//OpóŸnienie linii===
	PORT_LCD &= ~(_BV(EN_LCD));
	//Parametry Wyœwietlacza
	PORT_LCD&=~(_BV(RS_LCD));
	WyslijLCD(0b00101000);
	PORT_LCD|=_BV(RS_LCD);
	_delay_us(1);
	//Parametry Wyœwietlacza
	PORT_LCD&=~(_BV(RS_LCD));
	WyslijLCD(0b00000110);
	PORT_LCD|=_BV(RS_LCD);
	_delay_us(1);
	//W³¹cz wyœwietlacz
	PORT_LCD&=~(_BV(RS_LCD));
	WyslijLCD(0b00001110);
	PORT_LCD|=_BV(RS_LCD);
	_delay_ms(1);
	//Czyœæ wyœwietlacz
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
	while(true)//pêtla czekaj¹ca na wciœniêcie przycisku
	{
		PORTC|=(1<<PC0);
		PORTC&=~(1<<PC2);
		_delay_us(10);
		if(bit_is_clear(PINB,6))
		{
			_delay_ms(120);//Poczekaj 120 ms na ustalenie siê stanu nieustalonego
			if(bit_is_clear(PINB,6))// czy na pewno przycisk zosta³ wciœniêty? Czy tylko drgania styków.
			{
				return plusminus;
			}
		}
		if(bit_is_clear(PINC,3))
		{
			_delay_ms(120);//Poczekaj 120 ms na ustalenie siê stanu nieustalonego
			if(bit_is_clear(PINC,3))// czy na pewno przycisk zosta³ wciœniêty? Czy tylko drgania styków.
			{
				return dotbutton;
			}
		}
		if(bit_is_clear(PINC,4))
		{
			_delay_ms(120);//Poczekaj 120 ms na ustalenie siê stanu nieustalonego
			if(bit_is_clear(PINC,4))// czy na pewno przycisk zosta³ wciœniêty? Czy tylko drgania styków.
			{
				return CCE;
			}
		}
		if(bit_is_clear(PINB,7))
		{
			_delay_ms(120);//Poczekaj 120 ms na ustalenie siê stanu nieustalonego
			if(bit_is_clear(PINB,7))// czy na pewno przycisk zosta³ wciœniêty? Czy tylko drgania styków.
			{
				return DP;
			}
		}
		if(bit_is_clear(PINB,3))
		{
			_delay_ms(120);//Poczekaj 120 ms na ustalenie siê stanu nieustalonego
			if(bit_is_clear(PINB,3))// czy na pewno przycisk zosta³ wciœniêty? Czy tylko drgania styków.
			{
				return MR;
			}
		}
		if(bit_is_clear(PINB,1))
		{
			_delay_ms(120);//Poczekaj 120 ms na ustalenie siê stanu nieustalonego
			if(bit_is_clear(PINB,1))// czy na pewno przycisk zosta³ wciœniêty? Czy tylko drgania styków.
			{
				return MPlus;
			}
		}
		if(bit_is_clear(PINB,4))
		{
			_delay_ms(120);//Poczekaj 120 ms na ustalenie siê stanu nieustalonego
			if(bit_is_clear(PINB,4))// czy na pewno przycisk zosta³ wciœniêty? Czy tylko drgania styków.
			{
				return equal;
			}
		}
		if(bit_is_clear(PINB,0))
		{
			_delay_ms(120);//Poczekaj 120 ms na ustalenie siê stanu nieustalonego
			if(bit_is_clear(PINB,0))// czy na pewno przycisk zosta³ wciœniêty? Czy tylko drgania styków.
			{
				return MC;
			}
		}
		PORTC|=(1<<PC2);
		PORTC&=~(1<<PC1);
		_delay_us(10);
		if(bit_is_clear(PINC,3))
		{
			_delay_ms(120);//Poczekaj 120 ms na ustalenie siê stanu nieustalonego
			if(bit_is_clear(PINC,3))// czy na pewno przycisk zosta³ wciœniêty? Czy tylko drgania styków.
			{
				return 0;
			}
		}
		if(bit_is_clear(PINB,6))
		{
			_delay_ms(120);//Poczekaj 120 ms na ustalenie siê stanu nieustalonego
			if(bit_is_clear(PINB,6))// czy na pewno przycisk zosta³ wciœniêty? Czy tylko drgania styków.
			{
				return 1;
			}
		}
		if(bit_is_clear(PINB,7))
		{
			_delay_ms(120);//Poczekaj 120 ms na ustalenie siê stanu nieustalonego
			if(bit_is_clear(PINB,7))// czy na pewno przycisk zosta³ wciœniêty? Czy tylko drgania styków.
			{
				return 2;
			}
		}
		if(bit_is_clear(PINB,4))
		{
			_delay_ms(120);//Poczekaj 120 ms na ustalenie siê stanu nieustalonego
			if(bit_is_clear(PINB,4))// czy na pewno przycisk zosta³ wciœniêty? Czy tylko drgania styków.
			{
				return 3;
			}
		}
		if(bit_is_clear(PINC,4))
		{
			_delay_ms(120);//Poczekaj 120 ms na ustalenie siê stanu nieustalonego
			if(bit_is_clear(PINC,4))// czy na pewno przycisk zosta³ wciœniêty? Czy tylko drgania styków.
			{
				return 4;
			}
		}
		if(bit_is_clear(PINB,5))
		{
			_delay_ms(120);//Poczekaj 120 ms na ustalenie siê stanu nieustalonego
			if(bit_is_clear(PINB,5))// czy na pewno przycisk zosta³ wciœniêty? Czy tylko drgania styków.
			{
				return 5;
			}
		}
		if(bit_is_clear(PINB,3))
		{
			_delay_ms(120);//Poczekaj 120 ms na ustalenie siê stanu nieustalonego
			if(bit_is_clear(PINB,3))// czy na pewno przycisk zosta³ wciœniêty? Czy tylko drgania styków.
			{
				return 6;
			}
		}
		if(bit_is_clear(PINB,1))
		{
			_delay_ms(120);//Poczekaj 120 ms na ustalenie siê stanu nieustalonego
			if(bit_is_clear(PINB,1))// czy na pewno przycisk zosta³ wciœniêty? Czy tylko drgania styków.
			{
				return 7;
			}
		}
		if(bit_is_clear(PINB,2))
		{
			_delay_ms(120);//Poczekaj 120 ms na ustalenie siê stanu nieustalonego
			if(bit_is_clear(PINB,2))// czy na pewno przycisk zosta³ wciœniêty? Czy tylko drgania styków.
			{
				return 8;
			}
		}
		if(bit_is_clear(PINB,0))
		{
			_delay_ms(120);//Poczekaj 120 ms na ustalenie siê stanu nieustalonego
			if(bit_is_clear(PINB,0))// czy na pewno przycisk zosta³ wciœniêty? Czy tylko drgania styków.
			{
				return 9;
			}
		}
		PORTC|=(1<<PC1);
		PORTC&=~(1<<PC0);
		_delay_us(10);
		if(bit_is_clear(PINB,5))
		{
			_delay_ms(120);//Poczekaj 120 ms na ustalenie siê stanu nieustalonego
			if(bit_is_clear(PINB,5))// czy na pewno przycisk zosta³ wciœniêty? Czy tylko drgania styków.
			{
				return add;
			}
		}
		if(bit_is_clear(PINB,3))
		{
			_delay_ms(120);//Poczekaj 120 ms na ustalenie siê stanu nieustalonego
			if(bit_is_clear(PINB,3))// czy na pewno przycisk zosta³ wciœniêty? Czy tylko drgania styków.
			{
				return substr;
			}
		}
		if(bit_is_clear(PINB,0))
		{
			_delay_ms(120);//Poczekaj 120 ms na ustalenie siê stanu nieustalonego
			if(bit_is_clear(PINB,0))// czy na pewno przycisk zosta³ wciœniêty? Czy tylko drgania styków.
			{
				return percent;
			}
		}
		if(bit_is_clear(PINB,6))
		{
			_delay_ms(120);//Poczekaj 120 ms na ustalenie siê stanu nieustalonego
			if(bit_is_clear(PINB,6))// czy na pewno przycisk zosta³ wciœniêty? Czy tylko drgania styków.
			{
				return mul;
			}
		}
		if(bit_is_clear(PINC,3))
		{
			_delay_ms(120);//Poczekaj 120 ms na ustalenie siê stanu nieustalonego
			if(bit_is_clear(PINC,3))// czy na pewno przycisk zosta³ wciœniêty? Czy tylko drgania styków.
			{
				return sqrroot;
			}
		}
		if(bit_is_clear(PINB,7))
		{
			_delay_ms(120);//Poczekaj 120 ms na ustalenie siê stanu nieustalonego
			if(bit_is_clear(PINB,7))// czy na pewno przycisk zosta³ wciœniêty? Czy tylko drgania styków.
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
			if(druga==false)//Je¿eli wpisujemy pierwsz¹ liczbê
			{
				
				if(kropka==false)// Je¿eli nie jest to liczba po kropce
				{
					LiczbaA=LiczbaA*10;//Przesuñ liczbê w lewo (wymnó¿ przez 10)
					LiczbaA+=klaw;// Dodaj liczbê odpowiadaj¹c¹ wciœniêtemu klawiszowi tu 7.
				}
				else//A je¿eli jest to liczba po kropce
				{
					LiczbaA+=(double)klaw/pow(10.0,LicznikKropki);//Dodaj liczbê odpowiadaj¹c¹ wciœniêtemu klawiszowi tu 1, przesuniêt¹ i tyle miejsc w prawo
					//od przecinka ile odnotowano wciœniêæ danego klawisza
				}
				dtostrf(LiczbaA,10,8,bufor);//Zamieñ liczbê typu double na ci¹g znaków do wyœwietlenia
				CzyscLCD();
				WyswietlLCD(bufor,CheckBuf(bufor,kropka));//wyœwietl ci¹g znaków
			}
			else// analogicznie jak wy¿ej dla drugiej liczby...
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
		druga=true;//Zaznacz ¿e od teraz wpisujemy drug¹ liczbê
		LicznikKropki=1;//licznik kropki znowu od 1
		kropka=false;// wpisujemy now¹ liczbê nie musi to byæ liczba po przecinku
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
		case mul: //operacja mno¿enia nr 12
		operacja=1;//Operacja nr 1 ty mno¿enie
		druga=true;//Zaznacz ¿e od teraz wpisujemy drug¹ liczbê
		LicznikKropki=1;//licznik kropki znowu od 1
		kropka=false;// wpisujemy now¹ liczbê nie musi to byæ liczba po przecinku
		CzyscLCD();
		dtostrf(LiczbaB,10,8,bufor);
		CzyscLCD();
		WyswietlLCD(bufor,CheckBuf(bufor,kropka));
		break;
		case substr://odejmowanie
		operacja=2;//operacja nr 2
		druga=true;//zaznacz ¿e od teraz wpisujemy druga liczbê
		LicznikKropki=1;//w przypadku liczby po przecinku wpisuj od pierwszego miejsca
		kropka=false;//zacznij od liczb za³kowitych
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
		case dotbutton://obs³uga kropki (liczby zmienoprzecinkowe
		kropka=true;//zaznacz ¿e kolejno wpisane liczby maj¹ byæ po przecinku
		break;
		case equal://(równa siê) wyœwietl wynik operacji
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