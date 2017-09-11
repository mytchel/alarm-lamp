

#define LAMP_PIN_N  0
#define DDR_LAMP    DDRB
#define PIN_LAMP    PINB
#define PORT_LAMP   PORTB

#define BUTTON_PIN_N  2
#define DDR_BUTTON    DDRB
#define PIN_BUTTON    PINB
#define PORT_BUTTON   PORTB

#define DIO_PIN_N  6
#define DDR_DIO    DDRD
#define PIN_DIO    PIND
#define PORT_DIO   PORTD

#define DCK_PIN_N  7
#define DDR_DCK    DDRD
#define PIN_DCK    PIND
#define PORT_DCK   PORTD

#define SDA_PIN_N  8
#define DDR_SDA    DDRB
#define PIN_SDA    PINB
#define PORT_SDA   PORTB

#define SCL_PIN_N  8
#define DDR_SCL    DDRB
#define PIN_SCL    PINB
#define PORT_SCL   PORTB

void
init_display(void);

void
display_draw(bool sep, uint8_t a, uint8_t b, uint8_t c, uint8_t d);

void
set_display_state(bool on);

bool
get_display_state(void);

void
init_i2c(void);

int
i2c_send(char *data, int len);

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
