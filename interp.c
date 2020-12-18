/* ***********************************************
 *   Doubles the sample rate of a WAV file by using linear interpolation.
 *
 *   Creator: Matthieu 'Rubisetcie' Carteron
 *   Forked from: Simontime's interp repository
 * ***********************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "int24.h"

#define MIN(a, b) ((a)<(b)?(a):(b))
#define MAX(a, b) ((a)>(b)?(a):(b))

#define CHECK_MAGIC(a, b)   ((*(int *)(&(a))) == (*(int *)(&(b))))
#define SET_MAGIC(a, b)     ((*(int *)(&(a))) = (*(int *)(&(b))))

/* Definition of the WAV position peak */
typedef struct PeakPos
{
    float   value;
    int32_t position;
} PeakPos;

/* Definition of the WAV header structure */
typedef struct WavHeader
{
	char    chunkId[4];
	int32_t chunkSize;
	char    format[4];
	char    subChunk1Id[4];
	int32_t subChunk1Size;
	int16_t audioFormat;
	int16_t numChannels;
	int32_t sampleRate;
	int32_t byteRate;
	int16_t blockAlign;
	int16_t bitsPerSample;
} WavHeader;

/* Definition of the WAV Fact chunk mandatory for non-PCM formats (like IEEE float) */
typedef struct WavFact
{
    char    chunkId[4];
	int32_t chunkSize;
	int32_t sampleLength;
} WavFact;

/* Definition of the WAV Peak chunk */
typedef struct WavPeak
{
    char    chunkId[4];
    int32_t chunkSize;
    int32_t version;
    int32_t timeStamp;
    PeakPos peak[];
} WavPeak;

/* Definition of the WAV data chunk */
typedef struct WavData
{
    char    chunkId[4];
	int32_t chunkSize;
} WavData;

/* Global variables */
static int16_t  g_AudioFormat;
static int16_t  g_BitsPerSample;
static int16_t  g_NumChannels;
static int32_t  g_SampleRate;
static int32_t  g_SampleLength;
static int32_t  g_TotalSize;
static int      g_PeakChunkLength;
static WavPeak *g_PeakChunk = NULL;

/* Reads the WAV header from a file */
static inline int parseWavHeader(FILE *f)
{
	WavHeader header;
	WavFact fact;
	WavData data;
	char sbuf[4];

	/* Reading the header */
	fread(&header, sizeof(WavHeader), 1, f);

	if (!CHECK_MAGIC(header.chunkId, "RIFF"))
    {
        return 0;
    }

    g_AudioFormat   = header.audioFormat;
	g_BitsPerSample = header.bitsPerSample;
	g_NumChannels   = header.numChannels;
	g_SampleRate    = header.sampleRate;

    /* If the format is PCM */
    if (header.audioFormat == 1)
    {
        /* Checking if the bits per sample are correct */
        if (header.bitsPerSample != 16 && header.bitsPerSample != 24 && header.bitsPerSample != 32)
        {
            return 0;
        }

        /* Checking the presence of the optional "fact chunk" */
        fread(&fact, sizeof(fact), 1, f);

        if (!CHECK_MAGIC(fact.chunkId, "fact"))
        {
            fseek(f, -sizeof(fact), SEEK_CUR);
        }

        /* Reading the data chunk */
        fread(&data, sizeof(data), 1, f);

        g_TotalSize = data.chunkSize;

        return 1;
    }
    /* If the format is IEEE float */
    else if (header.audioFormat == 3)
    {
        /* Checking if the bits per sample are correct */
        if (header.bitsPerSample != 32 && header.bitsPerSample != 64)
        {
            return 0;
        }

        /* The IEEE float specifies that an extension must be present on the header chunk,
         * however, 32-bit floating point WAV exported from Audacity doesn't have it.
         *
         * So I guess it's actually not mandatory ?...
         */

        /* Skipping directly to the fact chunk */
        while (!feof(f))
        {
            if (CHECK_MAGIC(*fgets(sbuf, 5, f), "fact"))
            {
                fseek(f, -4, SEEK_CUR);
                break;
            }

            fseek(f, -3, SEEK_CUR);
        }

        /* Reading the fact chunk */
        fread(&fact, sizeof(fact), 1, f);

        g_SampleLength = fact.sampleLength;

        /* Initializing the peak chunk relative to the number of channels */
        g_PeakChunkLength = sizeof(WavPeak) + sizeof(PeakPos) * g_NumChannels;
        g_PeakChunk = malloc(g_PeakChunkLength);

        /* Reading the peak chunk */
        fread(g_PeakChunk, g_PeakChunkLength, 1, f);

        /* Reading the data chunk */
        fread(&data, sizeof(data), 1, f);

        g_TotalSize = data.chunkSize;

        return 1;
    }

    return 0;
}

/* Write the WAV header to a file */
static inline void writeWavHeader(FILE *f)
{
	WavHeader header = {0};
	WavFact fact = {0};
	WavData data = {0};

	/* Setting the WAV header */
	SET_MAGIC(header.chunkId, "RIFF");
	header.chunkSize = (g_TotalSize * 2) + sizeof(WavHeader);
	SET_MAGIC(header.format, "WAVE");

	SET_MAGIC(header.subChunk1Id, "fmt ");
	header.subChunk1Size    = 16;
	header.audioFormat      = g_AudioFormat;
	header.numChannels      = g_NumChannels;
	header.sampleRate       = g_SampleRate * 2;
	header.byteRate         = g_SampleRate * 2 * g_NumChannels * 2;
	header.blockAlign       = g_NumChannels * 2;
	header.bitsPerSample    = g_BitsPerSample;

	fwrite(&header, sizeof(WavHeader), 1, f);

	/* Setting the WAV fact chunk */
	if (g_AudioFormat != 1)
	{
        SET_MAGIC(fact.chunkId, "fact");
        fact.chunkSize = 4;
        fact.sampleLength = g_SampleLength * 2;

        fwrite(&fact, sizeof(WavFact), 1, f);
	}

	/* Setting the WAV peak chunk */
	if (g_PeakChunk != NULL)
	{
        fwrite(g_PeakChunk, g_PeakChunkLength, 1, f);
	}

	/* Setting the WAV data chunk */
	SET_MAGIC(data.chunkId, "data");
	data.chunkSize = g_TotalSize * 2;

	fwrite(&data, sizeof(WavData), 1, f);
}

/* Processes the sampling for WAV PCM 16 bits */
static inline void processPCM16bits(void *inBuf, void *outBuf)
{
    int16_t *inPtr = (int16_t*) inBuf, *outPtr = (int16_t*) outBuf, min, max, *sample, *sampleHistory;
	int l = g_NumChannels * sizeof(int16_t);
	register int i, c;

	sample = malloc(l);
	sampleHistory = malloc(l);

	/* Proceed to initialize the sample history with zeros or else the algorithm will not work */
    memset(sampleHistory, 0, l);

    for (i = 0; i < g_TotalSize; i += l)
    {
        for (c = 0; c < g_NumChannels; c++)
        {
            sample[c] = *inPtr++;

            min = MIN(sample[c], sampleHistory[c]);
            max = MAX(sample[c], sampleHistory[c]);

            /* Calculate the average of the previous and current sample */
            *outPtr++ = min + ((max - min) / 2);
        }

        for (c = 0; c < g_NumChannels; c++)
        {
            sampleHistory[c] = *outPtr++ = sample[c];
        }
    }

    free(sampleHistory);
    free(sample);
}

/* Processes the sampling for WAV PCM 24 bits */
static inline void processPCM24bits(void *inBuf, void *outBuf)
{
    int24 *inPtr = (int24*) inBuf, *outPtr = (int24*) outBuf, *sample, *sampleHistory;
	int min, max, l = g_NumChannels * sizeof(int24);
	register int i, c;

	sample = malloc(l);
	sampleHistory = malloc(l);

	/* Proceed to initialize the sample history with zeros or else the algorithm will not work */
    memset(sampleHistory, 0, l);

    for (i = 0; i < g_TotalSize; i += l)
    {
        for (c = 0; c < g_NumChannels; c++)
        {
            sample[c] = *inPtr++;

            min = min24(sample[c], sampleHistory[c]);
            max = max24(sample[c], sampleHistory[c]);

            /* Calculate the average of the previous and current sample */
            *outPtr++ = toInt24(min + ((max - min) / 2));
        }

        for (c = 0; c < g_NumChannels; c++)
        {
            sampleHistory[c] = *outPtr++ = sample[c];
        }
    }

    free(sampleHistory);
    free(sample);
}

/* Processes the sampling for WAV PCM 32 bits */
static inline void processPCM32bits(void *inBuf, void *outBuf)
{
    int32_t *inPtr = (int32_t*) inBuf, *outPtr = (int32_t*) outBuf, min, max, *sample, *sampleHistory;
	int l = g_NumChannels * sizeof(int32_t);
	register int i, c;

	sample = malloc(l);
	sampleHistory = malloc(l);

	/* Proceed to initialize the sample history with zeros or else the algorithm will not work */
    memset(sampleHistory, 0, l);

    for (i = 0; i < g_TotalSize; i += l)
    {
        for (c = 0; c < g_NumChannels; c++)
        {
            sample[c] = *inPtr++;

            min = MIN(sample[c], sampleHistory[c]);
            max = MAX(sample[c], sampleHistory[c]);

            /* Calculate the average of the previous and current sample */
            *outPtr++ = min + ((max - min) / 2);
        }

        for (c = 0; c < g_NumChannels; c++)
        {
            sampleHistory[c] = *outPtr++ = sample[c];
        }
    }

    free(sampleHistory);
    free(sample);
}

/* Processes the sampling for WAV IEEE Float 32 bits */
static inline void processFloat32bits(void *inBuf, void *outBuf)
{
    float *inPtr = (float*) inBuf, *outPtr = (float*) outBuf, min, max, *sample, *sampleHistory;
	int l = g_NumChannels * sizeof(float);
	register int i, c;

	sample = malloc(l);
	sampleHistory = malloc(l);

	/* Proceed to initialize the sample history with zeros or else the algorithm will not work */
    memset(sampleHistory, 0, l);

    for (i = 0; i < g_TotalSize; i += l)
    {
        for (c = 0; c < g_NumChannels; c++)
        {
            sample[c] = *inPtr++;

            min = MIN(sample[c], sampleHistory[c]);
            max = MAX(sample[c], sampleHistory[c]);

            /* Calculate the average of the previous and current sample */
            *outPtr++ = min + ((max - min) / 2.0f);
        }

        for (c = 0; c < g_NumChannels; c++)
        {
            sampleHistory[c] = *outPtr++ = sample[c];
        }
    }

    free(sampleHistory);
    free(sample);
}

/* Processes the sampling for WAV IEEE Float 64 bits */
static inline void processFloat64bits(void *inBuf, void *outBuf)
{
    double *inPtr = (double*) inBuf, *outPtr = (double*) outBuf, min, max, *sample, *sampleHistory;
	int l = g_NumChannels * sizeof(double);
	register int i, c;

	sample = malloc(l);
	sampleHistory = malloc(l);

	/* Proceed to initialize the sample history with zeros or else the algorithm will not work */
    memset(sampleHistory, 0, l);

    for (i = 0; i < g_TotalSize; i += l)
    {
        for (c = 0; c < g_NumChannels; c++)
        {
            sample[c] = *inPtr++;

            min = MIN(sample[c], sampleHistory[c]);
            max = MAX(sample[c], sampleHistory[c]);

            /* Calculate the average of the previous and current sample */
            *outPtr++ = min + ((max - min) / 2.0f);
        }

        for (c = 0; c < g_NumChannels; c++)
        {
            sampleHistory[c] = *outPtr++ = sample[c];
        }
    }

    free(sampleHistory);
    free(sample);
}

int main(int argc, char **argv)
{
	FILE *in, *out;
	void *inBuf, *outBuf;

	if (argc < 2 || argc > 3)
	{
        printf("Usage: %s <input wav> <output wav>\n", argv[0]);
		return 0;
	}

	/* Opening the input file */
	if ((in = fopen(argv[1], "rb")) == NULL)
    {
        perror("Error! Can't open the input file.");
        return 1;
    }

    if (!parseWavHeader(in))
	{
		fputs("Error: Invalid WAV file.\n", stderr);
		return 1;
	}

	/* When the output file is specified */
	if (argc == 3)
	{
        if ((out = fopen(argv[2], "wb")) == NULL)
        {
            perror("Error! Can't open the output file.");
            return 1;
        }
	}
	/* When the output file is not specified */
	else if (argc == 2)
	{
        if ((out = fopen(strcat(argv[1], "_Doubled.wav"), "wb")) == NULL)
        {
            perror("Error! Can't open the output file.");
            return 1;
        }
	}

	inBuf  = malloc(g_TotalSize);
	outBuf = malloc(g_TotalSize * 2);

	fread(inBuf, 1, g_TotalSize, in);
	fclose(in);

	printf("Interpolating %s (%d Hz) to %s (%d Hz)...\n", argv[1], g_SampleRate, argv[2], g_SampleRate * 2);

	/* Processing sampling */
	if (g_AudioFormat == 1)
	{
        if (g_BitsPerSample == 16)      processPCM16bits(inBuf, outBuf);
        else if (g_BitsPerSample == 24) processPCM24bits(inBuf, outBuf);
        else                            processPCM32bits(inBuf, outBuf);
	}
	else
	{
        if (g_BitsPerSample == 32)      processFloat32bits(inBuf, outBuf);
        else                            processFloat64bits(inBuf, outBuf);
	}

	writeWavHeader(out);

	fwrite(outBuf, 1, g_TotalSize * 2, out);
	fclose(out);

	if (g_PeakChunk != NULL)
	{
        free(g_PeakChunk);
	}

	free(inBuf);
	free(outBuf);

	puts("Done!");

	return 0;
}
