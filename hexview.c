/*****************************************************//**
 * @brief    Prmitive hexadecimal file viewer (pipe it to 'less' or 'more'
 *        to make it much less primitive... see Usage below).
 * @file    hexview.c
 * @version    0.26A Public (only for alpha-testing)
 * @date    29 Feb, 2012
 * @author     migf1 <mig_f1@hotmail.com>
 * @par Language:
 *        C (ANSI C99)
 * @par Usage:
 *        hexview [-raw] [filename]
 *        \n
 *        Use -raw as the 1st command-line argument to force raw output
 *        (useful for piping, e.g: hexview -raw file | less)
 *
 * @remark    Feel free to experiment with the values of the pre-processor
 *         constants: FMT_NCOLS, FMT_PGLINES and FMT_POS. They affect the
 *        appearance of the output.
 *********************************************************
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

#include "con_color.h"
#include "hexview.h"

typedef unsigned char Byte;

typedef struct Buffer {
    char    fname[ MAXINPUT ];    /* name of the file to be viewed      */
    size_t    len, nrows, npages;    /* total Bytes, rows and pages        */
//    size_t     bt, row, pg;        /* current byte, row & page indicies  */
    Byte     *data;            /* the actual data buffer             */
} Buffer;

typedef struct Settings {
    _Bool colorize;
    unsigned short int charset;
    _Bool israw;
    _Bool unlimfsize;
} Settings;

enum KeyCommand {
    KEY_HLP        = 'h',
    KEY_QUIT    = 'q',
    KEY_COLOR    = 'c',
    KEY_CHARSET    = 't',
    KEY_LOADFILE    = 'f',
    KEY_TOP     = '{',
    KEY_BOT     = '}',
    KEY_PGUP     = '[',
    KEY_PGDN     = ']',
    KEY_ROWUP     = ',',
    KEY_ROWDN     = '.',
    KEY_ROWSTA    = '(',
    KEY_ROWEND    = ')',
    KEY_BYTEB     = '<',
    KEY_BYTEF    = '>',
    KEY_GPAGE    = 'p',
    KEY_GROW    = 'r',
    KEY_GBYTE    = 'b',
    KEY_FNDSTR    = '/',
    KEY_RFNDSTR    = '\\',
    KEY_FNDSEQ    = ';',
    KEY_RFNDSEQ    = ':',
};

void     buffer_cleanup( Buffer *buffer );
_Bool     buffer_read_file_longmax( Buffer *buffer, const char *fname );
_Bool     buffer_read_file( Buffer *buf, const char *fname, size_t chunklen );


/*********************************************************//**
 * Return the size of a byte in bits
 *************************************************************
 */
size_t bin_bitcount( Byte byte )
{
    return 8U * sizeof(byte);
}

/*********************************************************//**
 * Return a bitstring from a given (decimal) byte.
 *************************************************************
 */
char *bin_byte2bitstring( Byte byte )
{
    int len     = bin_bitcount( byte );
    char *bitstring = calloc( len + 1, sizeof(char) );
    if ( !bitstring )
        return NULL;

    memset(bitstring, '0', len--);
    while ( byte && len > -1) {
        bitstring[len--] = (byte & 1) ? '1' : '0';
        byte >>= 1;
    }

    return bitstring;
}

/*********************************************************//**
 * Read a c-string from stdin.
 *************************************************************
 */
char *s_read( char *s, size_t maxlen )
{
    size_t len = 0U;

    if ( !s || !fgets( s, maxlen, stdin) )
        return NULL;

    if ( maxlen > MAXINPUT)
        maxlen = MAXINPUT;

    if ( s[ (len=strlen(s))-1 ] == '\n')
        s[ len-1 ] = '\0';

    return s;
}

/*********************************************************//**
 * 
 *************************************************************
 */
_Bool fileexists(const char *fname)
{
    FILE *fp = fopen(fname, "rb");
    if ( !fp )
        return false;

    fclose(fp);
    return true;
}

/*********************************************************//**
 * Return the size of a file in bytes, or -1 on error (cannot be more than LONG_MAX).
 *************************************************************
 */
long int filesize(const char *fname)
{
    long int size;
    FILE      *fp;

    if ( NULL == (fp = fopen(fname, "rb")) )    /* binary mode */
        return -1;

    if( fseek(fp, 0, SEEK_END) ) {
        fclose(fp);
        return -1;
    }

    size = ftell(fp);
    fclose(fp);

    return size;
}

/*********************************************************//**
 * Display help screen.
 *************************************************************
 */
void show_help( const unsigned short int colorize )
{
    CLS();

    colorPRINTF( colorize, FGCLR_EM1, BG_NOCHANGE, "%52s\n\n", NAME_VERSION ); 

#if DISABLED    // ==============================================
    /* list of command line arguments */

    colorPRINTF( colorize, FGCLR_EM1, BG_NOCHANGE, "Command Line Arguments\n\n" );

    puts( "-raw \t\t Force raw output (useful for piping)" );

    puts( "\0" );
#endif        // ==============================================

    /* list of available commands */

    colorPRINTF(colorize, FGCLR_EM1, BG_NOCHANGE, "Available Commands\n"); 

    puts( "Commands are NOT case-sensitive but require you to press ENTER at the end.\n" );
    printf( "%c \t\t This help screen\n",             KEY_HLP );
    printf( "%c \t\t Quit the program\n",             KEY_QUIT );
    puts( "ENTER \t\t Repeat last command" );
    printf( "%cfilename\t Load a new file (leave NO blanks between %c and filename)\n",
        KEY_LOADFILE, KEY_LOADFILE
    );
    printf( "%c \t\t Toggle colorization (on/off)\n",    KEY_COLOR );
    printf(    "%c\t\t Toggle ASCII character set (plain/extended)\n", KEY_CHARSET);

    putchar('\n');

    printf(    "%c or %c \t\t Start or end of file\n",     KEY_TOP, KEY_BOT );
    printf(    "%c or %c \t\t Start or end of row (line)\n",     KEY_ROWSTA, KEY_ROWEND);
    printf(    "%c or %c n\t Move n bytes back or forward (1 is assumed if no n is present)\n",
        KEY_BYTEB, KEY_BYTEF
    );
    printf(    "%c or %c n\t Move n rows  up or down (1 is assumed if no n is present)\n",
        KEY_ROWUP, KEY_ROWDN
    );
    printf( "%c or %c n\t Move n pages up or down (1 is assumed if no n is present)\n",
        KEY_PGUP, KEY_PGDN );
    printf( "%c n \t\t Goto n'th byte (0xn for hex, 0n for oct)\n", KEY_GBYTE);
    printf( "%c n \t\t Goto n'th row  (0xn for hex, 0n for oct)\n", KEY_GROW );
    printf( "%c n \t\t Goto n'th page\n", KEY_GPAGE);

    putchar('\n');

    printf(    "%c or %c string\t Search ahead or backwards for a text-string (case sensitive)\n",
        KEY_FNDSTR, KEY_RFNDSTR
    );
    printf(    "%c or %c sequence\t Search ahead or backwards for a byte-sequence\n",
        KEY_FNDSEQ, KEY_RFNDSEQ
    );

    putchar('\n');
    pressENTER();

    return;
}

/*********************************************************//**
 * Display text labels and other info on the currently displayed page.
 *************************************************************
 */
void show_header( const size_t btcurr, const Buffer *buffer, const Settings *settings )
{
    unsigned short int i;
    const unsigned short totalcols = (FMT_OFST + 1) + (3*FMT_NCOLS) + FMT_NCOLS + (FMT_NCOLS / FMT_GRPCOLS) - 1;

    if ( !buffer || !buffer->data )
        return;

    CLS();

    /* text label for file-offset */
    colorPRINTF(
        settings->colorize, FGCLR_ROWOFST, BG_NOCHANGE,
        " %-*s ", FMT_OFST,"OFFSET"
    );

    /* hex indicies for Bytes */
    for (i=0; i < FMT_NCOLS; i++)
    {
        if ( i != 0 && i % FMT_GRPCOLS == 0 )        /* group columns      */
            putchar(' ');

        if ( btcurr % FMT_NCOLS == i )
            colorPRINTF(
                settings->colorize, FGCLR_BYTCURR, BG_NOCHANGE,
                "%-1hX* ", i
            );
        else
            colorPRINTF(
                settings->colorize, FGCLR_ROWOFST, BG_NOCHANGE,
                "%-2hX ", i
            );
    }

    /* hex indicies for Characters */
    putchar(' ');
    for (i=0; i < FMT_NCOLS; i++)
    {
        if ( btcurr % FMT_NCOLS == i )
            colorPRINTF(
                settings->colorize, FGCLR_BYTCURR, BG_NOCHANGE, "*"
            );
        else
            colorPRINTF(
                settings->colorize, FGCLR_ROWOFST,BG_NOCHANGE,
                "%hX", i
            );
    }
    putchar('\n');

    /* separating lines */

    putchar(' ');
    for (i=0; i < totalcols - FMT_NCOLS - 1; i++)
        colorPRINTF( settings->colorize, FGCLR_ROWOFST, BG_NOCHANGE, "-" );
    printf("  ");
    for (i=0; i < FMT_NCOLS; i++)
        colorPRINTF( settings->colorize, FGCLR_ROWOFST, BG_NOCHANGE, "-" );

    putchar('\n');

    return;
}

/*********************************************************//**
 * Return user selection on page-breaks
 *************************************************************
 */
_Bool show_prompt(
    const size_t     bt,
    char         *cmd,
    const char     *prevcmd,
    const Buffer     *buffer,
    const Settings    *settings
)
{
    size_t row = 0U, slen = 0U;
    char *cp = NULL, *bitstr = NULL;

    if ( !cmd || !prevcmd || !buffer || !settings || !buffer->data)
        return false;

    /* -----------------------
     * display 1st prompt line
     */

    /* filename (display FNAME_SHOWLEN last chars, if longer) */
    slen = strlen( buffer->fname );
    if ( slen > FNAME_SHOWLEN )
        cp = (char *) &buffer->fname[ slen-FNAME_SHOWLEN ];
    else
        cp = (char *) &buffer->fname;
    colorPRINTF(
        settings->colorize,
        FGCLR_PMTFNAME,
        BGCLR_PMTFNAME,
        " %s%s ",
        cp != buffer->fname ? "..." : "\0",
        cp
    );

    putchar(':');

    /* filesize (in Mbytes) */
    colorPRINTF(
        settings->colorize, FGCLR_PMTFNAME, BGCLR_PMTFNAME,
        " %.3f Mb :", ( (buffer->len * sizeof(Byte)) / (1024.0 * 1024) )
    );

    /* filesize in viewer-rows */
    colorPRINTF(
        settings->colorize, FGCLR_PMTFNAME, BGCLR_PMTFNAME,
        " %llu rows ", (unsigned long long) (buffer->nrows)
    );

    row = BT2ROW(bt, buffer->len, buffer->nrows);    /* calc the row index for bt */

    putchar('|');

    /* filesize in viewer-pages */
    colorPRINTF(
        settings->colorize, FGCLR_PMTPG, BGCLR_PMTPG,
        " Pg:%llu/%llu ",
        (unsigned long long) (1 + ROW2PG(row, buffer->nrows)),
        (unsigned long long) (buffer->npages)
    );

    putchar('|');

    /* selected character set */
    colorPRINTF(
        settings->colorize, FGCLR_PMTCHRSET, BGCLR_PMTCHRSET,
        " %s ", NAME_CHARSET(settings->charset)
    );
    puts("\0");

    /* -----------------------
     * display 2nd prompt line
     */

    /* current byte position */
    colorPRINTF(
        settings->colorize, FGCLR_PMTBYTPOS, BGCLR_PMTBYTPOS,
        " %llu=%llx/%llX ",
        (unsigned long long) bt,
        (unsigned long long) bt,
        (unsigned long long) buffer->len - 1
    );
    /* current row position */
    colorPRINTF(
        settings->colorize, FGCLR_PMTROWPOS, BGCLR_PMTROWPOS,
        "[%llx] ",
        (unsigned long long) row
    );

    putchar('|');
    /* decimal values of current byte (unsigned & signed) */
    colorPRINTF(
        settings->colorize, FGCLR_PMTDEC, BGCLR_PMTDEC,
        " d:%hhu %hhd ",
        buffer->data[bt],
        buffer->data[bt] > 127 ? buffer->data[bt] - 256 : buffer->data[bt]
    );

    putchar('|');
    /* octal value of current byte */
    colorPRINTF(
        settings->colorize, FGCLR_PMTOCT, BGCLR_PMTOCT,
        " o:%hho ",
        buffer->data[bt]
    );

    putchar('|');
    /* binary value of current byte */
    colorPRINTF(
        settings->colorize, FGCLR_PMTBIN, BGCLR_PMTBIN,
        " %s ",
        (bitstr = bin_byte2bitstring( buffer->data[bt] )) ? bitstr : "error"
    );
    if ( bitstr )
        free(bitstr);

    putchar('|');

    /* previous command */
    colorPRINTF(settings->colorize, FGCLR_PMTPRVCMD, BGCLR_PMTPRVCMD, " %s ", prevcmd );

    /* user prompt */
    colorPRINTF(settings->colorize, FGCLR_EM1, BG_NOCHANGE, ": ");

    fflush( stdout );

    /* read the user command */
    if ( !fgets( cmd, MAXINPUT, stdin ) )
        return false;

    /* if not just an ENTER, remove '\n' from the end */
    slen = strlen(cmd);
    if ( '\n' != *cmd && '\n' == cmd[ slen-1 ] )
        cmd[ slen-1 ] = '\0';

    return true;
}

/*********************************************************//**
 * List the contents of a row in hex/char format.
 *************************************************************
 */
_Bool view_row(
    const size_t     row,
    const size_t     btcurr,
    const Buffer     *buffer,
    const Settings     *settings
)
{
    char clead;                    /* leading displayed char     */
    size_t i;
    const size_t row2idx = row * FMT_NCOLS;
    int fgcolor;

    if ( !buffer || !buffer->data || row2idx > buffer->len )
        return false;

    /* display row offset */
    clead     = row == btcurr / FMT_NCOLS ? '*' : ' ';
    fgcolor = row == btcurr / FMT_NCOLS ? FGCLR_BYTCURR : FGCLR_ROWOFST;
    colorPRINTF(
        settings->colorize, fgcolor, BG_NOCHANGE,
        "%c%0*lX ",
        clead, FMT_OFST,(long unsigned)row2idx
    );


    /* display row's contents as bytes */
    for (i=0; i < FMT_NCOLS && row2idx + i < buffer->len; i++)
    {
        Byte byte = buffer->data[row2idx + i];
        _Bool currcharset =     (settings->charset == FMT_ASCII)
                    ? isprint((int)byte)
                    : byte > 31;

        if ( i != 0 && i % FMT_GRPCOLS == 0 )        /* group columns      */
            putchar(' ');

        if ( currcharset ) {            /* plain or extended ascii ? */
            fgcolor = btcurr != row2idx + i ? FGCLR_BYTPRT1 : FGCLR_BYTCURR;
            colorPRINTF(
                settings->colorize, fgcolor, BG_NOCHANGE,
                "%02hX ", byte
            );
        }
        else if ( byte == 0 ) {                /* zeroed byte        */
            fgcolor = btcurr != row2idx + i ? FGCLR_BYTZERO : FGCLR_BYTCURR;
            colorPRINTF(
                settings->colorize, fgcolor, BG_NOCHANGE,
                "%02hX ", byte
            );
        }
        else {                        /* non-printable byte */
            fgcolor = btcurr != row2idx + i ? FGCLR_BYTPRT0 : FGCLR_BYTCURR;
            colorPRINTF(
                settings->colorize, fgcolor, BG_NOCHANGE,
                "%02hX ", byte
            );
        }
    }
    for (; i < FMT_NCOLS; i++)
        printf("   ");
    putchar(' ');

    /* display row's contents as chars */
    for (i=0; i < FMT_NCOLS && row2idx + i < buffer->len; i++)
    {
        int c = (int)buffer->data[row2idx + i];
        _Bool currcharset =     (settings->charset == FMT_ASCII)
                    ? isprint(c)
                    : (c>31);

        if ( currcharset ) {            /* plain or extended ascii ? */
            fgcolor = btcurr != row2idx + i ? FGCLR_BYTPRT1 : FGCLR_BYTCURR;
            colorPRINTF( settings->colorize, fgcolor, BG_NOCHANGE, "%c", c);
        }
        else if ( c == '\0' ) {                /* zero-byte char     */
            fgcolor = btcurr != row2idx + i ? FGCLR_BYTZERO : FGCLR_BYTCURR;
            colorPRINTF( settings->colorize, fgcolor, BG_NOCHANGE, "." );
        }
        else {                        /* non-printable char */
            fgcolor = btcurr != row2idx + i ? FGCLR_BYTPRT0 : FGCLR_BYTCURR;
            colorPRINTF( settings->colorize, fgcolor, BG_NOCHANGE, "." );
        }
    }
    for (; i < FMT_NCOLS; i++)
        putchar(' ');
    putchar('\n');


    return true;
}

/*********************************************************//**
 *
 *************************************************************
 */
_Bool view_screen( const size_t btcurr, const Buffer *buffer, const Settings *settings )
{
    size_t i, rowstart;                /* for parsing rows */

    if ( !buffer || !buffer->data )
        return false;

    rowstart = BT2ROW(btcurr, buffer->len, buffer->nrows);

    for (i=0; i < FMT_PGLINES && (i+rowstart) < buffer->nrows; i++)
        view_row( (i+rowstart), btcurr, buffer, settings );

    /* if necessary, fill rest of the page with blank rows */
    for (; i < FMT_PGLINES; i++)
        putchar('\n');

    return true;
}

/*********************************************************//**
 *
 *************************************************************
 */
_Bool do_command(
    size_t         *bt,        /* by reference */
    const char     *cmd,
    const char     *prevcmd,
    Buffer         *buffer,
    Settings     *settings    /* by reference */
)
{
    enum KeyCommand key;
    size_t row;

    if ( !buffer || !buffer->data )
        return false;

    /* execute the command */

    row = BT2ROW( *bt, buffer->len, buffer->nrows );
    key = tolower( *cmd );

    /* display the help screen */
    if ( KEY_HLP == key ) {
        show_help( settings->colorize );
        return true;
    }

    /* load a new file */
    if ( KEY_LOADFILE == key )
    {
        _Bool success = false;
        char answer[256+1] = {'n'};
        char tmpfname[ MAXINPUT ] = {'\0'};

        if ( '\0' == cmd[1] ) {
            printf( "f must be followed by a filename! " );
            pressENTER();
            return true;
        }

        strncpy( tmpfname, &cmd[1], MAXINPUT );
        if ( !fileexists(tmpfname) ) {
            printf( "No such file, keeping current one! " );
            pressENTER();
            return true;
        }

        colorPRINTF(
            settings->colorize, FG_RED, BG_NOCHANGE,
            "Current file will be closed, are you sure (y/) ? "
        );
        if ( 'y' != tolower( *s_read(answer, 256+1) ) )
            return true;

        buffer_cleanup( buffer );
        success = (settings->unlimfsize) 
            ? buffer_read_file( buffer, tmpfname, 100*1024*1024 )
            : buffer_read_file_longmax( buffer, tmpfname );
        if ( !success ) {
            perror(NULL);
            return true;
        }

        return true;
    }

    /* toggle colorization */
    if ( KEY_COLOR == key ) {
        settings->colorize = (settings->colorize == true ) ? false : true;
        return true;
    }

    /* toggle character set */
    if ( KEY_CHARSET == key ) {
        settings->charset =     (settings->charset == FMT_ASCII)
                    ? FMT_XASCII : FMT_ASCII;
        return true;
    }

    /* byte back/forward */
    if ( KEY_BYTEB == key || KEY_BYTEF == key )
    {
        /* get relative step (if any) */
        long long step = strtoll( &cmd[1], NULL, 10 );
        if (errno == ERANGE)            /* over/underflowed step     */
            return true;
        if ( step == 0 )            /* no step ? assume 1        */
            step = 1;
        else if ( step < 0 )            /* negative step? negate it  */
            step = -step;

        /* apply relative row-step */
        if ( KEY_BYTEB == key )
            *bt =     (*bt > step - 1) ? *bt - step : 0;
        else /* KEY_BYTEF == key */
            *bt =     (*bt < buffer->len - 1 - step)
                ? *bt + step
                : buffer->len - 1;

        return true;
    }

    /* row start/end */
    if ( KEY_ROWSTA == key ) {
        *bt = ROW2BT(row, buffer->nrows);
        return true;
    }
    if ( KEY_ROWEND == key ) {
        *bt = ROW2BT(row, buffer->nrows) + FMT_NCOLS - 1;
        return true;
    }

    /* row up/down */
    if ( KEY_ROWUP == key || KEY_ROWDN == key )
    {
        /* get relative row-step (if any) */
        long long step = strtoll( &cmd[1], NULL, 10 );
        if (errno == ERANGE)            /* over/underflowed step     */
            return true;
        if ( step == 0 )            /* no step ? assume 1        */
            step = 1;
        else if ( step < 0 )            /* negative step? negate it  */
            step = -step;

        /* apply relative row-step */
        if ( KEY_ROWUP == key )
            row =     (row > step - 1) ? row - step : 0;
        else /* KEY_ROWDN == key */
            row =     (row < buffer->nrows - 1 - step)
                ? row + step
                : buffer->nrows - 1;
    }

    /* page up/down */
    else if ( KEY_PGUP == key || KEY_PGDN == key )
    {
        /* get relative page-step (if any) */
        long long step = strtoll( &cmd[1], NULL, 10 );
        if (errno == ERANGE)            /* over/underflowed step     */
            return true;
        if ( step == 0 )            /* no step ? assume 1        */
            step = 1;
        else if ( step < 0 )            /* negative step? negate it  */
            step = -step;

        /* apply relative page-step */
        if ( KEY_PGUP == key )
            row =     row > step * (FMT_PGLINES - 1)
                ? row - step * FMT_PGLINES
                : 0;
        else /* KEY_PGDN == key */
            row =     myMIN( (row + step * FMT_PGLINES), buffer->nrows - 1 );
    }

    /* top/bottom of file */
    else if ( KEY_TOP == key ) {
        *bt = 0;
        return true;
    }
    else if ( KEY_BOT == key ) {
        *bt = buffer->len - 1;
        return true;
    }

    /* goto to specified byte */
    else if ( KEY_GBYTE == key ) {
        size_t newbt;
        sscanf( &cmd[1], "%li", (long int *)&newbt );
        row =     BT2ROW(newbt, buffer->len, buffer->nrows);
        *bt = newbt > buffer->len - 1 ? buffer->len - 1 : newbt;
        return true;
    }
    /* goto to specified row */
    else if ( KEY_GROW == key ) {
        size_t newrow;
        sscanf( &cmd[1], "%li", (long int *)&newrow );
        row =     newrow > buffer->nrows - 1
            ? buffer->nrows - 1
            : newrow > 0 ? newrow : 0;
    }
    /* goto to specified page */
    else if ( KEY_GPAGE == key ) {
        size_t newpg = strtoul( &cmd[1], NULL, 10 );
        --newpg;
        row = PG2ROW(newpg, buffer->npages);
    }

    /* search forward for text-string */
    else if ( KEY_FNDSTR == key )
    {
        size_t ibt = *bt;            /* remember cursor position   */
        size_t slen = strlen( &cmd[1] );
        int tmp = 1;

        if ( !strcmp(cmd, prevcmd) )        /* fix ibt if cmd == prevcmd  */
            ibt = ibt > 0 ? ibt+1 : 1;

        /* do the search */
        colorPRINTF(settings->colorize, FG_RED, BG_NOCHANGE, "searching..." );
        while ( slen && ibt < buffer->len - slen + 1
            && (tmp = memcmp(&buffer->data[ibt], (Byte *)&cmd[1], slen * sizeof(Byte)))
        )
            ibt++;
            
        if ( 0 == tmp )                /* string found               */
            *bt = ibt;            /* ... update cursor position */
        else
            BELL(1);

        return true;
    }
    /* search backwards for text-string */
    else if ( KEY_RFNDSTR == key )
    {
        size_t ibt = *bt;            /* remember cursor position   */
        size_t slen = strlen( &cmd[1] );
        int tmp = 1;

        if ( !strcmp(cmd, prevcmd) )        /* fix ibt if cmd == prevcmd  */
            ibt = ibt > 0 ? ibt-1 : 0;

        /* do the search */
        colorPRINTF(settings->colorize, FG_RED, BG_NOCHANGE, "searching..." );
        while ( slen && ibt > 0
            && (tmp=memcmp(&buffer->data[ibt], (Byte *)&cmd[1], slen * sizeof(Byte)))
        )
            ibt--;
        /* 0 is treaded separately because ibt is unsigned */
        if ( ibt == 0 && tmp != 0 )
            tmp = memcmp(&buffer->data[ibt], (Byte *)&cmd[1], slen * sizeof(Byte));

        if ( 0 == tmp )                /* string found               */
            *bt = ibt;            /* ... update cursor position */
        else
            BELL(1);

        return true;
    }

    /* search forward for byte-sequence */
    else if ( KEY_FNDSEQ == key )
    {
        size_t     ibt = *bt;            /* remember cursor position   */
        size_t     iseq;                /* eventually the len of seq[]*/
        Byte     seq[MAXINPUT] = {0};        /* to hold the byte-sequence  */
        int     tmp = 1;            /* general purpose temp int   */

        if ( !strcmp(cmd, prevcmd) )        /* fix ibt if cmd == prevcmd  */
            ibt = ibt > 0 ? ibt+1 : 1;

        /* convert text-string &cmd[1] to byte-sequence seq[] */
        tmp = iseq = 0;
        while ( 1 == sscanf(&cmd[tmp+1], "%2hhx", (Byte *)&seq[iseq]) )
        {
            tmp += 2;
            iseq++;
        }
        seq[iseq] = '\0';            /* this is just for debugging */

        /* do the search */
        colorPRINTF(settings->colorize, FG_RED, BG_NOCHANGE, "searching..." );
        while ( iseq && ibt < buffer->len - iseq + 1
            && (tmp=(memcmp( &buffer->data[ibt], seq, iseq*sizeof(Byte) )))
        )
            ibt++;
            
        if ( 0 == tmp )                /* sequence found             */
            *bt = ibt;            /* ... update cursor position */
        else
            BELL(1);

        return true;
    }
    /* search backwards for byte-sequence */
    else if ( KEY_RFNDSEQ == key )
    {
        size_t     ibt = *bt;            /* remember cursor position   */
        size_t     iseq;                /* eventually the len of seq[]*/
        Byte     seq[MAXINPUT] = {0};        /* to hold the byte-sequence  */
        int     tmp = 1;            /* general purpose temp int   */

        if ( !strcmp(cmd, prevcmd) )        /* fix ibt if cmd == prevcmd  */
            ibt = ibt > 0 ? ibt-1 : 0;

        /* convert text-string &cmd[1] to byte-sequence seq[] */
        tmp = iseq = 0;
        while ( 1 == sscanf(&cmd[tmp+1], "%2hhx", (Byte *)&seq[iseq]) )
        {
            tmp += 2;
            iseq++;
        }
        seq[iseq] = '\0';            /* this is just for debugging */

        /* do the search */
        colorPRINTF(settings->colorize, FG_RED, BG_NOCHANGE, "searching..." );
        while ( iseq && ibt > 0
            && (tmp=memcmp(&buffer->data[ibt], seq, iseq * sizeof(Byte))) )
            ibt--;
        /* 0 is treaded separately because ibt is unsigned */
        if ( ibt == 0 && tmp != 0 )
            tmp = memcmp(&buffer->data[ibt], seq, iseq * sizeof(Byte));

        if ( 0 == tmp )                /* string found               */
            *bt = ibt;            /* ... update cursor position */
        else
            BELL(1);

        return true;
    }

/*
    else
        (*row) += FMT_PGLINES;
*/
    *bt = ROW2BT( row, buffer->nrows ) + ( *bt % FMT_NCOLS );

        return true;
}

/*********************************************************//**
 *
 *************************************************************
 */
_Bool view_buffer( Buffer *buffer, Settings *settings )
{
    size_t bt;                    /* current byte-index        */
    char cmd[ MAXINPUT ] = {']'}, prevcmd[ MAXINPUT ] = {'\0'};

    if ( !buffer || !buffer->data || !settings )
        return false;

    bt = 0;
    for (;;)
    {
        if ( !settings->israw )            /* no headers in raw-mode     */
            show_header( bt, buffer, settings );

        view_screen( bt, buffer, settings );

        if ( settings->israw )            /* no interaction in raw-mode */
            continue;

        strcpy( prevcmd, cmd );            /* backup current command     */

        /* get new command */
        if ( !show_prompt(bt, cmd, prevcmd, buffer, settings) )
            continue;

        if ( KEY_QUIT == *cmd )
            break;

        if ( '\n' == *cmd )            /* restore previous command  */
            strcpy( cmd, prevcmd );

        do_command( &bt, cmd, prevcmd, buffer, settings );

    }

    putchar('\n');

    return true;
}

/*********************************************************//**
 *
 *************************************************************
 */
void buffer_dump( const Buffer *buffer )
{
    size_t    i = 0U;

    if ( !buffer || !buffer->data)
        return;

    for (i=0U; i < buffer->len; i++)
        putchar( (int)buffer->data[i] );
    fflush( stdout );

    return;
}


/*********************************************************//**
 *
 *************************************************************
 */
void buffer_cleanup( Buffer *buffer )
{
    if ( !buffer )
        return;

    if ( buffer->data ) {
        free(buffer->data);
        buffer->data = NULL;
    }

    memset( buffer, 0, sizeof(Buffer) );
    return;
}

/*********************************************************//**
 * Read into a Buffer structure the contents of a file with size up tp LONG_MAX bytes.
 *************************************************************
 */
_Bool buffer_read_file_longmax( Buffer *buffer, const char *fname )
{
    long int buflen, bufsize;
    FILE     *fp = NULL;

    if ( !buffer || !fname || '\0' == *fname )
        return false;

    if ( -1 == (bufsize = filesize(fname)) || NULL == (fp = fopen(fname, "rb")) )
        return false;

    /* make sure our Buffer struct starts with zeroed fields */
    memset( buffer, 0, sizeof(Buffer) );

    /* allocate room in memory for our buffer->data (+1 for zero-terminator) */
    buflen = bufsize / sizeof(Byte);
    if ( NULL == (buffer->data = calloc( buflen+1, sizeof(Byte) )) )
        goto ret_failure;

    printf("Loading \"%s\"... ", fname);
    buflen = fread( buffer->data, sizeof(Byte), buflen, fp);
    if ( ferror(fp) )
        goto ret_failure;

    fclose(fp);
    puts("\nDone!");

    /* update fields in our Buffer structure */
    strcpy( buffer->fname, fname );
    buffer->len     = buflen;
    buffer->nrows     = buffer->len/FMT_NCOLS + (buffer->len % FMT_NCOLS != 0 ?1 :0);
    buffer->npages     = buffer->nrows / FMT_PGLINES
            + (buffer->nrows % FMT_PGLINES != 0 ? 1 : 0 );

    return true;

ret_failure:
    if ( buffer->data )
        free( buffer->data );
    memset( buffer, 0, sizeof(buffer) );

    if ( fp )
        fclose(fp);
    return false;
}

/*********************************************************//**
 * Read contents of file of any size into the Buffer structure.
 *************************************************************
 */
_Bool buffer_read_file( Buffer *buf, const char *fname, size_t chunklen )
{
    size_t    n;                /* # of Bytes read from file to chunk */
    size_t     buflen = chunklen;        /* current length of our data buffer  */
    Byte    *try = NULL;            /* to test reallocs before applying'em*/
    FILE    *fp = NULL;

    if ( !buf || !fname || '\0' == *fname || NULL == (fp=fopen(fname, "rb")) )
        return false;

    /* make sure our Buffer struct starts with zeroed fields */
    memset( buf, 0, sizeof(Buffer) );

    /* allocate ahead room for chunklen Bytes */
    if ( NULL == (buf->data = calloc( buflen, sizeof(Byte) )) )
        return false;

    /* inform the user */
    printf(    "Loading \"%s\\n", fname );
    printf( "(in chunks of %llu bytes)\n",
        (unsigned long long)(chunklen * sizeof(Byte))
    );
    printf(    "Bytes loaded so far: ");

    /* read the file into our Buffer structure */
    while ( !feof(fp) )
    {
        /* read data from file into the newely allocated part of the Buffer */
        n = fread( &buf->data[buflen-chunklen], sizeof(Byte), chunklen, fp);
        if (ferror(fp) || n < chunklen) {
            goto ret_failure;
        }

        /* allocate ahead chunklen more Bytes */
        buflen += chunklen;
        if ( NULL == (try = realloc( buf->data, buflen * sizeof(Byte) )) )
            goto ret_failure;
        buf->data = try;

        /* display bytes read so far */
        NBS( printf("%llu", (unsigned long long)(buflen-chunklen) * sizeof(Byte)) );
    }
    putchar('\n');

    if ( ferror(fp) )
        goto ret_failure;
    fclose(fp);

    /* truncate buffer=>data to actual file length */
    buflen -= (chunklen - n);
    if (NULL == (try = realloc( buf->data, buflen * sizeof(Byte) )) )
        goto ret_failure;
    buf->data = try;

    /* update fields in our Buffer structure */
    strcpy( buf->fname, fname );
    buf->len     = buflen;
    buf->nrows     = buf->len / FMT_NCOLS + (buf->len % FMT_NCOLS != 0 ? 1 : 0);
    buf->npages     = buf->nrows / FMT_PGLINES
            + (buf->nrows % FMT_PGLINES != 0 ? 1 : 0 );

    return true;

ret_failure:
    if ( buf->data )
        free( buf->data );
    memset( buf, 0, sizeof(buf) );

    if ( fp )
        fclose(fp);
    return false;
}

/*********************************************************//**
 *
 *************************************************************
 */
int main( int argc, char *argv[] )
{
    _Bool     success = false;
    char     tmpfname[ MAXINPUT ] = {'\0'};

    /* our Buffer structure    */
    Buffer buffer = {                
        .fname = {'\0'},
        .len=0U, .nrows=0U, .npages=0U,
//        .bt=0U, .row=0U, .pg=0U,
        .data = NULL                /* ... the actual buffer     */
    };
    /* our Settings structure */
    Settings settings = {                
        .colorize     = true,
        .charset     = FMT_XASCII,        /* ... extended ASCII        */
        .israw        = false,
        .unlimfsize    = false
    };

    CONOUT_INIT();
    CONOUT_SET_COLOR( FGCLR_NORMAL );        /* set console fg color      */

    /* parse the command line */
    if ( 1 == argc )                /* no command line arguments */
        strncpy(tmpfname, argv[0], MAXINPUT-1 );
    else                        /* load self                 */
        strncpy(tmpfname, argv[1], MAXINPUT-1 );

    /* read the file into the buffer */
    success = (settings.unlimfsize) 
        ? buffer_read_file( &buffer, tmpfname, 100*1024*1024 )
        : buffer_read_file_longmax( &buffer, tmpfname );
    if ( !success ) {
        perror(NULL);
        goto exit_failure;
    }

    /* list the file contents */
    if ( !view_buffer( &buffer, &settings ) ) {
        perror(NULL);
        goto exit_failure;
    }

    buffer_cleanup( &buffer );

    CONOUT_RESTORE();
    exit( EXIT_SUCCESS );

exit_failure:
    buffer_cleanup( &buffer );

    CONOUT_RESTORE();
    pressENTER();
    exit( EXIT_FAILURE );
}
