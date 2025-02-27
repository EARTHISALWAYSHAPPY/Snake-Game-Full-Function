//-----free-time-----//
//-----free-time-----//
//-----free-time-----//
//-----free-time-----//
//-----free-time-----//
#include <Arduino.h>
#include <avr/io.h>
#define F_CPU 16000000UL
#define INT0_vect _VECTOR(1)
#define INT1_vect _VECTOR(2)
#define TIMER0_OVF_vect _VECTOR(16)
#define TIMER1_OVF_vect _VECTOR(13)
#define UART_RX_vect _VECTOR(18)

#define CS PB2
#define MOSI PB3
#define SCK PB5

char ctrl;
#define limit 63
int x[limit] = {6, 7}, y[limit] = {7, 7}; // default row7 : 11110000

int horizontal = -1, vertical = 0; // default shift right -->
uint8_t pos_x_score = 0, pos_y_score = 0, snake = 2;

void setting_interrupt()
{
  // use sw1(D2) : INT0, sw2(D3) : INT1, Timer1
  EICRA = EICRA | 0b00001010;         // failling edge
  EIMSK |= (1 << INT1) | (1 << INT0); // enable INT1 , INT0
  TCCR0A = 0b00000000;
  TCCR0B = 0b00000101; // prescale 1024
  TIMSK0 = 0b00000001;
  TCNT0 = 0;
  TCCR1A = 0b00000000;
  TCCR1B = 0b00000101; // prescale 1024
  TIMSK1 = 0b00000001;
  TCNT1 = 62000;
}

void Serial_begin(int baudrate)
{
  UBRR0 = baudrate; // UBRRn = ( 16x10^6 / (16 x baud rate(19200))) - 1
  UCSR0A = 0b00000000;
  UCSR0B = 0b10011000;
  UCSR0C = 0b00000110;
  sei();
}
void Serial_putc(char data)
{
  char busy;
  do
  {
    busy = UCSR0A & 0b00100000;
  } while (busy == 0);
  UDR0 = data;
}
char Serial_getc()
{
  char busy;
  do
  {
    busy = UCSR0A & 0b10000000;
  } while (busy == 0);
  return (UDR0);
}

void Serial_puts(const char *data)
{
  while (*data)
  {
    Serial_putc(*data++);
  }
}

void spi_init()
{
  DDRB |= (1 << SCK) | (1 << MOSI) | (1 << CS);  // Set MOSI, SCK, CS as Output
  SPCR = (1 << SPE) | (1 << MSTR) | (1 << SPR0); // Enable SPI, Master Mode, Clock /16
}

void spi_putc(char data)
{
  SPDR = data;
  while (!(SPSR & (1 << SPIF)))
    ; // Wait for transmission complete
}

void max7219_wr(char addr, char data)
{
  PORTB &= ~(1 << CS); // SS = 0
  spi_putc(addr);
  spi_putc(data);
  PORTB |= (1 << CS); // SS = 1
}

void max7219_config()
{
  max7219_wr(0x0C, 0x00); // Shutdown
  max7219_wr(0x09, 0x00); // Decode mode
  max7219_wr(0x0A, 0x15); // Intensity
  max7219_wr(0x0B, 0x07); // Scan limit
  max7219_wr(0x0C, 0x01); // Turn on
}

void display()
{
  // update body snake
  byte matrix[8] = {0};
  for (int i = 0; i < snake; i++)
  {
    matrix[y[i]] |= (1 << x[i]);
  }

  // put dot for score
  // matrix[pos_y_score] |= (1 << pos_x_score);

  // show overall
  for (int row = 0; row < 8; row++)
  {
    max7219_wr(row + 1, matrix[row]);
  }
}

void put_score()
{
  pos_x_score = random(0, 8);
  pos_y_score = random(0, 8);
}
void check_get()
{
  // head eatting score
  if (x[0] == pos_x_score && y[0] == pos_y_score)
  {
    snake++;
    put_score();
  }
}
void condition_ctrl()
{
  switch (ctrl)
  {
  case 'A':
    if (horizontal == 0)
    {
      horizontal = 1;
      vertical = 0;
    }
    break;
  case 'D':
    if (horizontal == 0)
    {
      horizontal = -1;
      vertical = 0;
    }
    break;
  case 'W':
    if (vertical == 0)
    {
      horizontal = 0;
      vertical = -1;
    }
    break;
  case 'S':
    if (vertical == 0)
    {
      horizontal = 0;
      vertical = 1;
    }
    break;
  default:

    break;
  }
}

void move()
{
  // update x,y data dot frame by frame
  for (int position = snake - 1; position > 0; position--)
  {
    x[position] = x[position - 1];
    y[position] = y[position - 1];
  }
  // shift led for update
  x[0] += horizontal;
  y[0] += vertical;
  // horizontal กันหลุดจอ
  if (x[0] < 0)
    x[0] = 7;
  if (x[0] > 7)
    x[0] = 0;
  // vertical กันหลุดจอ
  if (y[0] < 0)
    y[0] = 7;
  if (y[0] > 7)
    y[0] = 0;
  condition_ctrl();
  check_get();
  display();
}

// ISR(INT0_vect)
// {
//   // shift right (default)
//   horizontal = 1, vertical = 0;
// }

// ISR(INT1_vect)
// {
//   // shift up
//   horizontal = 0, vertical = 1;
// }

ISR(UART_RX_vect)
{
  ctrl = Serial_getc();
  Serial_putc(ctrl);
  Serial_putc('\n');
}

ISR(TIMER0_OVF_vect)
{
  byte matrix[8] = {};
  for (int i = 0; i < snake; i++)
  {
    matrix[y[i]] |= (1 << x[i]);
  }
  matrix[pos_y_score] ^= (1 << pos_x_score);
  for (int row = 0; row < 8; row++)
  {
    max7219_wr(row + 1, matrix[row]);
  }
  TCNT0 = 0;
}

ISR(TIMER1_OVF_vect)
{
  move();
  TCNT1 = 62000;
}
void clear()
{
  for (int row = 0; row <= 8; row++)
  {
    max7219_wr(row + 1, 0x00);
  }
}
int main()
{
  Serial_begin(25);
  Serial_puts("Enter W A S D : ");
  Serial_puts("\r\n");

  setting_interrupt();
  spi_init();

  max7219_config();

  clear();
  sei();
  put_score();

  while (1)
  {
  }
}
/*
row
-------------->
7 6 5 4 3 2 1 0
7 6 5 4 3 2 1 0
7 6 5 4 3 2 1 0
7 6 5 4 3 2 1 0
7 6 5 4 3 2 1 0
7 6 5 4 3 2 1 0
7 6 5 4 3 2 1 0
7 6 5 4 3 2 1 0
colume
0 0 0 0 0 0 0 ^
1 1 1 1 1 1 1 |
2 2 2 2 2 2 2 |
3 3 3 3 3 3 3 |
4 4 4 4 4 4 4 |
5 5 5 5 5 5 5 |
6 6 6 6 6 6 6 |
7 7 7 7 7 7 7 |
*/