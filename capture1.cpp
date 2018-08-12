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
#include <sys/sysinfo.h>
#include <semaphore.h>
#include <sched.h>
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
#include <errno.h>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv/highgui.h>
#include<opencv2/imgcodecs/imgcodecs.hpp>
using namespace cv;
using namespace std;

#define HRES 		640
#define VRES 		480
#define NUM_OF_THREADS	4


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
sem_t A;					//semaphore for capture thread.
sem_t B;					//semaphore for the scan thread
sem_t C;					//semaphore for the save thread

// Transform display window
char timg_window_name[] = "Edge Detector Transform";

int lowThreshold=0;
int const max_lowThreshold = 100;
int kernel_size = 3;
int edgeThresh = 1;
int ratio = 3;
Mat canny_frame, cdst, timg_gray, timg_grad;

VideoCapture capture(-1);


char file_name[100];

char tag_number[50];
Mat temporary;

vector<Mat> g1;//buffer of three 
vector<Mat> save1;//buffer of 1800

double readTOD(void)
{

	struct timeval tv;
	double ft=0.0;
	if(gettimeofday(&tv,NULL)!=0)
	{
		perror("readTOD");
		return 0.0;
	}
	else
	{
		ft=((double)(((double)tv.tv_sec)+(((double)tv.tv_usec)/1000000.0)));
		return ft;
	}

}



double elapsedTOD(double stopTOD, double startTOD)
{

	double dt;
	if(stopTOD>startTOD)
	{
		dt=stopTOD-startTOD;
	}
	else
	{
		printf("WARNING OVERFLOW \n");
		dt=0.0;
	}
}

void testcapture(void)
{

	
	double startTOD,stopTOD,diffTOD;
	if(!capture.isOpened())
	{
		printf("\nerror in opening");	
	}

	long count=1;
	startTOD=readTOD();
	while(count<=1800)
	{
	
	if(!capture.read(FRAME))
	{
		printf("\nError in reading the frame");
		break;
	}			
	count++;
	}

	stopTOD=readTOD();
	diffTOD=elapsedTOD(stopTOD,startTOD);
	printf("Avg frame rate is : %f  \n",(count/diffTOD));	

}

void *sequencer(void *arg)//sequence_t will point to this
{

	struct timespec tv1;
	struct timespec tv2;
	tv1.tv_sec=0;
	tv1.tv_nsec=33333333;


	int count=1;
	//while(1)
	do
	{	
		if(count%10==0)
		{
		sem_post(&A);
		}

		//if(count%30==0)
		//{
		//sem_post(&B);
		//}
		
		//if(count%30==0)
		//{
		//sem_post(&C);
		//}
		count++;
		nanosleep(&tv1,&tv2);
	}while(1);

}


void *capture1(void *arg)//capture_t will point to this
{
	char key;
	int count1=0;

	
	if(!capture.isOpened())
	{
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
	else
	{
		
		g1.push_back(FRAME);
		
		if(g1.size()==3)
		{
		temporary=g1.at(1).clone();
		//g1.clear();
		sem_post(&B);
		}
	}

	
	
	pthread_mutex_unlock(&mylock);
	
	
	
    	}
    
}


void *scan(void *arg)//scan_t will point to this
{	

	//printf("\nThread2");

	Mat temp;

	while(1)
	{
	sem_wait(&B);

 	
	
	pthread_mutex_lock(&mylock);
 	//temp=g1.at(1);
	
	save1.push_back(temporary);
	g1.clear();
	if(save1.size()==180)
	{
	
	pthread_cancel(capture_t);
	pthread_cancel(sequence_t);
	
	sem_post(&C);
	}
	pthread_mutex_unlock(&mylock);

 	
	
	}
}
void *save(void *param)
{
	int i;
	sem_wait(&C);
	pthread_cancel(scan_t);
	for(i=1;i<=180;i++)
	{

	strcpy(file_name,"");
	sprintf(tag_number,"%04d",i);
	strcat(file_name,tag_number);
	strcat(file_name,".ppm");
	
	//imwrite(file_name,FRAME);
	imwrite(file_name,save1.at(i-1));

	}

}

//void *save(void *arg)//save_t will point to this
//{	
	
//	int count=1;
	
	
//	Mat prev_image;

//	while(1)
//	{
//	sem_wait(&C);
 	
 	
	
//	pthread_mutex_lock(&mylock);
 //	//imshow( "Save Thread", FRAME );
//	strcpy(file_name,"");
//	sprintf(tag_number,"%04d",count);
//	strcat(file_name,tag_number);
//	strcat(file_name,".ppm");
	
	//imwrite(file_name,FRAME);
//	imwrite(file_name,g1.at(1));
//	g1.clear();
//	pthread_mutex_unlock(&mylock);
//	count++;
 	
	
//	}
 //}







int main( int argc, char** argv )
{


	//int coreid;
	//cpu_set_t threadcpu;

	
	status=XInitThreads();
	if(!status)
	printf("\n The X Window system failed  to initialize\n");
	else
	printf("\n The X Window system initialized successfully\n");

	//printf("This system has %d processors configured and %d processors available.\n", get_nprocs_conf(), get_nprocs());

	//int numberOfProcessors = get_nprocs_conf();
 
	//printf("number of CPU cores=%d\n", numberOfProcessors);

	cpu_set_t cpuset;
    	CPU_SET(1, &cpuset);
	

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
	pthread_attr_setaffinity_np(&rt_sched_attr[i], sizeof(cpu_set_t), &cpuset);
	    
	    
	    //rt_param[i].sched_priority = rt_max_prio - (i + 1);
	    pthread_attr_setschedparam(&rt_sched_attr[i], &rt_param[i]);
	}
	
	
	rt_param[0].sched_priority = rt_max_prio - (1);
	rt_param[1].sched_priority = rt_max_prio - (2);
	rt_param[2].sched_priority = rt_max_prio - (3);
	rt_param[3].sched_priority = rt_max_prio - (4);

	sem_init(&A, 0, 0);
        sem_init(&B, 0, 0);
	sem_init(&C, 0, 0);


	pthread_mutex_init(&mylock,NULL);

	printf("\n let us begin ");

	

	//status=pthread_create(&sequence_t,&rt_sched_attr[0],sequencer,NULL);//creation of thread
        //if(status==0)
	//{
	//printf("\n Sequencer thread  creation successfull");
	//}
	//else
	//printf("\n Sequencer thread  creation unsuccessfull");
	
	

	

	

	
	printf("\n let us create the third thread ");
	status=pthread_create(&save_t,&rt_sched_attr[3],save,NULL);//creation of thread
        if(status==0)
	{
	printf("\n Save thread creation successfull");
	}
	else
	printf("\n Save thread creation unsuccessfull");


	printf("\n let us create the second thread ");
	status=pthread_create(&scan_t,&rt_sched_attr[2],scan,NULL);//creation of thread
        if(status==0)
	{
	printf("\n Scan thread creation successfull");
	}
	else
	printf("\n Scan thread creation unsuccessfull");


	status=pthread_create(&capture_t,&rt_sched_attr[1],capture1,NULL);//creation of thread
        if(status==0)
	{
	printf("\n Capture thread  creation successfull");
	}
	else
	printf("\n Capture thread  creation unsuccessfull");


	status=pthread_create(&sequence_t,&rt_sched_attr[0],sequencer,NULL);//creation of thread
        if(status==0)
	{
	printf("\n Sequencer thread  creation successfull");
	}
	else
	printf("\n Sequencer thread  creation unsuccessfull");


	pthread_join(sequence_t,NULL);
	pthread_join(capture_t,NULL);
	pthread_join(scan_t,NULL);
	pthread_join(save_t,NULL);

	//testcapture();	
    
};