/******************************************************************************
 * $Id$
 *
 * Project:  libtiff tools
 * Purpose:  Mainline for setting metadata in existing TIFF files.
 * Author:   Frank Warmerdam, warmerda@home.com
 *
 ******************************************************************************
 * Copyright (c) 2000, Frank Warmerdam
 *
 * Permission to use, copy, modify, distribute, and sell this software and 
 * its documentation for any purpose is hereby granted without fee, provided
 * that (i) the above copyright notices and this permission notice appear in
 * all copies of the software and related documentation, and (ii) the names of
 * Sam Leffler and Silicon Graphics may not be used in any advertising or
 * publicity relating to the software without the specific, prior written
 * permission of Sam Leffler and Silicon Graphics.
 * 
 * THE SOFTWARE IS PROVIDED "AS-IS" AND WITHOUT WARRANTY OF ANY KIND, 
 * EXPRESS, IMPLIED OR OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY 
 * WARRANTY OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.  
 * 
 * IN NO EVENT SHALL SAM LEFFLER OR SILICON GRAPHICS BE LIABLE FOR
 * ANY SPECIAL, INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY KIND,
 * OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER OR NOT ADVISED OF THE POSSIBILITY OF DAMAGE, AND ON ANY THEORY OF 
 * LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE 
 * OF THIS SOFTWARE.
 ******************************************************************************
 *
 * $Log$
 * Revision 1.6  2004-06-07 09:05:09  dron
 * Forgotten debug output removed.
 *
 * Revision 1.5  2004/06/05 09:05:26  dron
 * tiffset now can set any libtiff supported tags. Tags can be supplied by the
 * mnemonic name or number.
 *
 * Revision 1.4  2004/05/03 16:39:11  warmerda
 * Increase -sf buffer size.
 *
 * Revision 1.3  2002/01/16 17:50:05  warmerda
 * Fix bug in error output.
 *
 * Revision 1.2  2001/09/26 17:42:18  warmerda
 * added TIFFRewriteDirectory
 *
 * Revision 1.1  2001/03/02 04:58:53  warmerda
 * New
 *
 */


#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "tiffio.h"

static char* usageMsg[] = {
"usage: tiffset [options] filename",
"where options are:",
" -s <tagname> <value>...   set the tag value",
" -sf <tagname> <filename>  read the tag value from file (for ASCII tags only)",
NULL
};

static void
usage(void)
{
	int i;
	for (i = 0; usageMsg[i]; i++)
		fprintf(stderr, "%s", usageMsg[i]);
	exit(-1);
}

const TIFFFieldInfo *GetField(TIFF *tiff, const char *tagname)
{
    const TIFFFieldInfo *fip;

    if( atoi(tagname) > 0 )
        fip = TIFFFieldWithTag(tiff, (ttag_t)atoi(tagname));
    else
        fip = TIFFFieldWithName(tiff, tagname);

    if (!fip)
    {
        fprintf( stderr, "Field name %s not recognised.\n", tagname );
        return (TIFFFieldInfo *)NULL;
    }

    return fip;
}

int
main(int argc, char* argv[])
{
    TIFF *tiff;
    int  arg_index;

    if (argc < 2)
        usage();

    tiff = TIFFOpen(argv[argc-1], "r+");
    if (tiff == NULL)
        return -2;

    for( arg_index = 1; arg_index < argc-1; arg_index++ )
    {
        if( strcmp(argv[arg_index],"-s") == 0 && arg_index < argc-3 )
        {
            const TIFFFieldInfo *fip;

            arg_index++;
            fip = GetField(tiff, argv[arg_index]);

            if (!fip)
                return -3;

            arg_index++;
            if (fip->field_type == TIFF_ASCII)
            {
                TIFFSetField(tiff, fip->field_tag, argv[arg_index]);
                {
                    fprintf( stderr, "Failed to set %s=%s\n",
                             fip->field_name, argv[arg_index] );
                }
            }

            else if (fip->field_writecount > 0)
            {
                int     i;
                void    *array;

                if (argc - arg_index < fip->field_writecount)
                {
                    fprintf( stderr,
                             "Too few tag values: %d. "
                             "Expected %d values for %s tag\n",
                             argc - arg_index, fip->field_writecount,
                             fip->field_name );
                    return -4;
                }
                    

                array = malloc(fip->field_writecount
                               * TIFFDataWidth(fip->field_type));

                switch (fip->field_type)
                {
                    case TIFF_BYTE:
                        for (i = 0; i < fip->field_writecount; i++)
                            ((uint8 *)array)[i] = atoi(argv[arg_index+i]);
                        break;
                    case TIFF_SHORT:
                        for (i = 0; i < fip->field_writecount; i++)
                            ((uint16 *)array)[i] = atoi(argv[arg_index+i]);
                        break;
                    case TIFF_SBYTE:
                        for (i = 0; i < fip->field_writecount; i++)
                            ((int8 *)array)[i] = atoi(argv[arg_index+i]);
                        break;
                    case TIFF_SSHORT:
                        for (i = 0; i < fip->field_writecount; i++)
                            ((int16 *)array)[i] = atoi(argv[arg_index+i]);
                        break;
                    case TIFF_LONG:
                        for (i = 0; i < fip->field_writecount; i++)
                            ((uint32 *)array)[i] = atol(argv[arg_index+i]);
                        break;
                    case TIFF_SLONG:
                    case TIFF_IFD:
                        for (i = 0; i < fip->field_writecount; i++)
                            ((uint32 *)array)[i] = atol(argv[arg_index+i]);
                        break;
                    case TIFF_RATIONAL:
                    case TIFF_SRATIONAL:
                    case TIFF_DOUBLE:
                        for (i = 0; i < fip->field_writecount; i++)
                            ((double *)array)[i] = atof(argv[arg_index+i]);
                        break;
                    case TIFF_FLOAT:
                        for (i = 0; i < fip->field_writecount; i++)
                            ((float *)array)[i] = (float)atof(argv[arg_index+i]);
                        break;
                    default:
                        break;
                }
                
                if( TIFFSetField(tiff, fip->field_tag, array) != 1 )
                    fprintf( stderr, "Failed to set %s\n", fip->field_name );
                arg_index += fip->field_writecount;
                
                free(array);
            }
        }
        else if( strcmp(argv[arg_index],"-sf") == 0 && arg_index < argc-3 )
        {
            FILE    *fp;
            const TIFFFieldInfo *fip;
            char    *text;
            int     len;

            arg_index++;
            fip = GetField(tiff, argv[arg_index]);

            if (!fip)
                return -3;

            if (fip->field_type != TIFF_ASCII)
            {
                fprintf( stderr,
                         "Only ASCII tags can be set from file. "
                         "%s is not ASCII tag.\n", fip->field_name );
                return -5;
            }

            arg_index++;
            fp = fopen( argv[arg_index], "rt" );
            if( fp == NULL )
            {
                perror( argv[arg_index] );
                continue;
            }

            text = (char *) malloc(1000000);
            len = fread( text, 1, 999999, fp );
            text[len] = '\0';

            fclose( fp );

            if( TIFFSetField( tiff, fip->field_tag, text ) != 1 )
            {
                fprintf( stderr, "Failed to set %s from file %s\n", 
                         fip->field_name, argv[arg_index] );
            }

            free( text );
            arg_index++;
        }
        else
        {
            fprintf( stderr, "Unrecognised option: %s\n",
                     argv[arg_index] );
            usage();
        }
    }

    TIFFRewriteDirectory(tiff);
    TIFFClose(tiff);
    return (0);
}

