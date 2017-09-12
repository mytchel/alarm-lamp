

#define LAMP_PIN_N  2
#define DDR_LAMP    DDRD
#define PIN_LAMP    PIND
#define PORT_LAMP   PORTD

#define BUTTON_PIN_N  1
#define DDR_BUTTON    DDRB
#define PIN_BUTTON    PINB
#define PORT_BUTTON   PORTB

#define DIO_PIN_N  0
#define DDR_DIO    DDRB
#define PIN_DIO    PINB
#define PORT_DIO   PORTB

#define DCK_PIN_N  7
#define DDR_DCK    DDRD
#define PIN_DCK    PIND
#define PORT_DCK   PORTD

void
init_display(void);

void
display_draw(bool sep, 
             uint8_t a, uint8_t b, 
             uint8_t c, uint8_t d);

void
set_display_state(bool on);

bool
get_display_state(void);

void
init_i2c(void);

bool
i2c_message(uint8_t addr,
            uint8_t *sdata, int slen,
            uint8_t *rdata, int rlen);

void
delay(uint32_t t);

void
set_time(uint8_t h, uint8_t m);

void
set_alarm(uint8_t h, uint8_t m);

void
get_time(uint8_t *h, uint8_t *m);

void
get_alarm(uint8_t *h, uint8_t *m);

void
clock_test(void);
