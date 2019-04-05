CC=sh3eb-elf-gcc
CPP=sh3eb-elf-g++
OBJCOPY=sh3eb-elf-objcopy
MKG3A=mkg3a
RM=rm
CFLAGS=-mb -m4a-nofpu -Isrc -O2 -fmerge-all-constants -ggdb -mhitachi -fuse-linker-plugin -fshort-enums -flto -Wall -Wextra -I../../include -lgcc -L../../lib -I../libmpeg2-0.5.1/include 
LDFLAGS=$(CFLAGS) -nostartfiles -T../../toolchain/prizm.x -Wl,-static -Wl,-gc-sections -lgcc -flto -fuse-linker-plugin -lmpeg2 -lmpeg2convert -L../libmpeg2-0.5.1/libmpeg2/.libs/ -L../libmpeg2-0.5.1/libmpeg2/convert/.libs
OBJECTS=main.o filegui.o fileicons.o
PROJ_NAME=mpegtest
BIN=$(PROJ_NAME).bin
ELF=$(PROJ_NAME).elf
ADDIN=$(BIN:.bin=.g3a)
 
all: $(ADDIN)

$(ADDIN): $(BIN)
	$(MKG3A) -n :MpegPlayer -i uns:unselected.png -i sel:selected.png $< $@
 
.c.o:
	$(CC) -c $(CFLAGS) $< -o $@
	
.cpp.o:
	$(CC) -c $(CFLAGS) $< -o $@
	
.cc.o:
	$(CC) -c $(CFLAGS) $< -o $@

$(ELF): $(OBJECTS)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@
$(BIN): $(ELF)
	$(OBJCOPY) -O binary $(PROJ_NAME).elf $(BIN)

clean:
	rm -f $(OBJECTS) $(PROJ_NAME).bin $(PROJ_NAME).elf $(ADDIN)
