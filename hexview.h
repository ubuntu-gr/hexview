#ifndef HEXVIEW_H				/* start of inclusion guard */
#define HEXVIEW_H

#include <errno.h>

/* -----------------------------------
 * Program specific Constants & Macros
 * -----------------------------------
 */

/* program's name & version */
#define NAME_VERSION		"HEXVIEW 0.26A (Feb 2012)"	

#define MAXINPUT		(1024+1)	/* max # of chars to handle in stdin  */

/* formatted output constants */

enum FmtCharSet { FMT_ASCII, FMT_XASCII };	/* plain or extended ASCII char set   */

#define NAME_CHARSET(cs)						\
	( (cs) == FMT_ASCII ? "ASCII" : (cs) == FMT_XASCII ? "xASCII" : "Unknown" )
#define FNAME_SHOWLEN		11		/* len of truncated fnames ( w/o '\0')*/
#define FMT_GRPCOLS		4		/* # of bytes to group columns by     */
#define FMT_OFST		8		/* offset column-width, in chars      */
#define FMT_NCOLS		16		/* max # of bytes in a row (up to 16) */
#define FMT_PGLINES		21		/* page length, in rows               */

/* calculate index of the FIRST byte in a given row */
#define ROW2BT( row, nrows )						\
(									\
	(row) > (nrows-1)						\
	? ( (nrows-1) * FMT_NCOLS )					\
	: (row) > 0 ? ( (row) * FMT_NCOLS ) : 0				\
)

/* calculate the page a given row lyes in */
#define ROW2PG(row, nrows)						\
(									\
	(row) > (nrows-1)						\
	? ( (nrows-1) / FMT_PGLINES )					\
	: (row) > 0 ? ( (row) / FMT_PGLINES ) : 0			\
)

/* calculate the row a given byte lyes in */
#define BT2ROW(bt, nbytes, nrows)					\
(									\
	(bt) > (nbytes) - 1						\
	? (nrows-1)							\
	: (bt) > 0 ? (bt) / FMT_NCOLS : 0				\
)

/* calculate the starting row of a given page */
#define PG2ROW(pg, npages)						\
(									\
	(pg) > (npages-1)						\
	? ( (npages-1) * FMT_PGLINES )					\
	: (pg) > 0 ? ( (pg) * FMT_PGLINES ) : 0				\
)

/* -----------------------------------
 * Color related Constants & Macros
 * -----------------------------------
 */

/* selected color theme */
#define CON_DARKBG	1				/* 0 on console with bright bg*/

/* color theme for consoles with dark background */
#if CON_DARKBG

	#define FGCLR_NORMAL	FG_GRAY			/* normal fg color for dark bg*/

	#define FGCLR_ROWOFST	FG_CYAN			/* row-offset fg color        */

	#define FGCLR_BYTPRT1	FGCLR_NORMAL		/* printable bytes fg-color   */
	#define FGCLR_BYTPRT0	FG_DARKYELLOW		/* non-printable bytes fg-colr*/
	#define FGCLR_BYTZERO	FG_DARKGRAY		/* zeroed bytes fg-color      */
	#define FGCLR_BYTCURR	FG_MAGENTA		/* current byte fg-color      */

	#define BGCLR_PMTFNAME	BG_NOCHANGE		/* filename bg-color in prompt*/
	#define FGCLR_PMTFNAME	FG_YELLOW		/* filename fg-color in prompt*/
	#define BGCLR_PMTPG	BG_NOCHANGE		/* page bg-color in prompt    */
	#define FGCLR_PMTPG	FG_GREEN		/* page fg-color in prompt    */
	#define BGCLR_PMTCHRSET	BG_DARKRED		/* charset bg-color in prompt */
	#define FGCLR_PMTCHRSET	FG_WHITE		/* charset fg-color in prompt */

	#define BGCLR_PMTBYTPOS	BG_DARKMAGENTA		/* byte pos bg-color in prompt*/
	#define FGCLR_PMTBYTPOS	FG_WHITE		/* byte pos fg-color in prompt*/
	#define BGCLR_PMTROWPOS	BG_DARKMAGENTA		/* row pos bg-color in prompt */
	#define FGCLR_PMTROWPOS	FG_GRAY			/* row pos fg-color in prompt */
	#define BGCLR_PMTDEC	BG_DARKMAGENTA		/* dec vals bg-color in prompt*/
	#define FGCLR_PMTDEC	FG_WHITE		/* dec vals fg-color in prompt*/
	#define BGCLR_PMTOCT	BG_DARKMAGENTA		/* oct val bg-color in prompt */
	#define FGCLR_PMTOCT	FG_WHITE		/* oct val fg-color in prompt */
	#define BGCLR_PMTBIN	BG_DARKMAGENTA		/* bin val bg-color in prompt */
	#define FGCLR_PMTBIN	FG_WHITE		/* bin val fg-color in prompt */
	#define BGCLR_PMTPRVCMD	BG_NOCHANGE		/* prev cmd bg-color in prompt*/
	#define FGCLR_PMTPRVCMD	FG_WHITE		/* prev cmd fg-color in prompt*/

#if 1
	#define FGCLR_EM1	FG_WHITE	/* 1st emphasised fg color on dark bg */
	#define FGCLR_EM2	FG_YELLOW	/* 2nd emphasised fg color on dark bg */
	#define FGCLR_EM3	FG_CYAN		/* 3rd emphasised fg color on dark bg */
	#define FGCLR_EM4	FG_GREEN	/* 4th emphasised fg color on dark bg */
	#define FGCLR_FD1	FG_DARKGRAY	/* 1st faded fg color on dark bg      */
	#define FGCLR_FD2	FG_DARKYELLOW	/* 2nd faded fg color on dark bg      */
	#define FGCLR_FD3	FG_DARKCYAN	/* 3rd faded fg color on dark bg      */
#endif
#endif

#define colorPRINTF( colorize, fg, bg, ... )				\
do {									\
	if ( (colorize) )						\
		CONOUT_PRINTF( fg, bg, __VA_ARGS__ );			\
	else								\
		printf( __VA_ARGS__ );					\
} while(0)

/* -----------------------------------
 * General Macros
 * -----------------------------------
 */

#define myMIN(x,y)	( (x) <= (y) ? (x) : (y) )
#define myMAX(x,y)	( (x) >= (y) ? (x) : (y) )

/* cross-platform alternative to Windows system("pause") */
#define pressENTER()							\
do{									\
	int mYcHAr;							\
	printf( "press ENTER..." );					\
	fflush(stdout);							\
	while ( (mYcHAr=getchar()) != '\n' && mYcHAr != EOF )		\
		;							\
}while(0)

/* print empty lines on the stdout */
#define CLS()								\
do {									\
	int i = FMT_PGLINES+4;						\
	while ( i-- )							\
		putchar('\n');						\
} while (0)

/* console backspace ntimes */
#define NBS( ntimes )							\
do {									\
	int nTImeSTMp = (ntimes);					\
	while (nTImeSTMp--)						\
		putchar('\b');						\
} while(0)

/* display a spin effect on console */
#define FANCYSPIN() 	printf("|\b"); printf("/\b"); printf("-\b"); printf("\\\b")
#define BELL(ntimes)							\
do {									\
	int nTImeSTMp = (ntimes);					\
	while (nTImeSTMp--)						\
		putchar('\a');						\
} while(0)


/* -----------------------------------
 * Currently unused Constants & Macros
 * -----------------------------------
 */

// #define HEXCHARS	"0123456789ABCDEF"
// #define IS_HEXCHR(c)	( strchr(HEXCHARS,toupper(c)) != NULL )

#endif						/* end of inclusion guard            */
