#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <err.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>

#define total_operation (20000)
#define root_dir "/mnt/ramdisk/scale-test"

typedef struct {
	int thread_id;
	int start_index, end_index;
} thread_arg_t;

pthread_barrier_t barrier;

void *thread_func(void *arg) {
	int i, fd;
	thread_arg_t *this_arg = arg;
	char filename[256];

	pthread_barrier_wait(&barrier);
	for (i = this_arg->start_index; i < this_arg->end_index; ++i) {
		sprintf(filename, root_dir "/file-%d", i);
		fd = creat(filename, 0644);
		if (fd < 0)
			err(1, "file create error tid=%d filename=%s", this_arg->thread_id, filename);
		close(fd);
	}

	return NULL;
}
int main(int argc, char *argv[]) {
	int i, r, opt = 0;
	int nthread = 10;
	pthread_t *tids;
	thread_arg_t *thread_args;

	// parse command line options
	while ((opt = getopt(argc, argv, "t:")) != -1) {
		switch(opt) {
		case 't':
			nthread = strtol(optarg, NULL, 0);
			break;
		default:
			fprintf(stderr, "Usage: %s [-t thread_number]", argv[0]);
			exit(EXIT_FAILURE);
		}
	}

	mkdir(root_dir, 0755);
	pthread_barrier_init(&barrier, NULL, nthread + 1);
	tids = calloc(nthread, sizeof(pthread_t));
	thread_args = calloc(nthread, sizeof(thread_arg_t));

	// set thread arguments
	for (i = 0; i < nthread; ++i) {
		thread_args[i].thread_id = i;
		thread_args[i].start_index = total_operation * i / nthread;
		thread_args[i].end_index = total_operation * (i + 1) / nthread;
	}

	// start 
	for (i = 0; i < nthread; ++i) {
		r = pthread_create(&tids[i], NULL, thread_func, &thread_args[i]);
		if (r)
			errx(r, "pthread_create error");
	}
	
	usleep(100);
	pthread_barrier_wait(&barrier);

	for (i = 0; i < nthread; ++i) {
		pthread_join(tids[i], NULL);
	}
	free(tids);
	free(thread_args);
	pthread_barrier_destroy(&barrier);

	return 0;
}
