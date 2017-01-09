/*
 * My_Alarm.c
 * 
 * AUTHORS: 
 * 		Jhan Perera - cse13187
 * 		Heaten Mystery - cse13131
 * 		Jason Hoi - cse31078
 *		Raghad Khudher - cse93170 
 *
 * This is an enhancement to the alarm_thread.c program, which
 * created an "alarm thread" for each alarm command. This new
 * version uses a single alarm thread, which reads the next
 * entry in a list. The main thread places new requests onto the
 * list, in order of absolute expiration time. The list is
 * protected by a mutex, and the alarm thread sleeps for at
 * least 1 second, each iteration, to ensure that the main
 * thread can lock the mutex to add new work to the list.
 *
 * COMPILE USING: cc My_Alarm.c -D_POSIX_PTHREAD_SEMANTICS -lpthread
 * OR via the "make" command. All the informaion is in the README file
 *
 */
#include <pthread.h>
#include <time.h>
#include "errors.h"

/*
 * The "alarm" structure now contains the time_t (time since the
 * Epoch, in seconds) for each alarm, so that they can be
 * sorted. Storing the requested number of seconds would not be
 * enough, since the "alarm thread" cannot tell how long it has
 * been on the list.
 */
typedef struct alarm_tag {
    struct alarm_tag    *link;
    int                 seconds;
    time_t              time;   /* seconds from EPOCH */
    char                message[64];
} alarm_t;

pthread_mutex_t alarm_mutex = PTHREAD_MUTEX_INITIALIZER;
alarm_t *alarm_list = NULL;


 /*
  * This function, new_thread_function, will be called and invoked
  * from the alarm_thread function, as a child thread. 
  *
  * Basically, everytime the alarm_thread function takes off an 
  * alarm_tag struct from the head of the list, alarm_list, it will
  * use that alarm_tag as an argument to be passed to the child thread.
  *
  * The child thread invoking this function, new_thread_function, is
  * responsible for waiting for the amount of time (seconds) to pass,
  * and for printing out a message each one of those seconds.
  * 
  * Once the alarm expires, the child thread will print out another
  * message, showing that the alarm expired.
  */
void *new_thread_function (void *arg)
{
    /* In order to use the argument passed to the child thread,
     * we must convert the type back to the alarm_t struct type
     * so that we may access its elements/members*/
    alarm_t *givenAlarm = (alarm_t*)arg;

    /* start and end are both time_t variables that will be used
     * to determine when the loop below should stop iterating.
     * Both start and end is the amount of seconds from the epoch till now.*/
    time_t start, end;
    int sleep_t = 1;

    /* start is assigned the time to indicate the starting time of
     * the alarm. 
     * end is assigned the time of start with the addition of 
     * the amount of seconds needed for the alarm to expire.*/
    start = time(NULL);
    end = start + givenAlarm->seconds;

    /* Upon each iteration:
     * The loop will check if start is less then end, it will then increment start by 1 second.
     * If start < end, then the child thread will sleep for one seccond and then print out the
     * specified message in the while loop body and then iterate once more.
     *
     * Once start = end, the while loop terminates
     */
    while((start++) < end)
    {
        sleep(sleep_t);
        printf("Alarm: >: <%d %s>\n", givenAlarm->seconds, givenAlarm->message);
    }

    /* Upon exiting the loop, a message will be printed to specify that the alarm has
     * expired. The alarm_t struct, allocated in memory, will then not be needed anymore
     * and hence its allocation of main memory will be deallocated. Once done the child
     * thread calls pthread_exit.
     */
    printf("Alarm Expired at <%d>:<%d %s>\n", time(NULL), givenAlarm->seconds, givenAlarm->message);
    free(givenAlarm);
    pthread_exit(0);
}


/*
 * The alarm thread's start routine.
 */
void *alarm_thread (void *arg)
{
    alarm_t *alarm;
    int sleep_time;
    time_t now;
    int status;

    int newThreadStatus;    /*Needed for A3.2*/
    pthread_t threadTemp;   /*Needed for A3.2*/

    /*
     * Loop forever, processing commands. The alarm thread will
     * be disintegrated when the process exits.
     */
    while (1) {
        alarm = alarm_list;

        /*
         * If the alarm list is empty, wait for one second. This
         * allows the main thread to run, and read another
         * command. If the list is not empty, remove the first
         * item. Compute the number of seconds to wait -- if the
         * result is less than 0 (the time has passed), then set
         * the sleep_time to 0.
         */
        if (alarm == NULL)
            sleep_time = 1;
        else {
            alarm_list = alarm->link;

            /*
             * The following message will be printed once the alarm_thread function selects an alarm
             * off the alarm list.
             * Each time the alarm_thread function does this, it will create a child thread to
             * process the alarm.
             * The alarm_thread function then iterates and does the same thing again if there are any alarms.*/
            printf("Alarm Retrieved at <%d>:<%d %s>\n", time(NULL), alarm->seconds, alarm->message);
            newThreadStatus = pthread_create(&threadTemp, NULL, new_thread_function, alarm);
        }
    }
}

int main (int argc, char *argv[])
{
    int status;
    char line[128];
    alarm_t *alarm, **last, *next;
    pthread_t thread;

	/*********************NEW CODE************************************/
	//Clear the terminal window - ASSUMING THIS WILL ONLY BE RUNNING IN PRISM LAB
	printf("\e[1;1H\e[2J");
	
	//Print out a instruction message to show the user what to do. 
	printf ("Please enter an alarm request in the format: # message\n");
	printf ("      # - the number of seconds until the alarm expires\n");
	printf ("message - the message that will be displayed when the alarm expires\n");
	/*****************END OF NEW CODE*********************************/
	 
    status = pthread_create (
        &thread, NULL, alarm_thread, NULL);
    if (status != 0)
        err_abort (status, "Create alarm thread");
    while (1) {
        printf ("alarm> ");
        if (fgets (line, sizeof (line), stdin) == NULL) exit (0);
        if (strlen (line) <= 1) continue;
        alarm = (alarm_t*)malloc (sizeof (alarm_t));
        if (alarm == NULL)
            errno_abort ("Allocate alarm");

        /*
         * Parse input line into seconds (%d) and a message
         * (%64[^\n]), consisting of up to 64 characters
         * separated from the seconds by whitespace.
		 * This will also check if the time is positive or 0. 
         */
        if (sscanf (line, "%d %64[^\n]", 
            &alarm->seconds, alarm->message) < 2 || alarm->seconds < 0) {
            fprintf (stderr, "Bad command\n");
            free (alarm);
        } else {
            status = pthread_mutex_lock (&alarm_mutex);
            if (status != 0)
                err_abort (status, "Lock mutex");

            //received = time(NULL);
            alarm->time = /*received*/ time(NULL) + alarm->seconds;

            /*
             * The main function prints out this message when a user enters an alarm. */
            printf("Alarm Received at <%d>: <%d %s>\n", (alarm->time - alarm->seconds), alarm->seconds, alarm->message);

            /*
             * Insert the new alarm into the list of alarms,
             * sorted by expiration time.
             */
            last = &alarm_list;
            next = *last;
            while (next != NULL) {
                if (next->time >= alarm->time) {
                    alarm->link = next;
                    *last = alarm;
                    break;
                }
                last = &next->link;
                next = next->link;
            }
            /*
             * If we reached the end of the list, insert the new
             * alarm there. ("next" is NULL, and "last" points
             * to the link field of the last item, or to the
             * list header).
             */
            if (next == NULL) {
                *last = alarm;
                alarm->link = NULL;
            }
#ifdef DEBUG
            printf ("[list: ");
            for (next = alarm_list; next != NULL; next = next->link)
                printf ("%d(%d)[\"%s\"] ", next->time,
                    next->time - time (NULL), next->message);
            printf ("]\n");
#endif
            status = pthread_mutex_unlock (&alarm_mutex);
            if (status != 0)
                err_abort (status, "Unlock mutex");
        }
    }
}