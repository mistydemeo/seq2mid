#include <stdio.h>

#define MAX_TEMPO       256
#define MAX_GATE_QUEUE  200
#define MAX_TRACK_BUFFER_SIZE   1000000
#define VER_NAME        "1.0rc1"

//////////////////////////////////////////////////////////////////////////////

typedef struct
{
   unsigned short resolution;        // resolution = 480 counts / beat
   unsigned short num_tempo_events;  // number of tempo events
   unsigned short data_offset;       // playing data track offset address
   unsigned short tempo_loop_offset; // offset of 1st tempo event in loop
} header_struct;

typedef struct
{
   unsigned long clock; // clock time to execute note off
   unsigned char ctlcmd; // saves time recalculating it
   unsigned char keynum; // key number of course
   unsigned char byte3;
} gatequeue_struct;

//////////////////////////////////////////////////////////////////////////////

extern header_struct header;
extern gatequeue_struct gatequeue[MAX_GATE_QUEUE]; // I hope that's enough

extern unsigned long tempo_step_time[MAX_TEMPO]; // step time
extern unsigned long tempo_mspb[MAX_TEMPO];      // micro sec par(per?) beat

extern unsigned char track_buffer[MAX_TRACK_BUFFER_SIZE];

extern unsigned long clock;
extern unsigned long delta;
extern unsigned long gate;

extern unsigned long countB0; // Control Change
extern unsigned long countC0; // Program Change
extern unsigned long countNoteOn;
extern unsigned long countNoteOnbit1;
extern unsigned long countQueue;
extern unsigned long countNoteOff;
extern unsigned long count81; // Reference
extern unsigned long count82; // Loop
extern unsigned long count88; // EG 0x200
extern unsigned long count89; // EG 0x800
extern unsigned long count8A; // EG 0x1000
extern unsigned long count8B; // EG 0x2000
extern unsigned long count8C; // ED 0x100
extern unsigned long count8D; // EG 0x200
extern unsigned long count8E; // EG 0x800
extern unsigned long count8F; // EG 0x1000
extern unsigned long countA0; // Polyphonic Pressure
extern unsigned long countD0; // Channel Pressure
extern unsigned long countE0; // Pitch Bend

//////////////////////////////////////////////////////////////////////////////
int DecodeCnv(FILE *fp, char *output_filename);
