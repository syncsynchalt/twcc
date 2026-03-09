all: clean run-hello run-hello2 run-puts run-printf run-upper

SYSLIBROOT=$(shell xcrun -sdk macosx --show-sdk-path)

run-%:
	$(eval prog=$(patsubst run-%,prog-%,$@))
	$(eval ofile=$(patsubst run-%,%.o,$@))
	$(eval sfile=$(patsubst run-%,%.s,$@))
	as $(sfile) -o $(ofile)
	ld -o $(prog) $(ofile) -lSystem -syslibroot $(SYSLIBROOT)
	./$(prog)
	# @rm -f $(ofile)
	@echo

clean:
	rm -f prog-* *.o a.out

.PHONY: all clean
