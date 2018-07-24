DEST=/opt/roc/

all: $(DEST)roc $(DEST)zephyr

$(DEST)roc: roc2.o
	rm $(DEST)roc
	gcc -o $(DEST)roc roc2.o -lconfig
roc2.o: roc2.c
	rm roc2.o
	gcc -MMD roc2.c -O0 -c -o roc2.o

$(DEST)zephyr: zephyr.o
	rm $(DEST)zephyr
	gcc -o $(DEST)zephyr zephyr.o -lconfig
zephyr.o: zephyr.c
	rm zephyr.o
	gcc -MMD zephyr.c -O0 -c -o zephyr.o
