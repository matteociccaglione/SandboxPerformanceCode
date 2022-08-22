
all:
	gcc -g -Wall -Wextra  lib/rngs.c lib/rvgs.c lib/rvms.c src/centers.c src/job.c src/events_queue.c src/verify.c src/estimations.c src/handle_events.c src/stats.c src/main.c -lm -o simulation

remove:
	rm simulation