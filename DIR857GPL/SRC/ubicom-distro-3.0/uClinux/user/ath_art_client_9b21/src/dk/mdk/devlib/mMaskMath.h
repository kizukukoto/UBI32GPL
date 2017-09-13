


A_UINT32 mask_max (A_UINT32 *x, A_UINT32 size, A_UINT32 *index) ;
void hist (A_UINT32 *y, A_UINT32 size, A_UINT32 *x, A_UINT32 *h, A_UINT32 *num_unique) ;
void qsort_mask (A_UINT32 *x, A_UINT32 low, A_UINT32 hi) ;
A_UINT32 mode (A_UINT32 *pop, A_UINT32 size, char *fname) ;
void fft (A_UINT32 n, double *xr, double *xi, double *wr, double *wi) ;
void bitrv (A_UINT32 n, double *xr, double *xi) ;
void wtable (A_UINT32 n, double *wr, double *wi) ;
void fftshift (A_UINT32 n, double *x) ;
void mag (A_UINT32 n, double *xr, double *xi, double *x) ;
double interp (double y2, double y1, A_UINT32 x2, A_UINT32 x1, A_UINT32 x) ;
