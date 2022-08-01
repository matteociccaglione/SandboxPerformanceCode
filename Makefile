
all:
	gcc -g -Wall -Wextra  lib/rngs.c lib/rvgs.c src/centers.c src/job.c src/main.c  -lm -o simulation

remove:
	rm simulation