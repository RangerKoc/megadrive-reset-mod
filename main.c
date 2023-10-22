// ----------------------------------------------------------------------------
// Megadrive Reset Mod
// ----------------------------------------------------------------------------
/*
 *   Arduino    Megadrive Board
 *       5V  <- Pad Port Pin 5 (VCC, +5V)
 *       GND <- GND
 * (PD2) D2  <- Pad Port Pin 7 (TH, SEL)
 * (PD3) D3  <- Reset Button Pin/Leg (Reset Input)
 * (PD4) D4  <- Pad Port Pin 6 (TL, DAT4)
 * (PD5) D5  <- Pad Port Pin 9 (TR, DAT5)
 * (PD6) D6  -> Reset Button Point on Board (Soft Reset Output)
 * (PD7) D7  -> Cart Slot Pin B2 (#MRES, Hard Reset) [optional]
 * (PB0) D8  <- Pad Port Pin 1 (DAT0)
 * (PB1) D9  <- Pad Port Pin 2 (DAT1)
 * (PB2) D10 <- Pad Port Pin 3 (DAT2)
 * (PB3) D11 <- Pad Port Pin 4 (DAT3)
 * (PB4) D12 -- x
 * (PC0) A0  -> LED Blue  [optional]
 * (PC1) A1  -> LED Green [optional]
 * (PC2) A2  -> LED Red   [optional]
 * (PC3) A3  -> JP1/2 (Language) [and to MultiBIOS Pin 39]
 * (PC4) A4  -> JP3/4 (Video Mode) [and to MultiBIOS Pin 38]
 * (PC5) A5  -- x
 */
// ----------------------------------------------------------------------------
/*
 * START + A + B + C        : Soft Reset
 * START + A + B + C + DOWN : Hard Reset (HARD_RESET_ENABLED must defined)
 * 
 * if defined ONLY_LANGUAGE_SWITCH:
 * START + B + LEFT         : Switch to English (Red)
 * START + B + RIGHT        : Switch to Japan (Green)
 * 
 * if defined ONLY_FREQUENCY_SWITCH:
 * START + B + UP           : Switch to 60 Hz (Red)
 * START + B + DOWN         : Switch to 50 Hz (Green)
 * 
 * else:
 * START + B + LEFT         : Switch to USA region (Blue)
 * START + B + RIGHT        : Switch to Europe region (Red)
 * START + B + UP           : Switch to Japan region (Purple)
 * START + B + DOWN         : Switch to Asia region (Green)
 */
// ----------------------------------------------------------------------------
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>
// ----------------------------------------------------------------------------
/* configuration */
//#define ONLY_LANGUAGE_SWITCH        /* Only Language switch is available */
//#define ONLY_FREQUENCY_SWITCH       /* Only Frequency switch is available */

//#define HARD_RESET_ENABLED

#define USE_BUTTON_DEBOUNCE         /* Use debouncing for button pressing on pad */
#define DEBOUNCE_THRESHOLD    200   /* Debounce threshold time in milliseconds */

#define RGB_LED_ENABLED             /* RGB LED is used */
#define RGB_LED_ANODE               /* RGB LED has common ANODE, else common CATHODE */

#define POLL_BY_MCU_ENABLED         /* If no pad polling, MCU will poll pad itself */
#define POLL_TIMEOUT          5000  /* Timeout when MCU will start pad polling */
#define POLL_PERIOD           100   /* Period between polls by MCU */

#define RESET_LONG_PRESS_TIME 750   /* Reset button long press detection time */
#define MODE_SAVE_DELAY       5000  /* Delay before saving current mode to EEPROM (if changed) */

//#define ATMEGA_CLONE_USED           /* Cloned ATmega uC (eg LGT328P) is used */
// ----------------------------------------------------------------------------
#define delay_us _delay_us
#define delay_ms _delay_ms
// ----------------------------------------------------------------------------
#define cbi(v, b) (v &= ~_BV(b))
#define sbi(v, b) (v |=  _BV(b))
// ----------------------------------------------------------------------------
#define LED_INIT sbi(DDRB,  DDB5)
#define LED_OFF  cbi(PORTB, DDB5)
#define LED_ON   sbi(PORTB, DDB5)
// ----------------------------------------------------------------------------
#if defined(RGB_LED_ENABLED)
#if defined(RGB_LED_ANODE)
#  define B_LED_ON  cbi(PORTC, DDC0)
#  define B_LED_OFF sbi(PORTC, DDC0)
#  define G_LED_ON  cbi(PORTC, DDC1)
#  define G_LED_OFF sbi(PORTC, DDC1)
#  define R_LED_ON  cbi(PORTC, DDC2)
#  define R_LED_OFF sbi(PORTC, DDC2)
#else
#  define B_LED_ON  sbi(PORTC, DDC0)
#  define B_LED_OFF cbi(PORTC, DDC0)
#  define G_LED_ON  sbi(PORTC, DDC1)
#  define G_LED_OFF cbi(PORTC, DDC1)
#  define R_LED_ON  sbi(PORTC, DDC2)
#  define R_LED_OFF cbi(PORTC, DDC2)
#endif
#else
#  define B_LED_ON
#  define B_LED_OFF
#  define G_LED_ON
#  define G_LED_OFF
#  define R_LED_ON
#  define R_LED_OFF
#endif

#if !defined(ONLY_FREQUENCY_SWITCH)
#define LANG_ENG  sbi(PORTC, DDC3)
#define LANG_JAP  cbi(PORTC, DDC3)
#endif

#if !defined(ONLY_LANGUAGE_SWITCH)
#define FREQ_60HZ sbi(PORTC, DDC4)
#define FREQ_50HZ cbi(PORTC, DDC4)
#endif
// ----------------------------------------------------------------------------
#define BTN_MASK 0b00110000
#define DIR_MASK 0b00001111

#define BTN_DATA (PIND & BTN_MASK)
#define DIR_DATA (PINB & DIR_MASK)
#define DATA     (DIR_DATA | BTN_DATA)
/* 0000 0100 */
#define SEL      (PIND & _BV(DDD2))
/* 0000 1000 */
#define RESET    (PIND & _BV(DDD3))
// ----------------------------------------------------------------------------
typedef struct
{
  union
  {
    uint8_t hi;
    struct
    {
      uint8_t up    : 1;
      uint8_t down  : 1;
      uint8_t left  : 1;
      uint8_t right : 1;
      uint8_t b     : 1;
      uint8_t c     : 1;
    };
  };
  union
  {
    uint8_t lo;
    struct
    {
      uint8_t up2   : 1;
      uint8_t down2 : 1;
      uint8_t       : 1;
      uint8_t       : 1;
      uint8_t a     : 1;
      uint8_t start : 1;
    };
  };
} buttons_t;

buttons_t buttons;
#if defined(POLL_BY_MCU_ENABLED)
uint8_t poll_int;
#endif
// ----------------------------------------------------------------------------
enum
{
#if defined(ONLY_LANGUAGE_SWITCH)
  L_ENG,
  L_JAP,

#elif defined(ONLY_FREQUENCY_SWITCH)
  F_60HZ,
  F_50HZ,

#else
  USA,
  JAP,
  EUR,
  ASIA,
#endif

  MODES_COUNT
};
// ----------------------------------------------------------------------------
void set_mode(uint8_t mode)
{
#if defined(ONLY_LANGUAGE_SWITCH)
  if (mode == L_JAP)
  {
    LANG_JAP;
  }
  else
  {
    LANG_ENG;
  }

#elif defined(ONLY_FREQUENCY_SWITCH)
  if (mode == F_50HZ)
  {
    FREQ_50HZ;
  }
  else
  {
    FREQ_60HZ;
  }

#else
  switch (mode)
  {
    case JAP:
      LANG_JAP;
      FREQ_60HZ;
      break;

    case EUR:
      LANG_ENG;
      FREQ_50HZ;
      break;

    case ASIA:
      LANG_JAP;
      FREQ_50HZ;
      break;

    case USA:
    default:
      LANG_ENG;
      FREQ_60HZ;
      break;
  }
#endif
}
// ----------------------------------------------------------------------------
#if defined(RGB_LED_ENABLED)
void update_rgb_led(uint8_t mode)
{
#if defined(ONLY_LANGUAGE_SWITCH)
  if (mode == L_JAP)
  {
    B_LED_OFF;
    G_LED_ON;
    R_LED_OFF;
  }
  else
  {
    B_LED_OFF;
    G_LED_OFF;
    R_LED_ON;
  }

#elif defined(ONLY_FREQUENCY_SWITCH)
  if (mode == F_50HZ)
  {
    B_LED_OFF;
    G_LED_ON;
    R_LED_OFF;
  }
  else
  {
    B_LED_OFF;
    G_LED_OFF;
    R_LED_ON;
  }

#else
  switch (mode)
  {
    case JAP:
      B_LED_ON;
      G_LED_OFF;
      R_LED_ON;
      break;

    case EUR:
      B_LED_OFF;
      G_LED_OFF;
      R_LED_ON;
      break;

    case ASIA:
      B_LED_OFF;
      G_LED_ON;
      R_LED_OFF;
      break;

    case USA:
    default:
      B_LED_ON;
      G_LED_OFF;
      R_LED_OFF;
      break;
  }
#endif
}
#else
#define update_rgb_led(mode)
#endif
// ----------------------------------------------------------------------------
void soft_reset(void)
{
  sbi(DDRD, DDD6);
  delay_ms(100);
  cbi(DDRD, DDD6);
}
// ----------------------------------------------------------------------------
#if defined(HARD_RESET_ENABLED)
void hard_reset(void)
{
  sbi(DDRD, DDD7);
  delay_ms(100);
  cbi(DDRD, DDD7);
}
#endif
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
/*
 *   | SEL | D5 | D4 |    |    |    |    |
 * # | TH  | TR | TL | D3 | D2 | D1 | D0 |
 * --|-----|----|----|----|----|----|----|
 * 1 | hi  | C  | B  | R  | L  | D  | U  |
 * 2 | lo  | S  | A  | 0  | 0  | D  | U  |
 * 3 | hi  | C  | B  | R  | L  | D  | U  |
 * 4 | lo  | S  | A  | 0  | 0  | D  | U  |
 * 5 | hi  | C  | B  | R  | L  | D  | U  |
 * 6 | lo  | S  | A  | 0  | 0  | 0  | 0  |
 * 7 | hi  | C  | B  | M  | X  | Y  | Z  |
 * 8 | lo  | S  | A  | 1  | 1  | 1  | 1  |
 */
// ----------------------------------------------------------------------------
ISR(INT0_vect)
{
  static uint8_t hi = 0;
  static uint8_t nd = 0;

  delay_us(2);

#if defined(ATMEGA_CLONE_USED)
  //asm("nop");
  //asm("nop");
  //asm("nop");
  //asm("nop");
  //asm("nop");
  //asm("nop");
  //asm("nop");
  //asm("nop");
#endif

  uint8_t dat = DATA;

  if (SEL)
  {
    hi = dat;
  }
  else if ( !(dat & 0b00001100) && (dat & 0b00000011) )
  {
    buttons.hi = ~hi;
    buttons.lo = ~dat;
    nd = 0;
  }
  else if (++nd == 8)
  {
    buttons.hi = buttons.lo = 0;
    nd = 0;
  }

#if defined(POLL_BY_MCU_ENABLED)
  poll_int = 1;
#endif
}
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
#if defined(ARDUINO)
int main_routine(void)
#else
int main(void)
#endif
{
  /* no pullup for pin PD6 */
  cbi(PORTD, DDD6);
  /* input mode for pin PD6 */
  cbi(DDRD,  DDD6);

#if defined(HARD_RESET_ENABLED)
  /* no pullup for pin PD7 */
  cbi(PORTD, DDD7);
  /* input mode for pin PD7 */
  cbi(DDRD,  DDD7);
#endif

  /* pullup pins PB0,PB1,PB2,PB3 */
  PORTB |=  (DIR_MASK);
  /* input mode for pins PB0,PB1,PB2,PB3 */
  DDRB  &= ~(DIR_MASK);
  /* pullup pins PD4,PD5 */
  PORTD |=  (BTN_MASK);
  /* intput mode for pins PD4,PD5 */
  DDRD  &= ~(BTN_MASK);

  /* pullup pin PD2 */
  sbi(PORTD, DDD2);
  /* intput mode for pin PD2 */
  cbi(DDRD, DDD2);

  /* pullup pin PD3 */
  sbi(PORTD, DDD3);
  /* intput mode for pin PD3 */
  cbi(DDRD, DDD3);

#if defined(RGB_LED_ENABLED)
  /* output mode for pins PC0,PC1,PC2 */
  DDRC  |=  (_BV(DDC0) | _BV(DDC1) | _BV(DDC2));
#endif

#if !defined(ONLY_FREQUENCY_SWITCH)
  /* output mode for pin PC3 */
  sbi(DDRC, DDC3);
#endif

#if !defined(ONLY_LANGUAGE_SWITCH)
  /* output mode for pin PC4 */
  sbi(DDRC, DDC4);
#endif

  LED_INIT;

  /* loading saved mode */
  uint8_t* mode_store_ptr = (uint8_t*)0;
  uint8_t mode_on_startup = eeprom_read_byte(mode_store_ptr) % MODES_COUNT;

  set_mode(mode_on_startup);

  update_rgb_led(mode_on_startup);

  soft_reset();

  buttons.hi = buttons.lo = 0;
#if defined(POLL_BY_MCU_ENABLED)
  poll_int = 0;
#endif

  /* interrupt for pin PD2 */
  sbi(EICRA, ISC00);
  sbi(EIMSK, INT0);
  /* enable interrupts */
  sei();

  uint8_t  mode_current     = mode_on_startup;
  uint16_t mode_change_time = 0;
  uint8_t  reset_short_down = 0;
  uint16_t reset_hold_time  = 0;

#if defined(USE_BUTTON_DEBOUNCE)
  uint16_t combo_time       = 0;
#endif

#if defined(POLL_BY_MCU_ENABLED)
  uint16_t poll_tmr = POLL_TIMEOUT;
#endif

  uint16_t t = 0;

  while (1)
  {
    // ----------------------------------------------------

#if defined(POLL_BY_MCU_ENABLED)
    /* handling no polling by interrupts */
    if (poll_int)
    {
      poll_int = 0;
      poll_tmr = POLL_TIMEOUT;
    }
    else if (poll_tmr > 0)
    {
      poll_tmr--;
    }
    else
    {
      LED_ON;

      /* output mode for pin D2 */
      sbi(DDRD, DDD2);

      buttons.hi = buttons.lo = 0;

      //for (uint8_t i = 0; i < 4; i++)
      {
        sbi(PORTD, DDD2);
        delay_us(10);

        cbi(PORTD, DDD2);
        delay_us(10);
      }

      /* input mode for pin D2 */
      cbi(DDRD, DDD2);
      delay_us(10);

      poll_int = 0;
      poll_tmr = POLL_PERIOD;

      LED_OFF;
    }
#endif

    // ----------------------------------------------------

    /* handling reset button */
    if (RESET)
    {
      reset_hold_time = 0;

      if (reset_short_down)
      {
        reset_short_down = 0;

        B_LED_OFF;
        G_LED_OFF;
        R_LED_OFF;

        soft_reset();
        buttons.hi = buttons.lo = 0;
#if defined(USE_BUTTON_DEBOUNCE)
        combo_time = 0;
#endif

#if defined(POLL_BY_MCU_ENABLED)
        poll_int = 0;
        poll_tmr = POLL_TIMEOUT;
#endif

        update_rgb_led(mode_current);
      }
    }
    else if (!reset_hold_time)
    {
      reset_hold_time  = t;
      reset_short_down = 1;
    }
    else if ( (t - reset_hold_time) > RESET_LONG_PRESS_TIME )
    {
      mode_current = (mode_current + 1) % MODES_COUNT;

      set_mode(mode_current);

      update_rgb_led(mode_current);

      mode_change_time = t;

      reset_hold_time  = t;
      reset_short_down = 0;
    }

    // ----------------------------------------------------

    /* handling buttons */
    if ( !buttons.start || !buttons.b )
    {
      LED_OFF;

#if defined(USE_BUTTON_DEBOUNCE)
      combo_time = 0;
#endif
    }
    else
    {
      LED_ON;

#if defined(USE_BUTTON_DEBOUNCE)
      if (!combo_time)
      {
        combo_time = t;
      }
      else if ( (t - combo_time) > DEBOUNCE_THRESHOLD )
#endif
      {
        if ( buttons.a && buttons.c )
        {
#if defined(HARD_RESET_ENABLED)
          if (buttons.down)
            hard_reset();
          else
#endif
            soft_reset();

          buttons.hi = buttons.lo = 0;

#if defined(USE_BUTTON_DEBOUNCE)
          combo_time = 0;
#endif

#if defined(POLL_BY_MCU_ENABLED)
          poll_int = 0;
          poll_tmr = POLL_TIMEOUT;
#endif
        }
        else
        {
          uint8_t dir = buttons.hi & DIR_MASK;
          uint8_t mode_new = 0xff;

#if defined(ONLY_LANGUAGE_SWITCH)
          switch (dir)
          {
            /* LEFT */
            case 0b00000100:
              mode_new = L_ENG;
              break;
            /* RIGHT */
            case 0b00001000:
              mode_new = L_JAP;
              break;
          }

#elif defined(ONLY_FREQUENCY_SWITCH)
          switch (dir)
          {
            /* UP */
            case 0b00000001:
              mode_new = F_60HZ;
              break;
            /* DOWN */
            case 0b00000010:
              mode_new = F_50HZ;
              break;
          }

#else
          switch (dir)
          {
            /* UP */
            case 0b00000001:
              mode_new = JAP;
              break;
            /* DOWN */
            case 0b00000010:
              mode_new = ASIA;
              break;
            /* LEFT */
            case 0b00000100:
              mode_new = USA;
              break;
            /* RIGHT */
            case 0b00001000:
              mode_new = EUR;
              break;
          }
#endif

          if (mode_new < MODES_COUNT && mode_current != mode_new)
          {
            set_mode(mode_new);

            update_rgb_led(mode_new);

            mode_current     = mode_new;
            mode_change_time = t;
          }
        }
      }
    }

    // ----------------------------------------------------

    /* saving current mode to eeprom, if needed */
    if (mode_change_time && ((t - mode_change_time) > MODE_SAVE_DELAY))
    {
      if (mode_on_startup != mode_current)
      {
        mode_on_startup = mode_current;
        eeprom_write_byte(mode_store_ptr, mode_current);

#if defined(RGB_LED_ENABLED)
        /* led blinking */
        for (uint8_t i = 0; i < 2; i++)
        {
          delay_ms(50);

          B_LED_ON;
          G_LED_ON;
          R_LED_ON;

          delay_ms(50);

          update_rgb_led(mode_current);
        }
#endif
      }

      mode_change_time = 0;
    }

    // ----------------------------------------------------

    t++;
    delay_ms(1);
  }

  return 0;
}
// ----------------------------------------------------------------------------
