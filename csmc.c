#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <signal.h>

/* variables --------- */

int students_num;
int tutors_num;
int chairs_num;
int help_num;
int tutors_ready = 0;
int students_ready = 0;

int done = 0;

pthread_t coordinator;
pthread_t* students;
pthread_t* tutors;

int id = 0;
int tId = 0;

pthread_mutex_t chair_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t id_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t tId_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t done_mutex = PTHREAD_MUTEX_INITIALIZER;

sem_t tutor_seat;
sem_t tutor_sleep;
sem_t finished_help;
sem_t student_signal;
sem_t helped;

/* functions ---------- */

// student thread function
void* student_func(void* arg) {

	int visits = 0;
	int chair_index;
	int studentId = id;
	unsigned int sleep_time;

	// get student id
	pthread_mutex_lock(&id_mutex);
	id++;
	studentId = id;
	printf("Student %d created\n", studentId);
	pthread_mutex_unlock(&id_mutex);

	while (visits < help_num) {
		// program for some time
		printf("Student %d is programming\n", studentId);
		sleep_time = (rand() % 5);
		sleep(sleep_time);

		// look for empty seat in waiting room
		pthread_mutex_lock(&chair_mutex);
		printf("Student %d is looking for chair...\n", studentId);
		if (chairs_num > 0) {
			chairs_num--;
			printf("Student %d found a chair\n", studentId);
			pthread_mutex_unlock(&chair_mutex);

			// notify coordinator
			sem_post(&student_signal);
			printf("Student %d signalling coordinator\n", studentId);

			// sleep until tutor available
			sem_wait(&tutor_seat);

			// mark seat as empty
			pthread_mutex_lock(&chair_mutex);
			chairs_num++;
			printf("Student %d is leaving the waiting room\n", studentId);
			pthread_mutex_unlock(&chair_mutex);

			// talk to tutor
			printf("Student %d is talking to a tutor\n", studentId);
			sem_wait(&helped);

			visits++;
			printf("Student %d was helped (%d visits)\n", studentId, visits);
		}
		else {
			printf("Student %d coudln't find a chair", studentId);
			pthread_mutex_unlock(&chair_mutex);
		}
	}

	pthread_mutex_lock(&done_mutex);
	done++;
	printf("Student %d has been fully helped\n", studentId);
	pthread_mutex_unlock(&done_mutex);

	pthread_exit(0);
}

// tutor thread function
void* tutor_func(void* arg) {
	int tutorId;
	unsigned int sleep_time;

	// get tutor id
	pthread_mutex_lock(&tId_mutex);
	tId++;
	tutorId = tId;
	printf("Tutor %d created\n", tutorId);
	pthread_mutex_unlock(&tId_mutex);

	// start tutoring
	while (done < students_num) {
		// wait to be assigned a student
		printf("Tutor %d is waiting...\n", tutorId);
		sem_wait(&tutor_sleep);

		// help student
		sem_post(&tutor_seat);
		printf("Tutor %d is helping a student\n", tutorId);
		sleep_time = (rand() % 5);
		sleep(sleep_time);
		sem_post(&helped);
		printf("Tutor %d is finished helping\n", tutorId);
	}

	pthread_exit(0);
}

// coordinator thread function
void* coordinator_func(void* arg) {
	printf("Coordinator created\n");
	while (done < students_num) {
		// wait on student to ask for help
		printf("Coordinator waiting...\n");
		sem_wait(&student_signal);

		// wake up tutor
		sem_post(&tutor_sleep);
	}

	pthread_exit(0);

}

int main(int argc, char* argv[]) {

	// seed rand
	srand((unsigned)time(NULL));

	// check the number of arguments
	if (argc < 5) {
		printf("Not enough arguments.\n");
		exit(-1);
	}

	// initialize num variables
	students_num = atoi(argv[1]);
	tutors_num = atoi(argv[2]);
	chairs_num = atoi(argv[3]);
	help_num = atoi(argv[4]);

	// allocate space for students, tutors, and chairs
	students = malloc(sizeof(pthread_t) * students_num);
	tutors = malloc(sizeof(pthread_t) * tutors_num);

	sem_init(&tutor_seat, 0, tutors_num);
	sem_init(&tutor_sleep, 0, 0);
	sem_init(&student_signal, 0, 0);
	sem_init(&helped, 0, 0);

	// create thread for coordinator
	pthread_create(&coordinator, NULL, coordinator_func, NULL);

	// create threads for students
	for (int i = 0; i < tutors_num; i++) {
		printf("Creating tutor thread...\n");
		pthread_create(&tutors[i], NULL, tutor_func, NULL);
	}
	tutors_ready = 1;

	// create threads for tutors
	for (int i = 0; i < students_num; i++) {
		printf("Creating student thread...\n");
		pthread_create(&students[i], NULL, student_func, NULL);
	}
	students_ready = 1;

	// wait on student threads to terminate
	for (int i = 0; i < students_num; i++) {
		pthread_join(students[i], NULL);
	}

	free(students);
	free(tutors);

	return 0;
}