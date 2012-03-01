#ifndef CON_COLOR_H
#define CON_COLOR_H

#include <string.h>

#if defined(_WIN32) || defined(_WIN64) || defined(__WINDOWS__) || defined(__TOS_WIN__)

    #define CON_COLOR_WIN32

#elif defined(__linux__) || defined(__unix__) || defined(__unix)        \
|| defined(__CYGWIN__) || defined(__GNU__) || defined(__MINGW32__)      \
|| defined(__MINGW64__)

    #define CON_COLOR_ANSI
#endif


#if 0   /* change to 1 to FORCE ANSI mode (for Windows you need: ansicon.exe)*/
    #ifdef CON_COLOR_WIN32
    #undef CON_COLOR_WIN32
    #endif

    #ifndef CON_COLOR_ANSI
    #define CON_COLOR_ANSI
    #endif
#endif

/* Windows ==================================================================== */
#if defined( CON_COLOR_WIN32 )

    #include <windows.h>

    HANDLE              hStdout;
    CONSOLE_SCREEN_BUFFER_INFO  csbiInfo, csbiSaved, csbiTemp;

    /* Win32 Macros */

    #define CONOUT_INIT()                           \
        do {                                \
            hStdout = GetStdHandle( STD_OUTPUT_HANDLE );        \
            if ( hStdout != INVALID_HANDLE_VALUE )          \
            {                           \
                GetConsoleScreenBufferInfo(hStdout, &csbiInfo); \
                memcpy( &csbiSaved, &csbiInfo,          \
                    sizeof(CONSOLE_SCREEN_BUFFER_INFO) );   \
            }                           \
            else                            \
                printf("*** CONOUT_INIT failed: %d!\n", GetLastError());\
        } while(0)

    #define CONOUT_SET_COLOR( color )                   \
        do {                                \
            memcpy( &csbiTemp, &csbiInfo,               \
                sizeof(CONSOLE_SCREEN_BUFFER_INFO) );       \
            csbiInfo.wAttributes = (color);             \
            SetConsoleTextAttribute( hStdout, (color) );        \
        } while(0)

    #define CONOUT_ADD_COLOR( color )                   \
        do {                                \
            csbiInfo.wAttributes |= (color);            \
            SetConsoleTextAttribute(hStdout, csbiInfo.wAttributes); \
        } while(0)
    
    #define CONOUT_RESET()                          \
        do {                                \
            SetConsoleTextAttribute(hStdout, csbiTemp.wAttributes); \
            memcpy( &csbiInfo, &csbiTemp,               \
                sizeof(CONSOLE_SCREEN_BUFFER_INFO) );       \
        } while(0)

    #define CONOUT_RESTORE()                        \
        SetConsoleTextAttribute( hStdout, csbiSaved.wAttributes )

    #define CONOUT_PRINTF(fg, bg, ...)                  \
        do {                                \
            if ( (fg) != FG_NOCHANGE )              \
                CONOUT_SET_COLOR( (fg) );           \
            if ( (bg) != BG_NOCHANGE )              \
                CONOUT_ADD_COLOR( (bg) );           \
            printf( __VA_ARGS__ );                  \
            if ( (fg) != FG_NOCHANGE || (bg) != BG_NOCHANGE )   \
                CONOUT_RESET();                 \
        } while(0)


    /* Win32 Foreground Colors */

    #define FG_NOCHANGE -1
    #define BG_NOCHANGE -1

    #define FG_WHITE                            \
        FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_BLUE|FOREGROUND_INTENSITY

    #define FG_GRAY     FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_BLUE

    #define FG_DARKGRAY FOREGROUND_INTENSITY
    #define FG_BLACK    0x00

    #define FG_RED      FOREGROUND_RED|FOREGROUND_INTENSITY
    #define FG_DARKRED  FOREGROUND_RED

    #define FG_GREEN    FOREGROUND_GREEN|FOREGROUND_INTENSITY
    #define FG_DARKGREEN    FOREGROUND_GREEN

    #define FG_BLUE     FOREGROUND_BLUE|FOREGROUND_INTENSITY
    #define FG_DARKBLUE FOREGROUND_BLUE

    #define FG_YELLOW   FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_INTENSITY
    #define FG_DARKYELLOW   FOREGROUND_RED|FOREGROUND_GREEN

    #define FG_MAGENTA  FOREGROUND_RED|FOREGROUND_BLUE|FOREGROUND_INTENSITY
    #define FG_DARKMAGENTA  FOREGROUND_RED|FOREGROUND_BLUE

    #define FG_CYAN     FOREGROUND_GREEN|FOREGROUND_BLUE|FOREGROUND_INTENSITY
    #define FG_DARKCYAN FOREGROUND_GREEN|FOREGROUND_BLUE

    /* Win32 Background Colors */

    #define BG_WHITE                            \
        BACKGROUND_RED|BACKGROUND_GREEN|BACKGROUND_BLUE|BACKGROUND_INTENSITY

    #define BG_GRAY     BACKGROUND_RED|BACKGROUND_GREEN|BACKGROUND_BLUE

    #define BG_DARKGRAY BACKGROUND_INTENSITY
    #define BG_BLACK    0x00

    #define BG_RED      BACKGROUND_RED|BACKGROUND_INTENSITY
    #define BG_DARKRED  BACKGROUND_RED

    #define BG_GREEN    BACKGROUND_GREEN|BACKGROUND_INTENSITY
    #define BG_DARKGREEN    BACKGROUND_GREEN

    #define BG_BLUE     BACKGROUND_BLUE|BACKGROUND_INTENSITY
    #define BG_DARKBLUE BACKGROUND_BLUE

    #define BG_YELLOW   BACKGROUND_RED|BACKGROUND_GREEN|BACKGROUND_INTENSITY
    #define BG_DARKYELLOW   BACKGROUND_RED|BACKGROUND_GREEN

    #define BG_MAGENTA  BACKGROUND_RED|BACKGROUND_BLUE|BACKGROUND_INTENSITY
    #define BG_DARKMAGENTA  BACKGROUND_RED|BACKGROUND_BLUE

    #define BG_CYAN     BACKGROUND_GREEN|BACKGROUND_BLUE|BACKGROUND_INTENSITY
    #define BG_DARKCYAN BACKGROUND_GREEN|BACKGROUND_BLUE

/* ANSI ====================================================================== */
#elif defined( CON_COLOR_ANSI )

    /* ANSI Macros */

    #define CONOUT_INIT()
    #define CONOUT_SET_COLOR( color )   printf( color )
    #define CONOUT_ADD_COLOR( color )   printf( color )
    #define CONOUT_RESET()          printf( "\033[0m" )
    #define CONOUT_RESTORE()        printf( "\033[0m" )

    #define CONOUT_PRINTF(fg, bg, ...)                  \
        do {                                \
            if ( strcmp((fg), FG_NOCHANGE) )            \
                CONOUT_SET_COLOR( (fg) );           \
            if ( strcmp((bg), BG_NOCHANGE) )            \
                CONOUT_ADD_COLOR( (bg) );           \
            printf( __VA_ARGS__ );                  \
            if ( strcmp((fg), FG_NOCHANGE) || strcmp((bg), BG_NOCHANGE) )\
                CONOUT_RESET();                 \
        } while(0)

    /* ANSI Foreground Colors */

    #define FG_NOCHANGE "\0"
    #define BG_NOCHANGE "\0"

    #define FG_WHITE    "\033[1;37m"
    #define FG_GRAY     "\033[0;37m"

    #define FG_DARKGRAY "\033[1;30m"
    #define FG_BLACK    "\033[0;30m"

    #define FG_RED      "\033[1;31m"
    #define FG_DARKRED  "\033[0;31m"

    #define FG_GREEN    "\033[1;32m"
    #define FG_DARKGREEN    "\033[0;32m"

    #define FG_BLUE     "\033[1;34m"
    #define FG_DARKBLUE "\033[0;34m"

    #define FG_YELLOW   "\033[1;33m"
    #define FG_DARKYELLOW   "\033[0;33m"

    #define FG_MAGENTA  "\033[1;35m"
    #define FG_DARKMAGENTA  "\033[0;35m"

    #define FG_CYAN     "\033[1;36m"
    #define FG_DARKCYAN "\033[0;36m"

    /* ANSI Background Colors */

    #define BG_WHITE    "\033[47m"
    #define BG_GRAY     "\033[47m"

    #define BG_DARKGRAY "\033[40m"
    #define BG_BLACK    "\033[40m"

    #define BG_RED      "\033[41m"
    #define BG_DARKRED  "\033[41m"

    #define BG_GREEN    "\033[42m"
    #define BG_DARKGREEN    "\033[42m"

    #define BG_BLUE     "\033[44m"
    #define BG_DARKBLUE "\033[44m"

    #define BG_YELLOW   "\033[43m"
    #define BG_DARKYELLOW   "\033[43m"

    #define BG_MAGENTA  "\033[45m"
    #define BG_DARKMAGENTA  "\033[45m"

    #define BG_CYAN     "\033[46m"
    #define BG_DARKCYAN "\033[46m"
#endif

#endif          /* ifndef CON_COLOR_H */

