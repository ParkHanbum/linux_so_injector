#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/syscall.h>
#include <atomic>

using namespace std;


static int test = 0;
static int test1 = 0;

atomic<int> n(0);

void *thread_fn(void *arg)
{
	int self_number = *(int *)arg;

	printf("pid: %d\n", syscall(SYS_gettid));
	printf("addr: %p\n", &test1);

	while (1) {
		test1++;
	}
	return NULL;
}

void do_fork(int arg)
{
	int self_number = arg;
	test = arg;
	int pid = fork();
	if (pid == 0) {
		while (1) {
			test++;
			n.fetch_add(1, std::memory_order_relaxed);
		}
	}
}

int main(int argc, char *argv[])
{
	pthread_t thid1;
	pthread_t thid2;
	pthread_t thid3;

	printf("pid: %d\n", getpid());
	printf("addr: %p\n", &test);

	int tid1 = 210001;
	pthread_create(&thid1, NULL, thread_fn, &tid1);
	
	while (1) {
		test++;
	}
	pthread_join(thid1, NULL);

	return 0;
}
