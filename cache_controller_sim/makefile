all:
	gcc -g cache_sim.c -o sim

run: all
	./sim stimulus/evict_with_wb.test    > logs/evict_with_wb.log
	./sim stimulus/evict_without_wb.test > logs/evict_without_wb.log
	./sim stimulus/fullset_no_wb.test    > logs/fullset_no_wb.log
	./sim stimulus/HM1.test > logs/HM1.log
	./sim stimulus/HM2.test > logs/HM2.log
	./sim stimulus/HM3.test > logs/HM3.log

clean:
	rm -f a.out sim *.log out logs/*.log
