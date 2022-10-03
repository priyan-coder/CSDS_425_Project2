
CFLAGS=-g -Wall -Werror

TARGETS=proj2 #socketsd

all: $(TARGETS)

clean:
	rm -f $(TARGETS) 
	rm -rf *.dSYM
	rm -f *.html
	rm -f *.png
	rm -f *.jpg
	rm -f *.dat

distclean: clean
