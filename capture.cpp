/*
 *
 *  Example by Sam Siewert 
 *
 *  Updated 10/29/16 for OpenCV 3.1

    There shall be four threads in my code. They are as follows in order of priority

    1.sequencer thread: to sequence all the other operations. I shall name it sequence_t;	

    2.capture thread which will capture frames at the rate of minimum 20fps. I shall name it capture_t

    3.scan thread which will scan all the threads to select which one must be saved in a buffer. I shall name it scan_t

    4.save thread which will save the captured thread, I shall name it save_t	

 *
 */


#include <X11/Xlib.h>
#include <semaphore.h>
#include <pthread.h>
#include <syslog.h>
#include <time.h>
#include <sys/time.h>
#include <sched.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv/highgui.h>
#include<opencv2/imgcodecs/imgcodecs.hpp>
using namespace cv;
using namespace std;

#define HRES 		640
#define VRES 		480
#define NUM_OF_THREADS	3


/****************************VARIABLES*********************************************/
 //struct timespec thiscycle;			//variable for current cycle
 //struct timespec previouscycle;			//variable for this cycle
 //long double delta_t;				//time between consecutive cycles
 //delta_t=0.0;					//setting the value initally to 0

int status;

pthread_t sequence_t;				//thread name for sequence
pthread_t capture_t;				//thread name for capture
pthread_t scan_t;				//thread for scanning
pthread_t save_t;				//thread for saving
Mat FRAME;					//matrix for storing the image

pthread_mutex_t mylock;				//lock to synchronize the frame variable
sem_t A;
sem_t B;
sem_t C;

// Transform display window
char timg_window_name[] = "Edge Detector Transform";

int lowThreshold=0;
int const max_lowThreshold = 100;
int kernel_size = 3;
int edgeThresh = 1;
int ratio = 3;
Mat canny_frame, cdst, timg_gray, timg_grad;

VideoCapture capture(0);






void *capture1(void *arg)//capture_t will point to this
{
	char key;

	printf("\nThread1");
	if(!capture.isOpened()){
	printf("\nerror in opening");	
	}
    	while(1)
    	{
	sem_wait(&A);	
	pthread_mutex_lock(&mylock);
	
        if(!capture.read(FRAME))
	{
		printf("\nError in reading the frame");
		break;
	}

	imshow( "Capture Thread", FRAME );
	
	pthread_mutex_unlock(&mylock);
	
	key=waitKey(1);
	if(key==27){
	printf("\nExiting the application");
	break;
	}
	sem_post(&B);
    	}

    


    
}




void *scan(void *arg)//scan_t will point to this
{	

	printf("\nThread2");
	char key;


	while(1)
	{
	sem_wait(&B);
 	Mat gray_image;
 	
	
	pthread_mutex_lock(&mylock);
 	imshow( "Scan Thread", FRAME );
	pthread_mutex_unlock(&mylock);

 	key=waitKey(1);
	if(key==27){
	printf("\nExiting the application");
	break;
	}
	//sleep(0.09);
	sem_post(&C);
	}
}


void *save(void *arg)//save_t will point to this
{	

	printf("\nThread3");
	char key;


	while(1)
	{
	sem_wait(&C);
 	Mat gray_image;
 	
	
	pthread_mutex_lock(&mylock);
 	imshow( "Save Thread", FRAME );
	//imwrite("Image_frame.jpeg",FRAME);
	pthread_mutex_unlock(&mylock);

 	key=waitKey(1);
	if(key==27){
	printf("\nExiting the application");
	break;
	}
	//sleep(1);
	sem_post(&A);
	}
}







int main( int argc, char** argv )
{




	
	status=XInitThreads();
	if(!status)
	printf("\n The X Window system failed  to initialize\n");
	else
	printf("\n The X Window system initialized successfully\n");



	struct sched_param rt_param[NUM_OF_THREADS];
    	pthread_attr_t rt_sched_attr[NUM_OF_THREADS];

	int rt_max_prio, rt_min_prio;
	
	rt_max_prio = sched_get_priority_max(SCHED_FIFO);
	rt_min_prio = sched_get_priority_min(SCHED_FIFO);

	//assinging the current main process thread to be of highest priority
	struct sched_param params;
	params.sched_priority = rt_max_prio;
	status = pthread_setschedparam(pthread_self(), SCHED_FIFO, &params);
	if (status != 0) 
	{
		printf("Unsuccessful in setting thread realtime prio\n");
		return 1;     
	}




	for(int i = 0; i < NUM_OF_THREADS; i++)
	{
	
	pthread_attr_init(&rt_sched_attr[i]);
        pthread_attr_setinheritsched(&rt_sched_attr[i], PTHREAD_EXPLICIT_SCHED);
        pthread_attr_setschedpolicy(&rt_sched_attr[i],  SCHED_FIFO);
	    
	    //Assigning highest priority to the 0th index which is used for Sum1 and onwards
	    rt_param[i].sched_priority = rt_max_prio - (i + 1);
	    pthread_attr_setschedparam(&rt_sched_attr[i], &rt_param[i]);
	}



	sem_init(&A, 0, 1);
        sem_init(&B, 0, 0);
	sem_init(&C, 0, 0);


	pthread_mutex_init(&mylock,NULL);

	printf("\n let us begin ");

	status=pthread_create(&capture_t,&rt_sched_attr[0],capture1,NULL);//creation of thread
        if(status==0)
	{
	printf("\n Capture thread  creation successfull");
	}
	else
	printf("\n Capture thread  creation unsuccessfull");
	

	printf("\n let us create the second thread ");
	status=pthread_create(&scan_t,&rt_sched_attr[1],scan,NULL);//creation of thread
        if(status==0)
	{
	printf("\n Scan thread creation successfull");
	}
	else
	printf("\n Scan thread creation unsuccessfull");

	
	printf("\n let us create the third thread ");
	status=pthread_create(&save_t,&rt_sched_attr[2],save,NULL);//creation of thread
        if(status==0)
	{
	printf("\n Save thread creation successfull");
	}
	else
	printf("\n Save thread creation unsuccessfull");


	pthread_join(capture_t,NULL);
	pthread_join(scan_t,NULL);
	pthread_join(save_t,NULL);	
    
};
