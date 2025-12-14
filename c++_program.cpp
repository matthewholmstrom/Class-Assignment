#include <stdio.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <omp.h>
#include <stdlib.h>


float Ranf( float low, float high );
int Ranf( int ilow, int ihigh );
float SQR( float x );
void	InitBarrier( int );
void	WaitBarrier( );




omp_lock_t	Lock;
volatile int	NumInThreadTeam;
volatile int	NumAtBarrier;
volatile int	NumGone;



int	NowYear;		// 2025- 2030
int	NowMonth;		// 0 - 11
int TotalMonths;

float	NowPrecip;		// inches of rain per month
float	NowTemp;		// temperature this month
float	NowHeight;		// grain height in inches
int	NowNumDeer;		// number of deer in the current population
int NowNumCoyote; //number of coyotes in the current population



const float GRAIN_GROWS_PER_MONTH =	       12.0;
const float ONE_DEER_EATS_PER_MONTH =		1.0;

const float AVG_PRECIP_PER_MONTH =		7.0;	// average
const float AMP_PRECIP_PER_MONTH =		6.0;	// plus or minus
const float RANDOM_PRECIP =			2.0;	// plus or minus noise

const float AVG_TEMP =				60.0;	// average
const float AMP_TEMP =				20.0;	// plus or minus
const float RANDOM_TEMP =			10.0;	// plus or minus noise

const float MIDTEMP =				40.0;
const float MIDPRECIP =				10.0;

unsigned int seed = 0;



float
SQR( float x )
{
        return x*x;
}



float
Ranf( float low, float high )
{
        float r = (float) rand();               // 0 - RAND_MAX
        float t = r  /  (float) RAND_MAX;       // 0. - 1.

        return   low  +  t * ( high - low );
}

int
Ranf( int ilow, int ihigh )
{
        float low = (float)ilow;
        float high = ceil( (float)ihigh );

        return (int) Ranf(low,high);
}

// call this if you want to force your program to use
// a different random number sequence every time you run it:
void
TimeOfDaySeed( )
{
	time_t now;
	time( &now );

	struct tm n;
	struct tm jan01;
#ifdef WIN32
	localtime_s( &n, &now );
	localtime_s( &jan01, &now );
#else
	n =     *localtime(&now);
	jan01 = *localtime(&now);
#endif
	jan01.tm_mon  = 0;
	jan01.tm_mday = 1;
	jan01.tm_hour = 0;
	jan01.tm_min  = 0;
	jan01.tm_sec  = 0;

	double seconds = difftime( now, mktime(&jan01) );
	unsigned int seed = (unsigned int)( 1000.*seconds );    // milliseconds
	srand( seed );
}



void
InitBarrier( int n )
{
        NumInThreadTeam = n;
        NumAtBarrier = 0;
	omp_init_lock( &Lock );
}


// have the calling thread wait here until all the other threads catch up:

void
WaitBarrier( )
{
        omp_set_lock( &Lock );
        {
                NumAtBarrier++;
                if( NumAtBarrier == NumInThreadTeam )
                {
                        NumGone = 0;
                        NumAtBarrier = 0;
                        // let all other threads get back to what they were doing
			// before this one unlocks, knowing that they might immediately
			// call WaitBarrier( ) again:
                        while( NumGone != NumInThreadTeam-1 );
                        omp_unset_lock( &Lock );
                        return;
                }
        }
        omp_unset_lock( &Lock );

        while( NumAtBarrier != 0 );	// this waits for the nth thread to arrive

        #pragma omp atomic
        NumGone++;			// this flags how many threads have returned
}





void Watcher(){


    while( NowYear < 2031 )
    {
        // compute a temporary next-value for this quantity
        // based on the current state of the simulation:
        
    
        // DoneComputing barrier:
        WaitBarrier( );
        
    
        // DoneAssigning barrier:
        WaitBarrier( );
        
        printf("%d,%d,%d, %d, %lf,%lf,%lf\n", TotalMonths, NowYear, NowNumDeer, NowNumCoyote, ((5./9.)*(NowTemp-32)) , (NowPrecip * 2.54), (NowHeight * 2.54 ) );
       


        if (NowMonth == 11) {
            NowYear = NowYear +1;
            NowMonth = 0;
        }
        else{
            NowMonth = NowMonth + 1;
        }
        
        TotalMonths ++;

        
        float ang = (  30.*(float)NowMonth + 15.  ) * ( M_PI / 180. );	// angle of earth around the sun

        float temp = AVG_TEMP - AMP_TEMP * cos( ang );
        NowTemp = temp + Ranf( -RANDOM_TEMP, RANDOM_TEMP );

        float precip = AVG_PRECIP_PER_MONTH + AMP_PRECIP_PER_MONTH * sin( ang );
        NowPrecip = precip + Ranf( -RANDOM_PRECIP, RANDOM_PRECIP );
        if( NowPrecip < 0. )
            NowPrecip = 0.;
    
        // DonePrinting barrier:
        WaitBarrier( );
        
    }

}


void Deer(){


    while( NowYear < 2031 )
    {
        // compute a temporary next-value for this quantity
        // based on the current state of the simulation:
        

        int nextNumDeer = NowNumDeer;
        int carryingCapacity = (int)( NowHeight );
        if( nextNumDeer < carryingCapacity )
                nextNumDeer++;
        else
                if( nextNumDeer > carryingCapacity )
                        nextNumDeer--;


        if (NowNumCoyote >= 1){

            nextNumDeer = nextNumDeer - NowNumCoyote;
        }


        if( nextNumDeer < 0 )
                nextNumDeer = 0;
        
    
        // DoneComputing barrier:
        WaitBarrier( );
        
        NowNumDeer = nextNumDeer;
    
        // DoneAssigning barrier:
        WaitBarrier( );
        
    
        // DonePrinting barrier:
        WaitBarrier( );
        
    }
    
}



void Grain(){


    while( NowYear < 2031 )
    {
        // compute a temporary next-value for this quantity
        // based on the current state of the simulation:
        
        float tempFactor = exp(   -SQR(  ( NowTemp - MIDTEMP ) / 10.  )   );

        float precipFactor = exp(   -SQR(  ( NowPrecip - MIDPRECIP ) / 10.  )   );

        float nextHeight = NowHeight;
        nextHeight += tempFactor * precipFactor * GRAIN_GROWS_PER_MONTH;
        nextHeight -= (float)NowNumDeer * ONE_DEER_EATS_PER_MONTH;
       
        if( nextHeight < 0. ) nextHeight = 0.;

        // DoneComputing barrier:
        WaitBarrier( );

        NowHeight = nextHeight;
        
    
        // DoneAssigning barrier:
        WaitBarrier( );
        
    
        // DonePrinting barrier:
        WaitBarrier( );
        
    }

}



void Coyote(){


    while( NowYear < 2031 )
    {
        // compute a temporary next-value for this quantity
        // based on the current state of the simulation:
        int NextNumCoyote = NowNumCoyote;
        float RandNum;

        if (NowMonth >=3 && NowMonth <=10){
           RandNum = Ranf(1,10);
           
           if (RandNum <=5){
              NextNumCoyote = NextNumCoyote + 1;
           }

           else{
            NextNumCoyote = NextNumCoyote -1;
           }

           if( NextNumCoyote < 0 ) NextNumCoyote = 0.;
        }
        else {
            NowNumCoyote = 0;
        }


        // DoneComputing barrier:
        WaitBarrier( );

        NowNumCoyote = NextNumCoyote;
        
    
        // DoneAssigning barrier:
        WaitBarrier( );
        
    
        // DonePrinting barrier:
        WaitBarrier( );
        
    }

}





int
main( int argc, char *argv[ ] )
{

        // starting date and time:
    NowMonth =    0;
    NowYear  = 2025;

    // starting state (feel free to change this if you want):
    NowNumDeer = 2;
    NowHeight =  5.;
    NowNumCoyote =  0;


    float ang = (  30.*(float)NowMonth + 15.  ) * ( M_PI / 180. );	// angle of earth around the sun

    float temp = AVG_TEMP - AMP_TEMP * cos( ang );
    NowTemp = temp + Ranf( -RANDOM_TEMP, RANDOM_TEMP );

    float precip = AVG_PRECIP_PER_MONTH + AMP_PRECIP_PER_MONTH * sin( ang );
    NowPrecip = precip + Ranf( -RANDOM_PRECIP, RANDOM_PRECIP );
    if( NowPrecip < 0. )
        NowPrecip = 0.;


    omp_set_num_threads( 4 );	// same as # of sections
    InitBarrier( 4 );	
    #pragma omp parallel sections
    {
        #pragma omp section
        {
            Deer( );
        }

        #pragma omp section
        {
            Grain( );
        }

        #pragma omp section
        {
            Watcher( );
        }
        
        #pragma omp section
        {
            Coyote( );	// your own
        }
        
    }       // implied barrier -- all functions must return in order
        // to allow any of them to get past here

}

