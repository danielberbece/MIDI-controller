# Makefile to build light_ws2812 library examples
# This is not a very good example of a makefile - the dependencies do not work, therefore everything is rebuilt every time.

# Change these parameters for your device

F_CPU = 16000000
DEVICE = atmega324p

# Tools:
CC = avr-gcc

LEDs       = light_ws2812
DEP_LEDs   = src/ws2812_config.h src/light_ws2812.h
USART     = usart
DEP_USART = src/usart.h
PROJ  = main

CFLAGS = -g2 -I. -ILight_WS2812 -mmcu=$(DEVICE) -DF_CPU=$(F_CPU) 
CFLAGS+= -Os -ffunction-sections -fdata-sections -fpack-struct -fno-move-loop-invariants -fno-tree-scev-cprop -fno-inline-small-functions  
CFLAGS+= -Wall -Wno-pointer-to-int-cast
#CFLAGS+= -Wa,-ahls=$<.lst

LDFLAGS = -Wl,--relax,--section-start=.text=0,-Map=main.map

all:	$(PROJ) 

$(LEDs): $(DEP_LEDs)
	@echo Building LEDs
	@$(CC) $(CFLAGS) -o Objects/$@.o -c src/$@.c 

$(USART): $(DEP_USART)
	@echo Building Usart 
	@$(CC) $(CFLAGS) -o Objects/$@.o -c src/$@.c 

$(PROJ): $(LEDs) $(USART)
	@echo Building $@
	@$(CC) $(CFLAGS) -o Objects/$@.o src/$@.c src/$(LEDs).c src/$(USART).c
	@avr-objcopy -j .text  -j .data -O ihex Objects/$@.o $@.hex
	@avr-size $@.hex
# 	@avr-objdump -d -S Objects/$@.o >Objects/$@.lss

.PHONY:	clean

clean:
	rm -f *.hex Objects/*.o Objects/*.lss
