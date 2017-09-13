#ifdef _WINDOWS
#include <windows.h>
#endif 
#include <stdio.h>
#ifdef WIN32
#include <conio.h>
#endif
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>
#include "wlantype.h"   /* typedefs for A_UINT16 etc.. */
#include "athreg.h"
#include "manlib.h"     /* The Manufacturing Library */
#include "mMaskMath.h"
#include "mMaskPhys.h"


// Subroutine for largest element in x
// along with index of maximum value.
A_UINT32 mask_max (A_UINT32 *x, A_UINT32 size, A_UINT32 *index) {
    A_UINT32 i = 0;
    double  PI = (4.0 * atan2(1,1));
    A_INT32  INF = (A_INT32) (sin(PI/2.0)/cos(PI/2.0));
    A_INT32 max = -(INF);
    for(i=0; i<=size; i++) {   // each element(@x) {
      //	printf("snoop: mask_max : max = %d, ", max);
        if ((A_INT32)x[i]>max) {
            max = x[i];
            *index = i;
        }
	//printf(" h[%d] = %d, new max = %d, index = %d\n", i, x[i], max, *index);
    }
    return (max);
}
        
// Subroutine for binning elements of y
void hist (A_UINT32 *y, A_UINT32 size, A_UINT32 *x, A_UINT32 *h, A_UINT32 *num_unique) {
    A_UINT32 i = 0;
    A_INT32 j = -1;
    double  PI = (4.0 * atan2(1,1));
    A_INT32  INF = (A_INT32) (sin(PI/2.0)/cos(PI/2.0));
    A_UINT32 prev = INF;
    A_UINT32 curr = 0;


    qsort_mask(y,0,(size-1));
    //printf("Snoop: hist: done with qsort_mask : ");

    for(i=0; i<size; i++) {
	    h[i] = 0;
	    x[i] = 0;
    }

    for(i=0; i<size; i++) {
	    curr = y[i];
	    if (curr != prev) {
		    j++;
		    x[j] = curr;
		    prev = curr;
	    }
	    h[j] += 1;
	    //printf("Snoop: hist: y[%d]=%d, prev=%d, curr=%d, j=%d, h[j]=%d\n", i, y[i], prev, curr, j, h[j]);
    }

    *num_unique = j;
    //printf("Snoop: hist: done with num_unique = %d\n", *num_unique);
}

// Subroutine for sorting the elements in x
// using n*log(n) comparisons
void qsort_mask (A_UINT32 *x, A_UINT32 low, A_UINT32 hi) {
   // int *tempPtr = *x;
  //  A_UINT32 mid; 

    if (hi > low) {
        A_UINT32 mid = x[(low + hi) / 2];
	A_INT32 i = low;
	A_INT32 j = hi;
	A_UINT32 temp = 0;

        while (i <= j) {
	  //printf(" xi%d=%d,xj%d=%d \n", i,x[i], j, x[j]);	
            if ((x[i] >= mid) && (x[j] <= mid)) {
                temp   = x[i];
                x[i] = x[j];
                x[j] = temp;
                i++;
                j--;
            } else {
                if (x[i] < mid) { i++; }
                if (x[j] > mid) { j--; }
            }
        }
        qsort_mask(x,low,j);
        qsort_mask(x,i,hi);
    }
}

// Subroutine for determining the most likely value in a set
A_UINT32 mode (A_UINT32 *pop, A_UINT32 size, char *fname) {
    A_UINT32 *x;
    A_UINT32 *h;
    A_UINT32 y = 0;
    A_UINT32 i = 0;
    A_UINT32 n_unique;

    //    char  logfile[32];

    //    FILE *LOG;

    //printf("snoop:mode: %s size = %d : ", fname, size);

	x = (A_UINT32 *)malloc(20*sizeof(A_UINT32));
	h = (A_UINT32 *)malloc(20*sizeof(A_UINT32));

//    sprintf(logfile, "%s.log", fname);
//    if( (LOG = fopen( logfile, "w")) == NULL ) {
//	  printf("Failed to open %s for writing\n", logfile);
//	  exit(0);
//    }

//    for (i=0; i<size; i++) {
//	    printf( " pop[%d] = %d ", i, pop[i]);
//    }
//    printf ("\n");
//    fclose(LOG) ;


    hist(pop, size, &(x[0]), &(h[0]), &n_unique);


//    sprintf(logfile, "%s.hist", fname);
//    if( (LOG = fopen( logfile, "w")) == NULL ) {
//	  printf("Failed to open %s for writing\n", logfile);
//	  exit(0);
//    }

//    for (i=0; i<n_unique; i++) {
//	  fprintf(LOG, "%d\t%d\n", x[i], h[i]);
//    }
//    fclose(LOG) ;
    
    y = mask_max(h, n_unique, &i);
	y = x[i];
	//printf("snoop: mode : x[%d] = %d (y=%d)\n", i, x[i], y);
	free(x);
	free(h);
	fname = NULL;  //keep the compiler happy
    return y;
}

// Subroutine for radix-2 decimation-in-frequency
// FFT algorithm (from Burrus and Parks (1985))
void fft (A_UINT32 n, double *xr, double *xi, double *wr, double *wi) {
    A_UINT32 i,i1,i2,j,k,l,m,n1,n2;
    double   c, s, tempr, tempi;

	n2 = 0;
	m = 0;

    i = 1;
    while( (i<=15) && (n != n2)) {
	    m = i;
	    n2 = (A_UINT32) pow(2.0, i*1.0);
//	    printf("snoop: fft : i=%d, n2=%d, n=%d\n", i, n2, n);
	    i++;
    }
    
    if (i == 16) {
	    printf("N is not a power of two in mMaskMath.c --> fft() !\n");
	    exit(0);
    }

    n2 = n;
    for (i=0; i<m; i++) {
        n1 = n2;
        n2 /= 2;
        i1 = 0;
        i2 = n / n1;
        for (j=0; j<n2; j++) {
            c = wr[i1];
            s = wi[i1];
            i1 = i1 + i2;
            for (k=j; k<n; k+=n1) {
	      //	    printf("snoop: fft : i=%d, j=%d, k=%d\n", i, j, k);

                l = k + n2;
                tempr = xr[k] - xr[l];
                xr[k] += xr[l];
                tempi = xi[k] - xi[l];
                xi[k] += xi[l];
                xr[l] = c * tempr + s * tempi;
                xi[l] = c * tempi - s * tempr;
            }
        }
    }
    
//    printf("snoop : fft : calling bitrv\n");

    bitrv(n, xr, xi);
//    printf("snoop : fft : done calling bitrv\n");
    
}

// Subroutine for bit reversal
// (from Burrus and Parks (1985))
void bitrv (A_UINT32 n, double *xr, double *xi) {
	A_UINT32 i,j;
	double k;
	double temp;

//	printf ("snoop : bitrv : n = %d\n", n);

	j = 0;
	for (i=0; i<n; i++) {
		if (i < j) {
			temp = xr[j];
			xr[j] = xr[i];
			xr[i] = temp;
			temp = xi[j];
			xi[j] = xi[i];
			xi[i] = temp;
		}
		k = n / 2.0;
		
		//		  printf("snoop: bitrv : i=%d, j=%d, k=%d, ", i, j, k);

		while (k <= j) {
			j -= (A_UINT32) k;
			k /= 2.0;
		}
		j += (A_UINT32) k;
		//		  printf("new i=%d, j=%d, k=%d\n", i, j, k);
		
	}
	
}

// Subroutine for generating a sine and cosine
// table for the computation of the DFT
void wtable (A_UINT32 n, double *wr, double *wi) {
    A_UINT32 k;
    double phi;
    double wn = 6.28318530717959 / n;

    for (k=0; k<n; k++) {
        phi = wn * k;
        wr[k] = cos(phi);
        wi[k] = sin(phi);
    }
}

// Subroutine for shifting DC component to
// center of spectrum 
void fftshift (A_UINT32 n, double *x) {
	A_UINT32 i;
	double temp[NUM_TURBO_MASK_PTS];
	
	for(i=0; i<n/2; i++) {
		temp[i] = x[i];
		x[i] = x[n/2+i];
	}

	for (i=n/2; i<n; i++) {
		x[i] = temp[i-n/2];
	}
}

// Subroutine for complex modulus (magnitude) of
// the elements of x
void mag (A_UINT32 n, double *xr, double *xi, double *x) {
    A_UINT32 k;

    for (k=0; k<n; k++) {
        x[k] = sqrt(xr[k]*xr[k]+xi[k]*xi[k]);
    }
}


// Subroutine for 1-D interpolation (table lookup)
double interp (double y2, double y1, A_UINT32 x2, A_UINT32 x1, A_UINT32 x) {
    return y1+(y2-y1)/(x2-x1)*(x-x1);
}
