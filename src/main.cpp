//-----free-time-----//
//-----free-time-----//
//-----free-time-----//
//-----free-time-----//
//-----free-time-----//
#include <Arduino.h>
#include <avr/io.h>
#define INT0_vect _VECTOR(1)
#define INT1_vect _VECTOR(2)
#define TIMER1_OVF_vect _VECTOR(13)

#define CS PB2
#define MOSI PB3
#define SCK PB5

int x[16] = {4, 5, 6, 7}, y[16] = {7, 7, 7, 7}; // default row7 : 11110000
int horizontal = 1, vertical = 0;               // default shift right -->
uint8_t pos_x_score = 0, pos_y_score = 0, snake = 4;

void setting_interrupt()
{
  // use sw1(D2) : INT0, sw2(D3) : INT1, Timer1
  EICRA = EICRA | 0b00001010;         // failling edge
  EIMSK |= (1 << INT1) | (1 << INT0); // enable INT1 , INT0
  TCCR1A = 0b00000000;
  TCCR1B = 0b00000101; // prescale 1024
  TIMSK1 = 0b00000001;
  TCNT1 = 60000;
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
  matrix[pos_y_score] |= (1 << pos_x_score);

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

void move()
{

  // update x,y data dot frame by frame
  for (int position = snake - 1; position > 0; position--)
  {
    x[position] = x[position - 1];
    y[position] = y[position - 1];
  }
  // shift led for update
  x[0] -= horizontal;
  y[0] -= vertical;
  // horizontal
  if (x[0] < 0)
  {
    x[0] = 7; // กันหลุดจอ
  }
  // vertical
  if (y[0] < 0)
  {
    y[0] = 7; // กันหลุดจอ

    check_get();
    display();
  }
}
ISR(INT0_vect)
{
  // shift right (default)
  horizontal = 1, vertical = 0;
}

ISR(INT1_vect)
{
  // shift up
  horizontal = 0, vertical = 1;
}

ISR(TIMER1_OVF_vect)
{
  move();
  TCNT1 = 60000;
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