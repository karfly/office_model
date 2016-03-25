#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>


#include "./pc_queue_lib/pc_queue_lib.h"

struct clerk {

        pthread_t thread;
        pc_queue_t * queue;

        void * document;

        int number;
        int nDoc;
        int tClerk;
};

struct scanner {

        pthread_t thread;
        pc_queue_t * queue;

        int number;
        int tScanner;
};

void office_usage ();
static void * clerk_action (void * arg);
static void * scanner_action (void * arg);


int main (int argc, char **argv) {

	if (argc != 2) {
		printf ("Wrong number of arguments!\n");
		office_usage ();
		exit (EXIT_FAILURE);
	}

	FILE * config = fopen (argv[1], "r");
        if (!config) {
                printf ("Failed to open file [%s].\nReason: %s.\n", argv[1], strerror(errno));
                exit (EXIT_FAILURE);
        }
        int nClerk = 0, nDoc = 0, tClerk = 0, nScanner = 0, tScanner = 0, sQueue = 0;
        fscanf (config, "nClerk   = %d\n", &nClerk   );  
        fscanf (config, "nDoc     = %d\n", &nDoc     );    
        fscanf (config, "tClerk   = %d\n", &tClerk   );  
        fscanf (config, "nScanner = %d\n", &nScanner );
        fscanf (config, "tScanner = %d\n", &tScanner );
        fscanf (config, "sQueue   = %d\n", &sQueue   );  
        fclose (config);
        //printf ("%d %d %d %d %d %d\n", nClerk, nDoc, tClerk, nScanner, tScanner, sQueue);

        pc_queue_t * queue = pc_queue_create (sQueue);
        if (!queue) {
                printf ("Failed to create queue.\n");
                exit (EXIT_FAILURE);
        }

        int i   = 0;
        int ret = 0;

        int documents[nClerk];
        for (i = 0; i < nClerk; i++) {
                documents[i] = rand()/1000000;
        }

        struct clerk clerks[nClerk];
        for (i = 0; i < nClerk; i++) {
                clerks[i].queue    = queue;
                clerks[i].document = (void *) &documents[i];
                clerks[i].number   = i;
                clerks[i].nDoc     = nDoc;
                clerks[i].tClerk   = tClerk;

                ret = pthread_create (&clerks[i].thread, NULL, clerk_action, (void *) &clerks[i]);
                if (ret) {
                        errno = ret;
                        printf ("Failed to create thread.\nReason: %s.\n", strerror (errno));
                        exit (EXIT_FAILURE);
                }
        }

        struct scanner scanners[nScanner];
        for (i = 0; i < nScanner; i++) {
                scanners[i].queue    = queue;
                scanners[i].number   = i;
                scanners[i].tScanner = tScanner;

                ret = pthread_create (&scanners[i].thread, NULL, scanner_action, (void *) &scanners[i]);
                if (ret) {
                        errno = ret;
                        printf ("Failed to create thread.\nReason: %s.\n", strerror (errno));
                        exit (EXIT_FAILURE);
                }

        }

        void * retval;
        struct clerk * tmp_clerk;
        for (i = 0; i < nClerk; i++) {
                ret = pthread_join (clerks[i].thread, &retval);
                if (ret) {
                        errno = ret;
                        printf ("Failed to join thread.\nReason: %s.\n", strerror (errno));
                        exit (EXIT_FAILURE);
                }

                tmp_clerk = (struct clerk *) retval;
                printf ("Clerk #%d finished his job\n", tmp_clerk->number);
        }

        /*
        struct scanner * tmp_scanner;
        for (i = 0; i < nScanner; i++) {
                pthread_join (scanners[i].thread, &retval);
                tmp_scanner = (struct scanner *) retval;
                printf ("Scanner #%d finished his job\n", tmp_scanner->number);
        }
        */
        exit (EXIT_SUCCESS);

}

void
office_usage () {
        printf (
		"\n"
		"+++++++++++++++++++++++++++++++++++++++++++++++++++\n"
		"\"office\" is a program, which uses pc_queue_lib, elmulates office.\n"
		"\n"
		"SYNOPSIS:\n"
		"\toffice CONFIG_FILE\n"
		"\n"
		"CONFIG_FILE MUST BE LIKE:\n\n"
                "\t  config.txt\n"
                "|nClerk \t= X\t\t|\n"
                "|nDoc \t\t= X\t\t|\n"
                "|tClerk \t= X\t\t|\n"
                "|nScaner \t= X\t\t|\n"
                "|tScaner \t= X\t\t|\n"
                "|sQueue \t= X\t\t|\n"
		"+++++++++++++++++++++++++++++++++++++++++++++++++++\n"
	);

}

static void *
clerk_action (void * arg) {

        struct clerk * me = (struct clerk *) arg;

        int i = 0;
        int ret = 0;

        for (i = 0; i < me->nDoc; i++) {
                sleep (me->tClerk);
                ret = pc_queue_put (me->queue, (void *) me);
                if (ret) {
                        printf ("Failed to put element to the queue\n");
                        exit (EXIT_FAILURE);
                }
                printf ("Clerk #%d putted document [%d]\n", me->number, * (int *) me->document);
        }

        return arg;
}

static void *
scanner_action (void * arg) {

        struct scanner * me = (struct scanner *) arg;
        
        struct clerk * clerk_info = 0;
        while (1) {
                clerk_info = (struct clerk *) pc_queue_get (me->queue);
                sleep(me->tScanner);
                printf ("Scanner #%d printed document [%d] from Clerk #%d\n", me->number, * (int *) clerk_info->document, clerk_info->number);
        }

        return arg;
}
