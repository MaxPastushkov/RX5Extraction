/*
 * ooo.c
 *
 *  Created on: Aug 1, 2021
 *      Author: max
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <math.h>

struct WAV_HEADER {
	char chunkID[4];		// "RIFF"
	uint32_t chunkSize;		// Overall file size
	char format[4];			// "WAVE"

	char subchunk1ID[4];	// "fmt "
	uint32_t subchunk1Size;	// 16
	uint16_t audioFormat;	// 1 is PCM
	uint16_t numChannels;	// Number of channels
	uint32_t sampleRate;	// Sample rate
	uint32_t byteRate;		// (sampleRate * bitsPerSample * numChannels) / 8
	uint16_t blockAlign;	// (bitsPerSample * channels) / 8
	uint16_t bitsPerSample;	// Bits per sample

	char subchunk2ID[4];	// "data"
	uint32_t subchunk2Size;	// Size of raw audio
} header;

int main(int argc, char *argv[]) {

	// Configuration
	int numChannels = 1;
	int bitsPerSample = 12;
	uint32_t sampleRate = 11025;

	FILE *inputFile, *outputFile;
	char outputPath[101], c;
	size_t inputSize;

	if (argc < 2) {
		fprintf(stderr, "Usage: %s input_file [output_file.wav]\n", argv[0]);
		return -1;
	}

	// Open files
	if (!(inputFile = fopen(argv[1], "rb"))) {
		perror("Unable to open input file");
		return -1;
	}

	if (argc > 2) {
		strcpy(outputPath, argv[2]);
	} else {
		strcpy(outputPath, argv[1]);
		strcat(outputPath, ".wav");
	}

	if (!(outputFile = fopen(outputPath, "wb"))) {
		perror("Unable to open output file");
		return -1;
	}

	// Get file size
	fseek(inputFile, 0, SEEK_END);
	inputSize = ftell(inputFile);
	rewind(inputFile);

	// Init header
	memcpy(header.chunkID, "RIFF", 4);
	header.chunkSize = inputSize + sizeof(struct WAV_HEADER);
	memcpy(header.format, "WAVE", 4);
	memcpy(header.subchunk1ID, "fmt ", 4);
	header.subchunk1Size = 16;
	header.audioFormat = 1;
	header.numChannels = numChannels;
	header.sampleRate = sampleRate;
	header.byteRate = ceil((float)(sampleRate * bitsPerSample * numChannels)/8);
	header.blockAlign = ceil((float)(bitsPerSample * numChannels)/8);
	header.bitsPerSample = bitsPerSample;
	memcpy(header.subchunk2ID, "data", 4);
	header.subchunk2Size = inputSize;

	// Write everything to output file
	fwrite(&header, sizeof(header), 1, outputFile);
	c = fgetc(inputFile);
	while (ftell(inputFile) != inputSize) {
		fputc(c, outputFile);
		c = fgetc(inputFile);
	}
	fputc(c, outputFile);

	printf("Successfully created file %s\n", outputPath);

	fclose(inputFile);
	fclose(outputFile);
	return 0;
}
