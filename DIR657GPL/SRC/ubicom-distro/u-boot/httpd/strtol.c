//#include <redboot.h>
//#include <common.h>
//#include <command.h>
#include <linux/ctype.h>

#define LONG_MIN   -1073741824
#define LONG_MAX    1073741823

int
isupper( int c )
{
    return (('A' <= c) && (c <= 'Z'));
} /* isupper() */


int
islower( int c )
{
    return (('a' <= c) && (c <= 'z'));
} /* islower() */


int 
isspace(int c)
{

    return ( (c == ' ') || (c == '\f') || (c == '\n') || (c == '\r') ||
             (c == '\t') || (c == '\v') );
} /* isspace() */


long
strtol( const char *nptr, char **endptr, int base )
{
    const char *s = nptr;
    unsigned long acc;
    int c;
    unsigned long cutoff;
    int neg = 0, any, cutlim;

//    CYG_REPORT_FUNCNAMETYPE( "strtol", "returning long %d" );
//    CYG_REPORT_FUNCARG3( "nptr=%08x, endptr=%08x, base=%d",
//                         nptr, endptr, base );
//    CYG_CHECK_DATA_PTR( nptr, "nptr is not a valid pointer!" );

//    if (endptr != NULL)
//        CYG_CHECK_DATA_PTR( endptr, "endptr is not a valid pointer!" );
    
    //
    // Skip white space and pick up leading +/- sign if any.
    // If base is 0, allow 0x for hex and 0 for octal, else
    // assume decimal; if base is already 16, allow 0x.
    //
    
    do {
        c = *s++;
    } while (isspace(c));
    if (c == '-') {
        neg = 1;
        c = *s++;
    } else if (c == '+')
        c = *s++;
    if ((base == 0 || base == 16) &&
        c == '0' && (*s == 'x' || *s == 'X')) {
        c = s[1];
        s += 2;
        base = 16;
    }
    if (base == 0)
        base = c == '0' ? 8 : 10;
    
    //
    // Compute the cutoff value between legal numbers and illegal
    // numbers.  That is the largest legal value, divided by the
    // base.  An input number that is greater than this value, if
    // followed by a legal input character, is too big.  One that
    // is equal to this value may be valid or not; the limit
    // between valid and invalid numbers is then based on the last
    // digit.  For instance, if the range for longs is
    // [-2147483648..2147483647] and the input base is 10,
    // cutoff will be set to 214748364 and cutlim to either
    // 7 (neg==0) or 8 (neg==1), meaning that if we have accumulated
    // a value > 214748364, or equal but the next digit is > 7 (or 8),
    // the number is too big, and we will return a range error.
    //
    // Set any if any `digits' consumed; make it negative to indicate
    // overflow.
    //
    
    cutoff = neg ? -(unsigned long)LONG_MIN : LONG_MAX;
    cutlim = cutoff % (unsigned long)base;
    cutoff /= (unsigned long)base;
    for (acc = 0, any = 0;; c = *s++) {
        if (isdigit(c))
            c -= '0';
        else if (isalpha(c))
            c -= isupper(c) ? 'A' - 10 : 'a' - 10;
        else
            break;
        if (c >= base)
            break;
        if (any < 0 || acc > cutoff || acc == cutoff && c > cutlim)
            any = -1;
        else {
            any = 1;
            acc *= base;
            acc += c;
        }
    }
    if (any < 0) {
        acc = neg ? LONG_MIN : LONG_MAX;
//        errno = ERANGE;
    } else if (neg)
        acc = -acc;
    if (endptr != 0)
        *endptr = (char *) (any ? s - 1 : nptr);

//    CYG_REPORT_RETVAL ( acc );

    return acc;
} // strtol()

int
atoi( const char *__nptr )
{
    int __retval;
    
    __retval = (int)strtol( __nptr, (char **)NULL, 10 );


    return __retval;
} /* atoi() */

long
atol( const char *__nptr )
{
    long __retval;
    
    __retval = strtol( __nptr, (char **)NULL, 10 );

    return __retval;
} /* atol() */
