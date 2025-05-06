#define F_CPU 4000000UL
#define USART_BAUD_RATE(BAUD_RATE) ((float)(F_CPU * 64 / (16 *(float)BAUD_RATE)) + 0.5)
#define COMMON_BAUD_RATE 9600
#define TIMEOUT 10000
#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <stdlib.h>

// Funksjonprototyper
void USART2_init(unsigned long baud);
void USART3_init(unsigned long baud);
void USART3_sendChar(char c);
uint8_t USART2_read();
uint8_t USART3_read();
static int USART3_printChar(char c, FILE *stream);
static FILE USART_stream = FDEV_SETUP_STREAM(USART3_printChar, NULL, _FDEV_SETUP_WRITE);
void USART2_flush(void);
char generate_random_ascii(void);
void BFR();

char rx_char = '0';  
char tx_char;        

int main(void) {
    /* Setter PIN B3, LED0 på AVR-kortet som utgang. */ 
    PORTB.DIRSET = PIN3_bm;
    PORTB.OUT |= PIN3_bm; 

    //Initaliserer
    USART2_init(COMMON_BAUD_RATE);  
    USART3_init(COMMON_BAUD_RATE); 
    
    int i = 0;
    while (i < 10000) {
        tx_char =  generate_random(); 
        USART2_flush(); // Tømmer RX-bufferet på USART2
        USART3_sendChar(tx_char); 
        _delay_ms(0.5); // Liten delay for å gi tid til at RX kan lese

        rx_char = USART2_read(); // Leser data fra USART2
        
        if (rx_char == 0xFF) {  // Sjekk om det er en timeout
            printf("%c \n\r", rx_char);  
            PORTB.OUT |= PIN3_bm;  // Slår på LED ved timeout
            _delay_ms(500);  
            continue;
        }
        printf("\n\rTX: %c \n\r", tx_char);  
        printf("RX: %c \n\r", rx_char);     

        // Beregner og skriver ut bitfeilratio
        BFR(); 
        
        _delay_ms(100); 
        i++;
    }
}

// Funksjon for å beregne bitfeilratio
void BFR()
{
    static uint16_t charNum = 0; 
    static double bitErrorRatio = 0; 
    static uint16_t bitError = 0;  
    uint8_t temp1 = tx_char ^ rx_char;  // XOR mellom sendt og mottatt tegn

    for(int i = 0; i < 8; i++){  // Går gjennom hvert bit i tegnene
        if((temp1 / 128) >= 1){  // Sjekker om det er en bitfeil (første bit er 1)
            bitError++;  // Øker bitfeiltelleren
        }
        temp1 = temp1 << 1;  // Flytter bitene til venstre
    }

    charNum++;  // Øker antall sammenlignede tegn
    bitErrorRatio = (float)bitError / (float)(8 * charNum);  // Beregner bitfeilratio
    printf("%f\r\n", bitErrorRatio);  // Skriver ut bitfeilratio
}

// Funksjon for å generere enten 0 eller 1 
char generate_random(void) {
    return '0' + (rand() % 2);  
}

// Funksjon for å tømme RX-bufferet på USART2
void USART2_flush(void) {
    // Leser og forkaster data fra RX-bufferet
    while (USART2.STATUS & USART_RXCIF_bm) {
        volatile uint8_t dummy = USART2.RXDATAL;  // Leser og kaster bort data
    }
}

// Funksjon for å sende et tegn til USART3
static int USART3_printChar(char c, FILE *stream) {
    USART3_sendChar(c);  
    return 0;
}

// Initialiserer USART2 med spesifisert baud rate
void USART2_init(unsigned long baud) {
    PORTF.DIRSET = PIN4_bm;  
    PORTF.DIRCLR = PIN5_bm;  
    USART2.BAUD = (uint16_t) USART_BAUD_RATE(baud);  
    USART2.CTRLB |= USART_TXEN_bm | USART_RXEN_bm;  
    PORTMUX.USARTROUTEA |= PORTMUX_USART2_ALT1_gc;  // Ruter USART2 til riktige pinner på AVR
}

// Funksjon for å lese et tegn fra USART2
uint8_t USART2_read() {
    uint32_t timeout_counter = 0;  // Tellevariabel for timeout
    while (!(USART2.STATUS & USART_RXCIF_bm)) {  // Venter på at data skal komme inn
        timeout_counter++;  // Øker timeout-telleren
        if (timeout_counter >= TIMEOUT){  // Hvis timeout er nådd
            return 0xFF;  // Returnerer 0xFF ved timeout
        }
    }
    return USART2.RXDATAL;  // Returnerer mottatt data
}

// Initialiserer USART3 med spesifisert baud rate
void USART3_init(unsigned long baud) {
    PORTB.DIRSET = PIN0_bm;  
    PORTB.DIRCLR = PIN1_bm;  
    USART3.BAUD = (uint16_t) USART_BAUD_RATE(baud);  
    USART3.CTRLB |= USART_TXEN_bm | USART_RXEN_bm;  
    
    stdout = &USART_stream;  // Setter opp stdout til å bruke USART3
}

// Funksjon for å sende et tegn til USART3
void USART3_sendChar(char c) {
    while (!(USART3.STATUS & USART_DREIF_bm)) // Venter på at data-registeret er klart til å sende
    {
        ;
    }
    USART3.TXDATAL = c; // Sender tegnet til USART3
}
