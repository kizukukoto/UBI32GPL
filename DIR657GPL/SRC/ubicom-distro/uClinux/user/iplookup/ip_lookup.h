#define QUESTION_TYPE_NBSTAT	0x0021	/* stats request */
#define QUESTION_CLASS_IN	0x0001	/* internet class */

typedef struct{
	unsigned char Name[34];			/* compressed name */
	unsigned short Type;			/* question type */
	unsigned short Class;			/* question class (always type IN - Internet) */
} QUESTION;

typedef struct {
	unsigned short TransactionID;		/* transaction id, responses match original packet, requests are random/sequential */
	unsigned short Flag;			/* opcode, flags and rcode */
	unsigned short QDCount;			/* number of questions */
	unsigned short ANCount;			/* number of answer resource records */
	unsigned short NSCount;			/* number of name service resource records */
	unsigned short ARCount;			/* number of athoratative resource records */
	QUESTION question;
}netbiosMsg;
