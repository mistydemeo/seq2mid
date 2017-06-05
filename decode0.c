#include <stdio.h>
#include "decode.h"

//#define DEBUG 1
#undef DEBUG

//////////////////////////////////////////////////////////////////////////////

header_struct header;
gatequeue_struct gatequeue[MAX_GATE_QUEUE]; // I hope that's enough

unsigned long tempo_step_time[MAX_TEMPO]; // step time
unsigned long tempo_mspb[MAX_TEMPO];      // micro sec par(per?) beat

unsigned char track_buffer[MAX_TRACK_BUFFER_SIZE];

#ifdef DEBUG
unsigned char last_cmd=0;
unsigned long real_clock=0;
#endif
unsigned long clock=0;
unsigned long delta=0;
unsigned long gate=0;

unsigned long countB0=0; // Control Change
unsigned long countC0=0; // Program Change
unsigned long countNoteOn=0;
unsigned long countNoteOnbit1=0;
unsigned long countQueue=0;
unsigned long countNoteOff=0;
unsigned long count81=0; // Reference
unsigned long count82=0; // Loop
unsigned long count88=0; // EG 0x200
unsigned long count89=0; // EG 0x800
unsigned long count8A=0; // EG 0x1000
unsigned long count8B=0; // EG 0x2000
unsigned long count8C=0; // ED 0x100
unsigned long count8D=0; // EG 0x200
unsigned long count8E=0; // EG 0x800
unsigned long count8F=0; // EG 0x1000
unsigned long countA0=0; // Polyphonic Pressure
unsigned long countD0=0; // Channel Pressure
unsigned long countE0=0; // Pitch Bend

unsigned char loop_type=0;
unsigned char disp_stats=0;

//////////////////////////////////////////////////////////////////////////////

int DecodeCnv(FILE *fp, char *output_filename);
unsigned long DecodeTrack(FILE *fp, unsigned char track_num, unsigned char *buffer);
char WriteDelta(unsigned char *track_buffer);
unsigned long CheckGateQueue(unsigned char *track_buffer);
unsigned long CheckGateQueue2(unsigned char *track_buffer, unsigned long cmd_delta);
unsigned long CheckTempoQueue(unsigned char *track_buffer);

//////////////////////////////////////////////////////////////////////////////

int DecodeCnv(FILE *fp, char *output_filename)
{
   int i=0;
   int done=0;
   unsigned long temp_length;
   FILE *output_fp;
   unsigned long fp_orig=0;

   clock=0;
   delta=0;
   gate=0;

   countB0=0; // Control Change
   countC0=0; // Program Change
   countNoteOn=0;
   countNoteOnbit1=0;
   countNoteOff=0;
   count81=0; // Reference
   count82=0; // Loop
   count88=0; // EG 0x200
   count89=0; // EG 0x800
   count8A=0; // EG 0x1000
   count8B=0; // EG 0x2000
   count8C=0; // ED 0x100
   count8D=0; // EG 0x200
   count8E=0; // EG 0x800
   count8F=0; // EG 0x1000
   countA0=0; // Polyphonic Pressure
   countD0=0; // Channel Pressure
   countE0=0; // Pitch Bend

   // zero out gate queues
   for (i = 0; i < MAX_GATE_QUEUE; i++)
   {
      gatequeue[i].clock = 0xFFFFFFFF;
      gatequeue[i].ctlcmd = 0xFF; 
      gatequeue[i].keynum = 0xFF; 
      gatequeue[i].byte3 = 0xFF;
   }

   printf("Writing %s...\n", output_filename);

   fp_orig = ftell(fp);

   fread(&header.resolution, 2, 1, fp);
   fread(&header.num_tempo_events, 2, 1, fp);
   fread(&header.data_offset, 2, 1, fp);
   fread(&header.tempo_loop_offset, 2, 1, fp);

   // byteswap
   header.resolution = (header.resolution << 8) +
                       ((header.resolution & 0xFF00) >> 8);
   header.num_tempo_events = (header.num_tempo_events << 8) +
                             ((header.num_tempo_events & 0xFF00) >> 8);
   header.data_offset = (header.data_offset << 8) +
                        ((header.data_offset & 0xFF00) >> 8);
   header.tempo_loop_offset = (header.tempo_loop_offset << 8) +
                              ((header.tempo_loop_offset & 0xFF00) >> 8);

   for (i = 0; i < header.num_tempo_events; i++)
   {
      fread(tempo_step_time + i, 4, 1, fp);
      fread(tempo_mspb + i, 4, 1, fp);

      tempo_step_time[i] = (tempo_step_time[i] << 24) +
                           ((tempo_step_time[i] & 0x0000FF00) << 8) +
                           ((tempo_step_time[i] & 0x00FF0000) >> 8) +
                           (tempo_step_time[i] >> 24);

      tempo_mspb[i] = (tempo_mspb[i] << 24) +
                      ((tempo_mspb[i] & 0x0000FF00) << 8) +
                      ((tempo_mspb[i] & 0x00FF0000) >> 8) +
                      (tempo_mspb[i] >> 24);

   }


   // Go to start of track's data
   fseek(fp, fp_orig + header.data_offset, SEEK_SET);

   // start generating midi file

   output_fp = fopen(output_filename, "wb");

   if (output_fp == NULL)
   {
      printf("unable to open file for writing\n");
      fclose (fp);
      exit (1);
   }

   // Write MThd Chunk

   fputc('M', output_fp);
   fputc('T', output_fp);
   fputc('h', output_fp);
   fputc('d', output_fp);

   fputc(0x00, output_fp);
   fputc(0x00, output_fp);  // number of bytes in chunk
   fputc(0x00, output_fp);
   fputc(0x06, output_fp);

   fputc(0x00, output_fp);  // midi format #0
   fputc(0x00, output_fp);

   fputc(0x00, output_fp);  // midi format #0 only has 1 track
   fputc(0x01, output_fp);     

   header.resolution = (header.resolution << 8) +
                       ((header.resolution & 0xFF00) >> 8);

   fwrite(&header.resolution, 2, 1, output_fp);

   // Write MTrk Chunk

   fputc('M', output_fp);
   fputc('T', output_fp);
   fputc('r', output_fp);
   fputc('k', output_fp);

   // convert compressed data here

   temp_length = DecodeTrack(fp, 0, track_buffer);

   // write size of MTrk chunk here

   fputc((temp_length + 9 + 68 + 4) << 24, output_fp);
   fputc(((temp_length + 9 + 68 + 4) & 0x00FF0000) << 16, output_fp);
   fputc(((temp_length + 9 + 68 + 4) & 0x0000FF00) >> 8, output_fp);
   fputc((temp_length + 9 + 68 + 4) & 0x000000FF, output_fp);

   // Write SMPTE meta event here (FF 54 05 ?? ?? ?? ?? ??)

   // temporary

   fputc(0x00, output_fp);
   fputc(0xFF, output_fp);
   fputc(0x54, output_fp);
   fputc(0x05, output_fp);
   fputc(0x60, output_fp);
   fputc(0x00, output_fp);
   fputc(0x00, output_fp);
   fputc(0x00, output_fp);
   fputc(0x00, output_fp);

   // Let's ID this midi so people know it was created with Seq2mid

   fputc(0x00, output_fp);
   fputc(0xFF, output_fp);
   fputc(0x01, output_fp);
   fputc(0x40, output_fp);
   // 4 + 64 characters
   fprintf(output_fp, "Created using Seq2mid(SATURN VERSION) v%s by Cyber Warrior X", VER_NAME); 
          
   // write converted midi data here

   fwrite((void *)track_buffer, 1, temp_length, output_fp);

   // End of Track

   fputc(0x00, output_fp);
   fputc(0xFF, output_fp);
   fputc(0x2F, output_fp);
   fputc(0x00, output_fp); 

   fclose (output_fp);

   printf("done\n");

   if (disp_stats)
   {
      printf("\nStatistics(number of calls)\n");
      printf("Number of Tempo Events: %d\n", header.num_tempo_events);
      printf("Control Change: %d\n", countB0);
      printf("Program Change: %d\n", countC0);
      printf("Note On: %d\n", countNoteOn);
      printf("Note On - Bit 1: %d\n", countNoteOnbit1);
      printf("Queues: %d\n", countQueue);
      printf("Note Off: %d\n", countNoteOff);
      printf("Reference: %d\n", count81);
      printf("Loop: %d\n", count82);
      printf("Extend Gate(0x200): %d\n", count88);
      printf("Extend Gate(0x800): %d\n", count89);
      printf("Extend Gate(0x1000): %d\n", count8A);
      printf("Extend Gate(0x2000): %d\n", count8B);
      printf("Extend Delta(0x100): %d\n", count8C);
      printf("Extend Delta(0x200): %d\n", count8D);
      printf("Extend Delta(0x800): %d\n", count8E);
      printf("Extend Delta(0x1000): %d\n", count8F);
      printf("Polyphonic Pressure: %d\n", countA0);
      printf("Channel Pressure: %d\n", countD0);
      printf("Pitch Bend: %d\n\n", countE0);
   }

   // adjust file offset so it's 16-bit aligned again
   if (ftell(fp) % 2 == 1)
      fseek(fp, 1, SEEK_CUR);

}

//////////////////////////////////////////////////////////////////////////////

unsigned long DecodeTrack(FILE *fp, unsigned char track_num, unsigned char *buffer)
{
   int done=0;
   unsigned long counter=0;
   unsigned char ctl_byte;
   unsigned long command_counter=0;
   int ref_enabled=0;
   unsigned char ref_counter=0;
   unsigned long old_fp=0;
   unsigned long orig_fp=0;
   int i;

   orig_fp = ftell(fp);

   while (!done)
   {
      fread((void *)&ctl_byte, 1, 1, fp);

      // check to see if a tempo can be written
      counter += CheckTempoQueue(buffer + counter);

      if (ctl_byte >= 0xB0 && ctl_byte <= 0xBF)
      {
         //CTRL Change

         unsigned char byte2, byte3, byte4;

         fread((void *)&byte2, 1, 1, fp);
         fread((void *)&byte3, 1, 1, fp);
         fread((void *)&byte4, 1, 1, fp); // grab the delta

         // check to see if note off can be written, update delta and clock
         counter += CheckGateQueue2(buffer + counter, byte4);

         counter += WriteDelta(buffer + counter);

         buffer[counter] = ctl_byte; 
         counter += 1;

         buffer[counter] = byte2;
         counter += 1;

         buffer[counter] = byte3;
         counter += 1;

         if (ref_enabled == 1)
         {
            ref_counter--;
         }

         countB0++; // Control Change count
      }
      else if (ctl_byte >= 0xC0 && ctl_byte <= 0xCF)
      {
         // Program Change

         unsigned char byte2, byte3;

         fread((void *)&byte2, 1, 1, fp);
         fread((void *)&byte3, 1, 1, fp); // grab the delta

         counter += CheckGateQueue2(buffer + counter, byte3);
         counter += WriteDelta(buffer + counter);

         buffer[counter] = ctl_byte;
         counter += 1;

         buffer[counter] = byte2;
         counter += 1;

         if (ref_enabled == 1)
         {
            ref_counter--;
         }

         countC0++; // Program Change count
      }
      else if (ctl_byte == 0xFF)
      {
         // Meta event(not used)

         if (ref_enabled == 1)
         {
            ref_counter--;
         }

         // skip the next 5 bytes
         fseek(fp, 5, SEEK_CUR);
      }
      else if (ctl_byte <= 0x7F)
      {
         // note on

         unsigned char key_num, velocity, byte4, byte5;
         unsigned short temp_delta=0;

         fread((void *)&key_num, 1, 1, fp);
         fread((void *)&velocity, 1, 1, fp); 
         fread((void *)&byte4, 1, 1, fp); // grab the gate (not accurate)
         fread((void *)&byte5, 1, 1, fp); // grab the delta

         temp_delta = byte5;

         if (ctl_byte & 0x20) temp_delta += 256;

         if (ctl_byte & 0x40) gate += 256;

         if (ctl_byte & 0x10)
         {
#ifdef DEBUG
            printf("Note On uses an unknown status bit 1(0x10)\n");
#endif

            countNoteOnbit1++;
         }

         gate += byte4;
#ifdef DEBUG
         if((ctl_byte & 0x0f) == 0x5)
         printf("note on: clock = %d ctl_byte = %x key_num = %x velocity = %x delta = %d gate = %d\n", clock, ctl_byte, key_num, velocity, delta, gate);
#endif

         // check to see if note off can be written, update delta and clock
         counter += CheckGateQueue2(buffer + counter, temp_delta);

         counter += WriteDelta(buffer + counter);

         buffer[counter] = (ctl_byte & 0x0F) + 0x90; // track
         counter += 1;

         buffer[counter] = key_num;         // key #
         counter += 1;

         buffer[counter] = velocity;         // velocity(I think)
         counter += 1;

         // queue up note off

         for (i = 0; i < MAX_GATE_QUEUE; i++)
         {
            if (gatequeue[i].ctlcmd == 0xFF)
            {
               gatequeue[i].clock = clock + gate;
               gatequeue[i].ctlcmd = (ctl_byte & 0x0F) + 0x80;
               gatequeue[i].keynum = key_num;
               gatequeue[i].byte3 = 0x40;

               gate = 0;
               countQueue++;

               break;
            }
         }
#ifdef DEBUG
         if((ctl_byte & 0x0f) == 0x5)
         printf ("Gate Queue: current clock = %d clock = %d ctlcmd = %x keynum = %x byte3 = %x\n", clock, gatequeue[i].clock, gatequeue[i].ctlcmd, gatequeue[i].keynum, gatequeue[i].byte3);
#endif

         if (ref_enabled == 1)
         {
            ref_counter--;
         }

         countNoteOn++; // count
      }
      else if (ctl_byte == 0x81)
      {
         unsigned short temp;

         // Reference - repeat event #?? for x amount of times

         // let the program know we want to reference an old pointer
         ref_enabled = 1;

         // read in the referenced pointer
         fread((void *)&temp, 2, 1, fp);
         fread((void *)&ref_counter, 1, 1, fp);

         // byteswap pointer
         temp = ((temp & 0x00FF) << 8) + (temp >> 8);

         // save old pointer
         old_fp = ftell(fp);

         fseek(fp, orig_fp + temp, SEEK_SET);

         count81++; // Reference count
      }
      else if (ctl_byte == 0x82)
      {
         // Loop (should only be used in first track)

         unsigned char byte2;

         fread((void *)&byte2, 1, 1, fp); // grab the delta

         // check to see if note off can be written, update delta and clock
         counter += CheckGateQueue2(buffer + counter, byte2);

         switch (loop_type)
         {
            case 0:
            {
               // Control Code 31 Method

               // write delta
               counter += WriteDelta(buffer + counter);

               buffer[counter] = 0xB0; // Control Code
               counter += 1;

               buffer[counter] = 0x1F; // 31
               counter += 1;

               buffer[counter] = 0x00; // Unknown
               counter += 1;
               break;
            }
            case 1:
            {
               // LoopStart/LoopEnd Method

               if (count82 == 0)
               {
                  // write delta
                  counter += WriteDelta(buffer + counter);

                  buffer[counter] = 0xFF; // Meta code
                  counter += 1;

                  buffer[counter] = 0x06; // type
                  counter += 1;                  

                  buffer[counter] = 0x09; // size
                  counter += 1;                  

                  buffer[counter] = 'l'; 
                  counter += 1;                  
                  buffer[counter] = 'o'; 
                  counter += 1;                  
                  buffer[counter] = 'o';
                  counter += 1;
                  buffer[counter] = 'p'; 
                  counter += 1;                  
                  buffer[counter] = 'S'; 
                  counter += 1;                  
                  buffer[counter] = 't'; 
                  counter += 1;                  
                  buffer[counter] = 'a'; 
                  counter += 1;                  
                  buffer[counter] = 'r'; 
                  counter += 1;                  
                  buffer[counter] = 't'; 
                  counter += 1;                  
               }
               else
               {
                  unsigned char num_queues_left=0;

                  // Check to see if there's any gate queues left

                  for (i=0; i < MAX_GATE_QUEUE; i++)
                  {
                     if (gatequeue[i].clock != 0xFFFFFFFF && gatequeue[i].ctlcmd != 0xFF)
                     {
                        num_queues_left++;
                     }
                  }

                  if (num_queues_left != 0)
                  {
#ifdef DEBUG
                     printf("Warning: Not all notes have been Note Off'ed by the time Loop end is called\n");
                     printf("LoopEnd clock: %d delta = %d\n", clock, delta);
#endif
                     // Temporary(I hope)
                     for (i = 0; i < MAX_GATE_QUEUE; i++)
                     {
                        if (gatequeue[i].clock != 0xFFFFFFFF && gatequeue[i].ctlcmd != 0xFF)
                        {
#ifdef DEBUG
                           printf("Unfinished Queue #%d: clock = %d ctlcmd = %02x keynum = %02x\n", i, gatequeue[i].clock, gatequeue[i].ctlcmd, gatequeue[i].keynum);
#endif

                           counter += WriteDelta(buffer + counter);

                           // Note off command for channel
                           buffer[counter] = gatequeue[i].ctlcmd; 
                           counter += 1;
 
                           // Key number
                           buffer[counter] = gatequeue[i].keynum; 
                           counter += 1;

                           // Unknown byte 3
                           buffer[counter] = gatequeue[i].byte3; 
                           counter += 1;

                           // clear entry
                           gatequeue[i].clock = 0xFFFFFFFF; 
                           gatequeue[i].ctlcmd = 0xFF; 
                           gatequeue[i].keynum = 0xFF; 
                           gatequeue[i].byte3 = 0xFF;

                           countNoteOff++;
                        }
                     }

                  }

                  // write delta
                  counter += WriteDelta(buffer + counter);

                  buffer[counter] = 0xFF; // Meta code
                  counter += 1;

                  buffer[counter] = 0x06; // type
                  counter += 1;                  

                  buffer[counter] = 0x07; // size
                  counter += 1;                  

                  buffer[counter] = 'l'; 
                  counter += 1;                  
                  buffer[counter] = 'o'; 
                  counter += 1;                  
                  buffer[counter] = 'o';
                  counter += 1;
                  buffer[counter] = 'p'; 
                  counter += 1;                  
                  buffer[counter] = 'E'; 
                  counter += 1;                  
                  buffer[counter] = 'n'; 
                  counter += 1;                  
                  buffer[counter] = 'd'; 
                  counter += 1;                  
               }

               break;
            }
            default: break;
         }

         if (ref_enabled == 1)
         {
            ref_counter--;
         }

         count82++; // Loop count
      }
      else if (ctl_byte == 0x83)
      {
         // song end
         unsigned char num_queues_left=0;

         // Finish up any pending notes. 

         for (i=0; i < MAX_GATE_QUEUE; i++)
         {
            if (gatequeue[i].clock != 0xFFFFFFFF && gatequeue[i].ctlcmd != 0xFF)
            {
               num_queues_left++;
            }
         }

         while (num_queues_left > 0)
         {
            delta++;
            clock++;
            counter += CheckGateQueue(buffer + counter);

            num_queues_left = 0;

            for (i=0; i < MAX_GATE_QUEUE; i++)
            {
               if (gatequeue[i].clock != 0xFFFFFFFF && gatequeue[i].keynum != 0xFF)
               {
                  num_queues_left++;
               }
            }
         }

         done = 1;
      }
      else if (ctl_byte == 0x88)
      {
         // extend gate time - 0x200

         gate += 0x200;

         count88++; // EG 0x200 count
      }
      else if (ctl_byte == 0x89)
      {
         // extend gate time - 0x800

         gate += 0x800;

         count89++; // EG 0x800 count
      }
      else if (ctl_byte == 0x8A)
      {
         // extend gate time - 0x1000

         gate += 0x1000;

         count8A++; // EG 0x1000 count
      }
      else if (ctl_byte == 0x8B)
      {
         // extend gate time - 0x2000 (MKR uses it)

         gate += 0x2000;

         count8B++; // EG 0x2000 count
      }
      else if (ctl_byte == 0x8C)
      {
         // extend delta time - 0x100

         // check to see if note off can be written, update delta and clock
         counter += CheckGateQueue2(buffer + counter, 0x100);

         count8C++; // ED 0x100 count
      }
      else if (ctl_byte == 0x8D)
      {
         // extend delta time - 0x200

         // check to see if note off can be written, update delta and clock
         counter += CheckGateQueue2(buffer + counter, 0x200);

         count8D++; // EG 0x200 count
      }
      else if (ctl_byte == 0x8E)
      {
         // extend delta time - 0x800

         // check to see if note off can be written, update delta and clock
         counter += CheckGateQueue2(buffer + counter, 0x800);

         count8E++; // EG 0x800 count
      }
      else if (ctl_byte == 0x8F)
      {
         // extend delta time - 0x1000

         // check to see if note off can be written, update delta and clock
         counter += CheckGateQueue2(buffer + counter, 0x1000);

         count8F++; // EG 0x1000 count
      }
      else if (ctl_byte >= 0xA0 && ctl_byte <= 0xAF)
      {
         // Polyphonic Pressure

         unsigned char byte2, byte3, byte4;

         fread((void *)&byte2, 1, 1, fp); // key
         fread((void *)&byte3, 1, 1, fp); // value
         fread((void *)&byte4, 1, 1, fp); // delta

         // check to see if note off can be written, update delta and clock
         counter += CheckGateQueue2(buffer + counter, byte4);

         counter += WriteDelta(buffer + counter);

         buffer[counter] = ctl_byte; // Control Code
         counter += 1;

         buffer[counter] = byte2; // key
         counter += 1;

         buffer[counter] = byte3; // value
         counter += 1;

         if (ref_enabled == 1)
         {
            ref_counter--;
         }

         countA0++; // Polyphonic Pressure count
      }
      else if (ctl_byte >= 0xD0 && ctl_byte <= 0xDF)
      {
         // Channel Pressure

         unsigned char byte2, byte3;

         fread((void *)&byte2, 1, 1, fp); // value
         fread((void *)&byte3, 1, 1, fp); // delta

         // check to see if note off can be written, update delta and clock
         counter += CheckGateQueue2(buffer + counter, byte3);

         counter += WriteDelta(buffer + counter);

         buffer[counter] = ctl_byte; // Control Code
         counter += 1;

         buffer[counter] = byte2; // value
         counter += 1;

         if (ref_enabled == 1)
         {
            ref_counter--;
         }

         countD0++; // Channel Pressure count
      }
      else if (ctl_byte >= 0xE0 && ctl_byte <= 0xEF)
      {
         // Pitch Bend/Wheel
         // (Langrisser DE, Lunar, MKR, Shining Force III uses it)

         unsigned char byte2, byte3;

         fread((void *)&byte2, 1, 1, fp); // pitch
         fread((void *)&byte3, 1, 1, fp); // delta

         // check to see if note off can be written, update delta and clock
         counter += CheckGateQueue2(buffer + counter, byte3);

         counter += WriteDelta(buffer + counter);

         // Pitch Bend code

         buffer[counter] = ctl_byte; // Control Code
         counter += 1;

         buffer[counter] = 0x00; // pitch value #1
         counter += 1;

         buffer[counter] = byte2; // pitch value #2
         counter += 1;


         if (ref_enabled == 1)
         {
            ref_counter--;
         }

         countE0++; // Pitch Bend count
      }
      else
      {
         printf("unknown code: address %x, value %x\n", ftell(fp) - 1, ctl_byte);

         done = 1;

         if (ref_enabled == 1)
         {
            ref_counter--;
         }
      }

      if (ref_enabled == 1 && ref_counter == 0)
      {
         // replace the old counter

         fseek(fp, old_fp, SEEK_SET);
         ref_enabled = 0;
      }

#ifdef DEBUG
      printf("command counter = %d clock = %d file address = %x buffer address = %x\n", command_counter, clock, ftell(fp), counter);
#endif

      command_counter++;

      if (counter >= MAX_TRACK_BUFFER_SIZE)
      {
         done = 1;
      }
   }

   return counter;
}

//////////////////////////////////////////////////////////////////////////////

char WriteDelta(unsigned char *track_buffer)
{
   unsigned long buffer;
   char counter=0;
   unsigned long value;

   value = delta;

   buffer = value & 0x7F;

   while ( (value >>= 7) )
   {
      buffer <<= 8;
      buffer |= ((value & 0x7F) | 0x80);
   }

   while (1)
   {
      track_buffer[counter] = buffer;
      counter++;

      if (buffer & 0x80)
          buffer >>= 8;
      else
         break;
   }

#ifdef DEBUG
   real_clock += delta;
#endif

   delta = 0;

   return counter;
}

//////////////////////////////////////////////////////////////////////////////

unsigned long CheckGateQueue(unsigned char *track_buffer)
{
   // Check to see if a note off is queued, if so, write it to buffer, delete
   // it off list, and return the offset.

   unsigned long counter=0;
   int i;

   for (i = 0; i < MAX_GATE_QUEUE; i++)
   {
      if (gatequeue[i].clock <= clock && gatequeue[i].ctlcmd != 0xFF)
      {
#ifdef DEBUG
         if((gatequeue[i].ctlcmd & 0x0f) == 0x5)
         printf("Note Off written at clock %d. Should've been written at %d, Real clock = %d CheckGateQueue used\n", clock, gatequeue[i].clock, real_clock);
#endif

         // Don't forget to write the delta!
         counter += WriteDelta(track_buffer + counter);
        
         // Note off command for channel
         track_buffer[counter] = gatequeue[i].ctlcmd; 
         counter += 1;

         // Key number
         track_buffer[counter] = gatequeue[i].keynum; 
         counter += 1;

         // Unknown byte 3
         track_buffer[counter] = gatequeue[i].byte3; 
         counter += 1;

         // clear entry
         gatequeue[i].clock = 0xFFFFFFFF; 
         gatequeue[i].ctlcmd = 0xFF; 
         gatequeue[i].keynum = 0xFF; 
         gatequeue[i].byte3 = 0xFF;

         // zero out delta
         delta = 0;

         countNoteOff++;
      }
   }

   return counter;
}

//////////////////////////////////////////////////////////////////////////////

unsigned long CheckGateQueue2(unsigned char *track_buffer, unsigned long cmd_delta)
{
   // Check to see if a note off is queued, if so, write it to buffer, delete
   // it off list, and return the offset.

   unsigned long counter=0;
   int i, i2;
   unsigned short num_matches=0;
   unsigned long orig_clock=0;

   orig_clock = clock;

   for (i2 = 0; i2 < cmd_delta; i2++)
   {
      for (i = 0; i < MAX_GATE_QUEUE; i++)
      {      
         if (gatequeue[i].clock == (orig_clock + i2 + 1) && gatequeue[i].ctlcmd != 0xFF)
         {
#ifdef DEBUG
         if((gatequeue[i].ctlcmd & 0x0f) == 0x5)
            printf("Clock before Note Off %d. Should've been written at %d, cmd_delta = %d, clock set to: %d delta = %d Key num = %d Real clock = %d\n", clock, gatequeue[i].clock, cmd_delta, gatequeue[i].clock, delta + i2 + 1 - (clock - orig_clock), gatequeue[i].keynum, real_clock);
#endif

            delta += i2 + 1 - (clock - orig_clock);

            // Don't forget to write the delta!
            counter += WriteDelta(track_buffer + counter);
        
            // Note off command for channel
            track_buffer[counter] = gatequeue[i].ctlcmd; 
            counter += 1;

            // Key number
            track_buffer[counter] = gatequeue[i].keynum; 
            counter += 1;

            // Unknown byte 3
            track_buffer[counter] = gatequeue[i].byte3; 
            counter += 1;

            // adjust the clock accordingly
            clock = gatequeue[i].clock;

            // clear entry
            gatequeue[i].clock = 0xFFFFFFFF; 
            gatequeue[i].ctlcmd = 0xFF; 
            gatequeue[i].keynum = 0xFF; 
            gatequeue[i].byte3 = 0xFF;


            countNoteOff++;
            num_matches++;
         }
         else if (gatequeue[i].clock < (orig_clock + i2 + 1) && gatequeue[i].ctlcmd != 0xFF)
         {
#ifdef DEBUG
            printf("Error: Missed gate queue. clock = %d key_num = %d last_cmd = %02x\n", clock, gatequeue[i].keynum, last_cmd);
#endif
         }

      }
   }

   if (num_matches == 0)
   {
      clock += cmd_delta;
      delta += cmd_delta;
   }
   else
   {
      delta = cmd_delta - (clock - orig_clock);
      clock += delta;
   }

   return counter;
}

//////////////////////////////////////////////////////////////////////////////

unsigned long CheckTempoQueue(unsigned char *track_buffer)
{
   unsigned long tempo_clock=0;
   unsigned long counter=0;
   int i;

   for (i=0; i < header.num_tempo_events; i++)
   {
      if (tempo_mspb[i] != 0)
      {
         if (tempo_clock <= clock)
         {
            // Write Tempo Change here (FF 51 03 ?? ?? ??) - see mspb

#ifdef DEBUG
            printf("Tempo written at clock %d. Should've been written at %d\n", clock, tempo_clock);
#endif

            // Don't forget to write the delta!
            counter += WriteDelta(track_buffer + counter);

            // Meta Event
            track_buffer[counter] = 0xFF; 
            counter += 1;

            // Meta Event #0x51
            track_buffer[counter] = 0x51; 
            counter += 1;

            // Meta Event Size
            track_buffer[counter] = 0x03; 
            counter += 1;

            // Tempo Part 1
            track_buffer[counter] = (tempo_mspb[i] & 0xFF0000) >> 16; 
            counter += 1;

            // Tempo Part 2
            track_buffer[counter] = (tempo_mspb[i] & 0x00FF00) >> 8; 
            counter += 1;

            // Tempo Part 3
            track_buffer[counter] = tempo_mspb[i] & 0x0000FF; 
            counter += 1;

            // zero out delta and mspb
            delta = 0;
            tempo_mspb[i] = 0;
         }
      }

      tempo_clock += tempo_step_time[i];
   }

   return counter;
}

//////////////////////////////////////////////////////////////////////////////
