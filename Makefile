
all:
	gcc -g -Wall -Wextra  lib/rngs.c lib/rvgs.c src/centers.c src/events_queue.c src/job.c src/main.c -o simulation

remove:
	rm simulation