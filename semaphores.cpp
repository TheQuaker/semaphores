
#include <iostream>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/sem.h>

using namespace std;

union semun {
    int val;
    struct semid_ds* buf;
    unsigned short *array;
};

struct out
{
	int side;
	bool result;
};
	

int main(int argc, char* argv[])
{
	int n,i,l;
	int **array2;
	if(argc > 2)
	{
		n = argc - 1;		// plhthos orismatwn -to onoma
		i = atoi(argv[n]);	// plhthos ektelesewn 
	}
	else
	{
		cout<<"wrong number of arguments"<<endl;
		return 0;
	}
	
	array2 = new int*[3];
	for(int k = 0; k < 3; k++)
	{
		array2[k] = new int[n-1];
	}

	for(int j = 0;j < 3;j++)
	{
		for(int k = 0;k < n-1;k++)
		{
			if(j == 0)
				array2[j][k] = atoi(argv[k+1]);
			else
				array2[j][k] = 0;
		}
	}
	
	int semin1, semin2, semout1, semout2;//basikoi shmaforoi gia mutual exclusion and synchronization
	int shmin, shmout;
	int Cpid; // pid for child
	
	int *input;
	struct out *output;
	
	struct sembuf up = {0,1,0}, down = {0,-1,0};
	
	//dhmhourgia shmaforwn
	semin1 = semget((key_t)getpid(), 1, 0660|IPC_CREAT);
	if (semin1 == -1)
	{
		cerr<<"semin1(semget) failed! : "<<endl;
		exit(1);
	}
	semin2 = semget((key_t)getpid()-1, 1, 0660|IPC_CREAT);
	if (semin2 == -1)
	{
		cerr<<"semin2(semget) failed! : "<<endl;
		exit(1);
	}
	semout1 = semget((key_t)getpid()-2, 1, 0660|IPC_CREAT);
	if(semout1 == -1)
	{
			cerr<<"semout1(semget) failed! : "<<endl;
			exit(1);
	}
	semout2 = semget((key_t)getpid()-3, 1, 0660|IPC_CREAT);
	if(semout2 == -1)
	{
			cerr<<"semout2(semget) failed! : "<<endl;
			exit(1);
	}
	
	//arxikopoihsh semaforwn
	union semun arg;
	arg.val = 1;
	semctl(semin1,0,SETVAL,arg);
	semctl(semout1,0,SETVAL,arg);
	arg.val = 0;
	semctl(semin2,0,SETVAL,arg);
	semctl(semout2,0,SETVAL,arg);
	
	//desmeush mnhmhs kai attach se pointers
	
	shmin = shmget((key_t)778, sizeof(int), 0660|IPC_CREAT);
	if(shmin == -1)
	{
        cerr<<"shmin(shmget) failed! : ";
        exit(1);
    }
    shmout = shmget((key_t)777, sizeof(struct out), 0660|IPC_CREAT);
	if(shmin == -1)
	{
        cerr<<"shmin(shmget) failed! : ";
        exit(1);
    }
    
    input = (int*)shmat(shmin,NULL,0);
    if(input == NULL)
    {
		cerr<<"input(shmat) failed! : "<<endl;
		exit(1);
	}
    output = (out*)shmat(shmout,NULL,0);
    if(output == NULL)
    {
		cerr<<"output(shamt) failed! : "<<endl;
		exit(1);
	}
	
	////////////////////////////////////////////////////////////////////
	for(int number_of_children = 0; number_of_children < n-1 ;number_of_children++)
	{
		Cpid = fork();
		if(Cpid == -1)
		{
			cerr<<"Cpid(fork) failed! : "<<endl;
			exit(1);
		}
		else if(Cpid == 0) // kwdikas paidiwn
		{
			bool temp_resault;
			srand(getpid());
			while(1)
			{
				if(semop(semin2, &down, 1) == -1)
				{
					cerr<<"semin2(down) failed! : "<<endl;
					exit(1);
				}
				
				int square_side = *input;
				
				if(semop(semin1, &up, 1)== -1)
				{
					cerr<<"semin1(up) failed! : "<<endl;
					exit(1);
				}
				if (square_side == -1)
				{	
					exit(0);
				}
				else
				{
					float x = rand()%(square_side) + (rand()%101)/100.0 ;
					float y = rand()%(square_side) + (rand()%101)/100.0 ;
					//eksiswsh kuklikou diskou
					if(pow((x - square_side/2.0),2.0) + pow((y - square_side/2.0),2.0) <= pow((square_side/2.0),2.0))
					{
						temp_resault = true;
					}
					else
					{
						temp_resault = false;
					}
				}
				if(semop(semout1, &down, 1) == -1)
				{
					cerr<<"semout1(down) failed! : "<<endl;
					exit(1);
				}
				
				output->side = square_side;
				output->result = temp_resault;
				
				if(semop(semout2, &up, 1) == -1)
				{
					cerr<<"semout2(down) failed! : "<<endl;
					exit(1);
				}
			}
		}
	}
	////////////////////////////////////////////////////////////////////
	int side;
	bool t_f;
	for(int c = 0;c < i+n-1 ;c++) // kwdikas mhtrikhs diergasias
	{
		l = rand() % (n-1);
		if(semop(semin1, &down, 1) == -1)
		{
			cerr<<"semin1(down) failed : "<<endl;
			exit(1);
		}
		if(c < i)//meta apo i epanalipseis stile shma na termatisoun ta paidia
			*input = array2[0][l];
		else
			*input = -1;
		
		if(semop(semin2, &up, 1) == -1)
		{
			cerr<<"semin2(up) failed : "<<endl;
			exit(1);
		}
		
		if(c < i)
		{
			if(semop(semout2, &down, 1) == -1)
			{
				cerr<<"semout2(down)  failed : "<<endl;
				exit(1);
			}
			
			side = output->side;
			t_f = output->result;
			
			if(semop(semout1, &up, 1) == -1)
			{
				cerr<<"semout1(up)  failed : "<<endl;
				exit(1);
			}
			int k = 0;
			while(array2[0][k] != side)
			{
				k++;
			}
			if(t_f == true)
			{
				array2[1][k]++;
			}
			array2[2][k]++;
		}
	}

    ////////////////////////////////////////////////////////////////////
	
	for(i=0;i<n-1;i++)
		waitpid(-1,NULL,0);
	
	for(int j = 0;j<3;j++) // ektupwsh
	{
		for(i=0;i<n-1;i++)
		{
			cout<<array2[j][i]<<"	";
		}
		cout<<endl;
	}
	cout<<endl;
	for(int j = 0;j < n-1;j++)
	{
		if (array2[2][j] != 0)
		{
			cout<<"o"<<array2[0][j]<<" = "<<4*(float(array2[1][j])/array2[2][j])<<endl;
		}
	}
    //detache memory
    shmdt(input);
    shmdt(output);
    //destroy memory
    shmctl(shmin,0,IPC_RMID);
    shmctl(shmout,0,IPC_RMID);
    //destroy semaphores
    semctl(semin1,0,IPC_RMID,0);
    semctl(semin2,0,IPC_RMID,0);
    semctl(semout1,0,IPC_RMID,0);
    semctl(semout2,0,IPC_RMID,0);
    
    for(int k = 0; k < 3; k++)
	{
		delete[] array2[k];
	}
	delete[] array2;
	
    return 0;
}
