
void
init_display(int dck, int dio);

void
display_draw(bool sep, uint8_t a, uint8_t b, uint8_t c, uint8_t d);

void
set_display_state(bool on);

bool
get_display_state(void);


void
init_i2c(uint8_t sda, uint8_t scl);

int
i2c_send(char *data, int len);

void
delay(uint32_t t);

void
init_clock(int sda, int scl);

void
set_time(uint8_t h, uint8_t m);

void
set_alarm(uint8_t h, uint8_t m);

void
get_time(uint8_t *h, uint8_t *m);

void
get_alarm(uint8_t *h, uint8_t *m);
