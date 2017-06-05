#include <stdio.h>
#ifdef WIN32
# include <dir.h>
#else
# include "dir_emu.h"
#endif
#include "decode.h"

#define SLASH_STR "\\"
#define SLASH_CHAR '\\'

unsigned long seq_offsets[100];
unsigned char seq_counter=0;

void CreateFileName(char *orig_filename, unsigned char number);
void ProgramUsage();

//#define DEBUG 1
#undef DEBUG

char output_fn[30];

extern unsigned char loop_type;
extern unsigned char disp_stats;

unsigned char trans_buffer[1000000];

//////////////////////////////////////////////////////////////////////////////

int main(int argc, char *argv[])
{
   FILE *fp;
   FILE *output_fp;
   char *filename;
   unsigned short num_seqs=0;
   unsigned long seq_offset=0;
   unsigned long f_size;
   unsigned long orig_offset=0;
   int done=0;
   int i;

   if (argc < 2)
   {
      ProgramUsage();
      exit (1);
   }

   printf ("Seq2mid v%s SATURN VERSION - by Cyber Warrior X (c)2002\n", VER_NAME);

   for (i = 0; i < (argc - 1); i++)
   {
      if (strcmp(argv[i + 1], "-d") == 0)
      {
         disp_stats = 1;
      }
      else if (strcmp(argv[i + 1], "-l") == 0)
      {
         loop_type = atoi(argv[i + 2]);

         if (loop_type > 1)
         {
            printf("Invalid loop type\n");
            exit (1);
         }
      }
      else
         filename = argv[i + 1];
   }

   fp = fopen (filename, "rb");

   if (fp == NULL)
   {
      printf("unable to open file\n");
      exit (1);
   }

   // grab the file size
   fseek(fp, 0, SEEK_END);
   f_size = ftell(fp) - 1;
   fseek(fp, 0, SEEK_SET);

   while(ftell(fp) < (f_size - 1))
   {
      unsigned long search_counter=0;
      unsigned long num_bytes_read=0;

      fseek(fp, orig_offset, SEEK_SET);
      memset(trans_buffer, 0, 1000000);

      if ((ftell(fp) + 1000000) > f_size)
      {
         num_bytes_read = f_size - ftell(fp) + 1;
      }
      else
      {
         num_bytes_read = 1000000;
      }

      fread((void *)trans_buffer, 1, num_bytes_read, fp);
      orig_offset = ftell(fp);

      while (search_counter < 800000)
      {
         // byte swap it and copy over 
         num_seqs = (trans_buffer[search_counter] << 8) +
                    trans_buffer[search_counter + 1];

         seq_offset = (trans_buffer[search_counter + 2] << 24) +
                      (trans_buffer[search_counter + 3] << 16) +
                      (trans_buffer[search_counter + 4] << 8) +
                      trans_buffer[search_counter + 5];

         if (((num_seqs * 4) + 2) == seq_offset)
         {
            // passed first test
            unsigned short temp1=0;

#ifdef DEBUG
            printf("offset = %08x ", orig_offset - num_bytes_read + search_counter + seq_offset + 4);
#endif

            temp1 = (trans_buffer[search_counter + seq_offset + 4] << 8) +
                    trans_buffer[search_counter + seq_offset + 4 + 1];

#ifdef DEBUG
            printf("temp1 = %04x\n", temp1);
#endif

            if ((temp1 + orig_offset - num_bytes_read + (search_counter + seq_offset)) < f_size)
            {            
               // passed second test
               unsigned short temp2;

#ifdef DEBUG
               printf("offset = %08x ", orig_offset - num_bytes_read + search_counter + seq_offset + temp1);
#endif
               temp2 = (trans_buffer[search_counter + seq_offset + temp1] << 8) +
                        trans_buffer[search_counter + seq_offset + temp1 + 1];
#ifdef DEBUG
               printf("temp2 = %04x\n", temp2);
#endif

               // 0xB07B is a lang5 hack, 0xB000 is a Langrisser DE hack.
               if ((temp2 & 0xF0FF) == 0xB020 || (temp2 & 0xF0FF) == 0xB07B ||
                   (temp2 & 0xF0FF) == 0xB000)
               {
                  // passed third test, this is definitely a seq

                  seq_offsets[seq_counter] = (orig_offset - num_bytes_read) + (search_counter + seq_offset);
                  seq_counter++;

#ifdef DEBUG
                  printf("found match at : %08x temp1 = %x seq_offset = %x\n", seq_offsets[seq_counter - 1], temp1, seq_offset);
#endif

                  fseek(fp, (orig_offset - num_bytes_read) + search_counter + 6, SEEK_SET);

                  if (num_seqs > 1)
                  {
                     for (i = 0; i < (num_seqs - 1); i++)
                     {
                        seq_offsets[seq_counter] = (trans_buffer[search_counter + 6 + (i * 4)] << 24) +
                                                   (trans_buffer[search_counter + 6 + (i * 4) + 1] << 16) +
                                                   (trans_buffer[search_counter + 6 + (i * 4) + 2] << 8) +
                                                    trans_buffer[search_counter + 6 + (i * 4) + 3];
                        seq_offsets[seq_counter] += (orig_offset - num_bytes_read) + search_counter;
                        seq_counter++;

#ifdef DEBUG
                        printf("found match at : %08x\n", seq_offsets[seq_counter - 1]);
#endif
                     }
                  }

                  // generate midi's
                  for (i = 0; i < num_seqs; i++)
                  {
                     CreateFileName(filename, seq_counter - num_seqs + i);
                     fseek(fp, seq_offsets[seq_counter - num_seqs + i], SEEK_SET);
#ifdef DEBUG
                     printf("Decoding Seq at : %08x\n", ftell(fp));
#endif
                     DecodeCnv(fp, output_fn);
                  }

                  search_counter = ftell(fp) - (orig_offset - num_bytes_read);
               }
               else
               {
                  search_counter += 2;
               }
            }
            else
            {
               search_counter += 2;
            }
         }
         else
         {
            search_counter += 2;
         }
      }

      if(num_bytes_read == 1000000)
         orig_offset -= 200000;
   }

   fclose (fp);
}

//////////////////////////////////////////////////////////////////////////////

void _makepath (char *path, const char *drive, const char *dir,
		const char *fname, const char *ext)
{
    if (drive && *drive)
    {
	*path = *drive;
	*(path + 1) = ':';
	*(path + 2) = 0;
    }
    else
	*path = 0;
	
    if (dir && *dir)
    {
	strcat (path, dir);
        if (strlen (dir) != 1 || *dir != '\\')
            strcat (path, SLASH_STR);
    }
	
    strcat (path, fname);
    if (ext && *ext)
    {
        strcat (path, ".");
        strcat (path, ext);
    }
}

//////////////////////////////////////////////////////////////////////////////

void _splitpath (const char *path, char *drive, char *dir, char *fname,
		 char *ext)
{
    if (*path && *(path + 1) == ':')
    {
	*drive = toupper (*path);
	path += 2;
    }
    else
	*drive = 0;

    char *slash = strrchr (path, SLASH_CHAR);
    if (!slash)
	slash = strrchr (path, '/');
    char *dot = strrchr (path, '.');
    if (dot && slash && dot < slash)
	dot = NULL;

    if (!slash)
    {
        if (*drive)
            strcpy (dir, "\\");
        else
            strcpy (dir, "");
	strcpy (fname, path);
        if (dot)
        {
	    *(fname + (dot - path)) = 0;
	    strcpy (ext, dot + 1);
        }
	else
	    strcpy (ext, "");
    }
    else
    {
        if (*drive && *path != '\\')
        {
            strcpy (dir, "\\");
            strcat (dir, path);
            *(dir + (slash - path) + 1) = 0;
        }
        else
        {
            strcpy (dir, path);
            if (slash - path == 0)
                *(dir + 1) = 0;
            else
                *(dir + (slash - path)) = 0;
        }

	strcpy (fname, slash + 1);
        if (dot)
	{
	    *(fname + (dot - slash) - 1) = 0;
    	    strcpy (ext, dot + 1);
	}
	else
	    strcpy (ext, "");
    }
}

//////////////////////////////////////////////////////////////////////////////

void CreateFileName(char *orig_filename, unsigned char number)
{
   char drive[MAXDRIVE];
   char path[MAXDIR];
   char file[MAXFILE];
   char ext[MAXEXT];

   _splitpath(orig_filename, drive, path, file, ext);
   sprintf(file, "%s%02d.mid", file, number);
   _makepath(output_fn, "", "", file, "");
}

void ProgramUsage()
{
   printf ("Seq2mid v%s SATURN VERSION - by Cyber Warrior X (c)2002\n", VER_NAME);
   printf ("usage: seq2mid [switches] [filename].ext\n");
   printf ("switches:\n");
   printf ("-l x\tSelect loop type x, where x is one of the following:\n");
   printf ("    \t 0 - Use Control Code 31, the default method.\n");
   printf ("    \t 1 - Use loopStart/loopEnd, Winamp only supports this.\n");
   printf ("-d  \tDisplay Midi statistics\n");
//   printf ("-f x\tSpecify which midi format to use. Either 0(default) or 1\n");
}
