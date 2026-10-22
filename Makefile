all:
	$(MAKE) -C c_project
	cp c_project/trabalho .

clean:
	$(MAKE) -C c_project clean
	rm -f trabalho resultado.csv

.PHONY: all clean
