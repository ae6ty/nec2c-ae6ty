/*
 * Miscellaneous support functions for nec2c.c
 */

#include "shared.h"

/*------------------------------------------------------------------------*/

/*  usage()
 *
 *  prints usage information
 */

void usage(void)
{
  fprintf( stderr,
	  "usage: nec2c [-i<input-file-name>] [-o<output-file-name>]"
	  "\n       -h: print this usage information and exit."
	  "\n       -v: print nec2c version number and exit.\n");

} /* end of usage() */

/*------------------------------------------------------------------------*/

/*  abort_on_error()
 *
 *  prints an error message and exits
 */

void abort_on_error( int why )
{
  switch( why )
  {
  case -20: //ae6ty 12-10-21
    fprintf( stderr, "%s\n",
	     "nec2c: Bad YY card format");
    break;
    break;
	case -1 : /* abort if input file name too long */
	  fprintf( stderr, "%s\n",
		  "nec2c: Input file name too long - aborting" );
	  break;

	case -2 : /* abort if output file name too long */
	  fprintf( stderr, "%s\n",
		  "nec2c: Output file name too long - aborting" );
	  break;

	case -3 : /* abort on input file read error */
	  fprintf( stderr, "%s\n",
		  "nec2c: Error reading input file - aborting" );
	  break;

	case -4 : /* Abort on malloc failure */
	  fprintf( stderr, "%s\n",
		  "nec2c: A memory allocation request has failed - aborting" );
	  break;

	case -5 : /* Abort if a GF card is read */
	  fprintf( stderr, "%s\n",
		  "nec2c: NGF solution option not supported - aborting" );
	  break;

	case -6: /* No convergence in gshank() */
	  fprintf( stderr, "%s\n",
		  "nec2c: No convergence in gshank() - aborting" );
	  break;

	case -7: /* Error in hankel() */
	  fprintf( stderr, "%s\n",
		  "nec2c: Hankel not valid for z=0. - aborting" );
	  break;
	  //ae6ty 12-7-21 failed to find Tag/Seg for YY card
  case -9:
    fprintf(stderr,"%s\n",
	    "nec2c: Couldn't find tag/segment for one of the YY card ports");
    break;
    //ae6ty 1-13-22  runaway memory problems.
  case -98:
    fprintf(stderr,"%s\n",
	    "nec2c: segment connection error\n");
    break;
  case -99:
    fprintf(stderr,"%s\n",
	    "nec2c: alloc of too large a memory chunk");
    break;
    
  }  /* switch( why ) */

  /* clean up and quit */
  stop( why );

} /* end of abort_on_error() */

/*------------------------------------------------------------------------*/

/* Returns process time (user+system) BUT in _msec_ */
//#ifndef WINDOWS
//void secnds( double *x)
//{
//  struct tms buffer;
//  double clk_tck;

//  times(&buffer);
//  clk_tck = sysconf( _SC_CLK_TCK );
//  *x = 1000.0 * (double)(buffer.tms_utime + buffer.tms_stime) / clk_tck;
//  return;
//}
//#else
#include <stdint.h>
extern uint64_t getTimeStamp();
void secnds( double *x) {
  uint64_t usecs = getTimeStamp();
  *x = (double)usecs/1e3;
}
//#endif

/*------------------------------------------------------------------------*/

/* Does the STOP function of fortran but with return value */
int stop( int flag )
{
  if( input_fp != NULL )
	fclose( input_fp );
  if( output_fp != NULL )
	fclose( output_fp );
  if( plot_fp != NULL )
	fclose( plot_fp );

  exit( flag );
}

/*------------------------------------------------------------------*/

/*  load_line()
 *
 *  loads a line from a file, aborts on failure. lines beginning
 *  with a '#' are ignored as comments. at the end of file EOF is
 *  returned.
 */

int load_line( char *buff, FILE *pfile )
{
  int
	num_chr, /* number of characters read, excluding lf/cr */
	eof = 0, /* EOF flag */
	chr;     /* character read by getc */

  num_chr = 0;

  /* clear buffer at start */
  buff[0] = '\0';

  /* ignore commented lines, white spaces and eol/cr */
  if( (chr = fgetc(pfile)) == EOF )
	return( EOF );

  while( (chr == '#') ||
	  (chr == ' ') ||
	  (chr == CR ) ||
	  (chr == LF ) )
  {
	/* go to the end of line (look for lf or cr) */
	while( (chr != CR) && (chr != LF) )
	  if( (chr = fgetc(pfile)) == EOF )
		return( EOF );

	/* dump any cr/lf remaining */
	while( (chr == CR) || (chr == LF) )
	  if( (chr = fgetc(pfile)) == EOF )
		return( EOF );

  } /* end of while( (chr == '#') || ... */

  while( num_chr < LINE_LEN )
  {
	/* if lf/cr reached before filling buffer, return */
	if( (chr == CR) || (chr == LF) )
	  break;

	/* enter new char to buffer */
	buff[num_chr++] = (char)chr;

	/* terminate buffer as a string on EOF */
	if( (chr = fgetc(pfile)) == EOF )
	{
	  buff[num_chr] = '\0';
	  eof = EOF;
	}

  } /* end of while( num_chr < max_chr ) */

  /* Capitalize first two characters (mnemonics) */
  if( (buff[0] > 0x60) && (buff[0] < 0x79) )
	buff[0] -= 0x20;
  if( (buff[1] > 0x60) && (buff[1] < 0x79) )
	buff[1] -= 0x20;

  /* terminate buffer as a string */
  buff[num_chr] = '\0';

  return( eof );

} /* end of load_line() */

/*------------------------------------------------------------------------*/

/***  Memory allocation/freeing utils ***/
void mem_alloc( void **ptr, size_t req )
{
  free_ptr( ptr );
  *ptr = malloc( req );
  if( *ptr == NULL )
	abort_on_error( -4 );

} /* End of void mem_alloc() */

/*------------------------------------------------------------------------*/
extern void print_trace(void);

void mem_realloc( void **ptr, size_t req, int tag)
{
  //  fprintf(stdout,"mem_realloc %ld %ld\n",(long)ptr,(long)req);
  if (req > 8e9) {
    fprintf(stdout," size too big %d, tag %d \n",(int)req, tag);
    print_trace();
    abort_on_error(-99);
  }
  *ptr = realloc( *ptr, req );
  if( *ptr == NULL )
	abort_on_error( -4 );

} /* End of void mem_realloc() */

/*------------------------------------------------------------------------*/

void free_ptr( void **ptr )
{
  if( *ptr != NULL )
	free( *ptr );
  *ptr = NULL;

} /* End of void free_ptr() */

/*------------------------------------------------------------------------*/

