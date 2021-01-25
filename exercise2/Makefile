.PHONY:exe 

PROGRAM = exercise2
SHELL := /bin/bash

sources = $(PROGRAM).cpp

ICC = icc -fsource-asm -fno-alias -lm $(sources)
GPP = g++ -O0 -lm -march=native $(sources)

#############################################################################################


exe: .autoclean $(PROGRAM) Makefile
	if test -a ./$(PROGRAM); then ./$(PROGRAM) > results;else rm -f results; fi
	cat results

$(PROGRAM): $(sources) Makefile 
	$(GPP) -S -o $(PROGRAM).s
	$(GPP) -o $(PROGRAM)

.autoclean: Makefile
	make clean
	touch .autoclean

.PHONY: clean
clean:
	rm -f $(PROGRAM) results

