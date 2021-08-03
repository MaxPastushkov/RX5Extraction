/*
 * extractor.c
 * Created on: Aug 1, 2021
 * By: Michael Pastushkov, michael@pastushkov.com
 * By: Max Pastushkov, max@pastushkov.com
 *
 *      This program extracts raw PCM samples from Yamaha RX5 ROM image
 *      and converts them to WAV format by adding a header
 *      It is compliant with ancient ANSI-C, and should compile and run
 *      on any platform
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <math.h>

#define MAX_SAMPLES 128 /* Maximum number of samples in input file */

/* The following PCM parameters are fixed for RX5 */
#define NUM_CHANNELS 1
#define BPS 12
#define SAMPLE_RATE 11025

struct FILE_HEADER {
    uint32_t nulls; /* unknown and unused */
    uint8_t unknown; /* unknown and unused */
    uint8_t samples; /* number of samples */
} file_header;
#define FILE_HEADER_SIZE 6 /* to avoid platform-dependent alignment */

struct SAMPLE_HEADER {
    uint16_t unknown1; /* unknown and unused */
    uint8_t flags; /* 0 - 8 bit, 1 - 12 bit ??? - unreliable */
    uint8_t unknown2; /* unknown, AND 01 is loop on??? what is AND $40? ??? */
    uint16_t offset; /* offset of the PCM raw data of the sample */
    uint16_t unknown3; /* unknown and unused */
    uint32_t end_offset; /* end offset ??? */
    uint16_t loop_end; /* end of the loop */
    char unknown4[12]; /* unknown and unused */
    char name[6]; /* name of the samples */
} sample_header;
#define SAMPLE_HEADER_SIZE 32 /* to avoid platform-dependent alignment */

struct WAV_HEADER {
    char chunk_id[4];           /* "RIFF" */
    uint32_t chunk_size;        /* Overall file size */
    char format[4];             /* "WAVE" */
    char subchunk1_id[4];       /* "fmt " */
    uint32_t subchunk1_size;    /* 16 */
    uint16_t audio_format;      /* 1 is PCM */
    uint16_t num_channels;      /* Number of channels */
    uint32_t sample_rate;       /* Sample rate */
    uint32_t byte_rate;         /* (sampleRate * bitsPerSample * numChannels) / 8 */
    uint16_t block_align;       /* (bitsPerSample * channels) / 8 */
    uint16_t bits_per_sample;   /* Bits per sample */
    char subchunk2_id[4];       /* "data" */
    uint32_t subchunk2_size;    /* Size of raw audio */
} wav_header;

FILE *in, *out; /* streams */
int sample; /* sample counter */
long prev_pos; /* storage */

/* Swap 32 bits */
uint32_t bit_swap32(uint32_t s) {
    uint32_t d =
        ((s & 0xff) << 24) +
        ((s & 0xff00) << 8) +
        ((s & 0xff0000) >> 8) +
        ((s & 0xff000000) >> 24)
        ;
    return d;
}

/* For some reason C does not provide trimming of strings */
void trim(char *s) {
    while (*s && *s != ' ')
        s++;
    *s = 0;
}

/* Initialize WAV header */
void init_wav_header(size_t pcm_size) {
    memset(&wav_header, 0, sizeof(struct WAV_HEADER));
    memcpy(wav_header.chunk_id, "RIFF", 4);
    wav_header.chunk_size = pcm_size + sizeof(struct WAV_HEADER);
    memcpy(wav_header.format, "WAVE", 4);
    memcpy(wav_header.subchunk1_id, "fmt ", 4);
    wav_header.subchunk1_size = 16;
    wav_header.audio_format = 1;
    wav_header.num_channels = NUM_CHANNELS;
    wav_header.sample_rate = BPS;
    wav_header.byte_rate = ceil((float)(SAMPLE_RATE * BPS * NUM_CHANNELS)/8);
    wav_header.block_align = ceil((float)(BPS * NUM_CHANNELS)/8);
    wav_header.bits_per_sample = BPS;
    memcpy(wav_header.subchunk2_id, "data", 4);
    wav_header.subchunk2_size = pcm_size;
}

/* Open input and output files, manage names */
int process_sample() {

    char sname[7]; /* sorry for using confusing numbers! but they are fixed */
    char out_path[11];
    char *depth_name;
    uint32_t full_offset, full_length;
    
    /* Reading sample header */
    if (fread(&sample_header, SAMPLE_HEADER_SIZE, 1, in) != 1) {
        fprintf(stderr, "Cannot read sample header. Wrong file format?\n");
        return -1;
    }
    
    /* Remember curent position after reading the header */
    prev_pos = ftell(in);

    /* Get the name right (with 0 trailing, guaranteed) */
    memcpy(sname, sample_header.name, 6);
    sname[6] = 0;
    trim(sname);
    printf("Processing sample %i: ... ", sample+1);
    
    /* Destination file name */
    strcpy(out_path, sname);
    strcat(out_path, ".wav");
    
    /* Swapping bytes and conversions */
    full_offset = (sample_header.offset & 0x0FFF) << 8;
    
    full_length = bit_swap32(sample_header.end_offset);
    full_length = ((full_length & 0x0FFFFF00) >> 8) - full_offset;
    
    /* Open output file */
    if (!(out = fopen(out_path, "wb"))) {
        perror("Unable to open output file");
        return -1;
    }

    /* Reading & writing the sample */
    {
        char buf[full_length];
        memset(buf, 0, full_length);

        /* Read PCM data */
        fseek(in, full_offset, SEEK_SET);
        if (fread(buf, full_length, 1, in) != 1) {
            perror("Something went wrong, can't read input file");
            return -1;
        }
        
        /* Prepare and write VAW header */
        init_wav_header(full_length);
        if (fwrite(&wav_header, sizeof(struct WAV_HEADER), 1, out) != 1) {
            perror("Something went wrong, can't write WAV header");
            return -1;
        }

        /* Write PCM data */
        if (fwrite(buf, full_length, 1, out) != 1) {
            perror("Something went wrong, can't write PCM data");
            return -1;
        }
    }

    /* Rewind the input to the next header  */
    fseek(in, prev_pos, SEEK_SET);

    /* It's done! */
    fclose(out);
    printf("%s done\n", out_path);
    
    return 0;

}

int main(int argc, char *argv[]) {
    
	if (argc < 2) {
		fprintf(stderr, "Usage: %s input_file [output_file.bin]\n", argv[0]);
		return -1;
	}
  
    /* Open input file */
    if (!(in = fopen(argv[1], "rb"))) {
        perror("Unable to open input file");
        return -1;
    }
    
    /* Read file header */
    printf("Reading header ...\n");
    if (fread(&file_header, FILE_HEADER_SIZE, 1, in) != 1) {
        fprintf(stderr, "Cannot read input file header. Wrong file format?\n");
        return -1;
    }
    
    if (file_header.samples <=0 || file_header.samples > MAX_SAMPLES) {
        fprintf(stderr, "Invalid number of PCM samples. Wrong file format?\n");
        return -1;
    }
    
    printf("Samples detected: %i\n", file_header.samples);
        
    /* Main cycle processing samples */
    for (sample = 0; sample < file_header.samples; sample++) {
        if (process_sample() < 0) {
            break;
        }
    }
    
    printf("Samples extracted: %i\n", sample);

    fclose(in);
    fclose(out);

	return 0;
}
