CC	  = sdcc
LD	  = sdcc
ARCHFLAGS = -mstm8 --std-sdcc99
INCLUDES  =
DEFINES   = -DSTM8S003
OPTFLAGS  = --max-allocs-per-node 1000  --opt-code-size
CFLAGS	  = $(ARCHFLAGS) $(OPTFLAGS) $(DEFINES) $(INCLUDES)
LDLIBS    =
LDFLAGS   = -mstm8 --out-fmt-ihx 

OBJECTS	  = main.rel uart.rel stm8s_uart1.rel i2c.rel PCF8583.rel moodlight.rel stm8s_flash.rel
TARGET   = blinky.ihx

INTERMEDIATE  = $(OBJECTS:.rel=.asm) $(OBJECTS:.rel=.lst) \
 $(OBJECTS:.rel=.sym) $(OBJECTS:.rel=.rst) $(TARGET:.ihx=.cdb) \
 $(TARGET:.ihx=.lk) $(TARGET:.ihx=.map) $(TARGET:.ihx=.mem) 

.PHONY: all clean flash

all: $(TARGET)

clean:
	-rm -f $(OBJECTS) $(INTERMEDIATE) $(TARGET)

flash: $(TARGET)
	stm8flash -c stlinkv2 -p stm8s103 -w $(TARGET)

$(TARGET): $(OBJECTS)
	$(LD) $(LDFLAGS) $(LDLIBS) $^ -o $@

%.rel: %.c
	$(CC) $(CFLAGS) -c $< -o $@

#dependencies
main.rel: main.c stm8s.h moodlight.h 
moodlight.rel: moodlight.c moodlight.h
uart.rel: uart.c uart.h 
stm8s_uart1.rel: stm8s_uart1.c stm8s_uart1.h
i2c.rel: i2c.c i2c.h
stm8s_clk.rel: stm8s_clk.c stm8s_clk.h
PCF8583.rel: PCF8583.c
stm8s_flash.rel: stm8s_flash.c stm8s_flash.h