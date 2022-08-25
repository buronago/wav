/* Reads waveform data and plays it to the speakers.
 * -----------------------------------------------------------------------------
 * Licensed under the Beer-Ware license (revision 42) as below:
 *
 *   pyurai <pyurai@proton.me> wrote this file. As long as you retain this
 *   notice you can do whatever you want with this stuff. If we meet some day,
 *   and you think this stuff is worth it, you can buy me a beer in return.
 *
 * The Beer-Ware license was written by Poul-Henning Kamp.
 * -----------------------------------------------------------------------------
 * https://github.com/pyuraii/wav
 */

#include <stdio.h>
#include <stdlib.h>

#include <alsa/asoundlib.h>
#include <arpa/inet.h>

#include "wav.h"

#define E01 "Couldn't open file"
#define E02 "Not a valid Microsoft RIFF file"
#define E03 "Not a valid WAV file"
#define E04 "Unexpectedly encountered EOF"
#define E05 "Internal WAV test cases failed"
#define E06 "Couldn't allocate memory for audio buffer"
#define E07 "Only PCM WAVs are supported"

#define E08 "Playback open error: ", 0, NULL
#define E09 "snd_pcm_writei failed: ", 1, handle
#define E10 "snd_pcm_drain failed: ", 1, handle

/* See verify_metadata. */
#define FAST_TESTS

void error(const char *message) {
	fprintf(stderr, "Error: %s\n", message);
	exit(1);
}

void error_snd(const char *message, int close, snd_pcm_t *handle) {
	fprintf(stderr, "Error: %s%s\n", message, snd_strerror(err));
	if (close) snd_pcm_close(handle)
	exit(2);
}

void verify_string(FILE *file, char *string, const char *message) {
	int i, length = *string++;
	char c;

	for (i = 0; i < length; i++, string++) {
		c = fgetc(file);

		if (c == EOF) error(E04);
		else if (*string != c) error(message);
	}

	/* If we reach here, the strings are equal. */
}

void read_i32(FILE *file, int *result) {
	if (!fread(result, 4, 1, file))
		error(E03);
}

void read_i16(FILE *file, short *result) {
	if (!fread(result, 2, 1, file))
		error(E03);
}

void verify_metadata(WAVMetadata *wav, int test1, short test2) {
	#ifdef SKIP_TESTS
		/* This will skip ALL tests. */
		return;
	#endif /* SKIP_TESTS */

	#ifdef FAST_TESTS
		/* This will just check if test1 and test2 are related with a
		 * factor of wav->sample_rate. In 99.99% of cases, this will be
		 * correct. Turn this off for the 0.01% of cases. */
		if (test1 / test2 != wav->sample_rate) error(E05);
		return;
	#endif /* FAST_TESTS */

	int tester = (wav->bits_per_sample * wav->channels) / 8;

	if ((test1 != tester * wav->sample_rate) ||
	    (test2 != tester)) error(E05);
}

void parse_fmt(FILE *file, WAVMetadata *wav) {
	verify_string(file, "\4RIFF", E02);
	read_i32(file, &wav->size.full);

	verify_string(file, "\10WAVEfmt ", E03);
	read_i32(file, &wav->size.fmt);

	read_i16(file, &wav->format);
	read_i16(file, &wav->channels);
	read_i32(file, &wav->sample_rate);
	
	int test1;
	short test2;

	read_i32(file, &test1);
	read_i16(file, &test2);

	read_i16(file, &wav->bits_per_sample);
	verify_metadata(wav, test1, test2);

	verify_string(file, "\4data", E03);
	read_i32(file, &wav->size.data);
}

void parse_data(FILE *file, WAVMetadata *wav) {
	snd_pcm_t *handle;
	snd_pcm_sframes_t frames;

	if (wav->format != 1) error(E07);

	char *buffer = malloc(wav->size.data);
	if (buffer == NULL) error(E06);

	if (snd_pcm_open(&handle, "default", SND_PCM_STREAM_PLAYBACK, 0) < 0)
		error_snd(E08);

	if (snd_pcm_set_params(
		handle, SND_PCM_FORMAT_U8, SND_PCM_ACCESS_RW_INTERLEAVED, wav->channels,
		wav->sample_rate, 1, 500000,
	) > 0) error_snd(E08);

	int i;

	for (i = 0; i < 16; i++) {
		frames = snd_pcm_writei(handle, buffer, sizeof(buffer));

		if (frames < 0) frames = snd_pcm_recover(handle, frames, 0);
		if (frames < 0) error_snd(E09);

		if (frames > 0 && frames < (long)sizeof(buffer))
			printf("Expected %li, wrote %li\n", (long)sizeof(buffer), frames);
	}

	if (snd_pcm_drain(handle) < 0) error_snd(E10);
	snd_pcm_close(handle);
}

void usage(const char *program) {
	fprintf(stderr, "usage: %s <file>\n", program);
	exit(-1);
}

int main(int argc, char *argv[]) {
	if (argc != 2) usage(argv[0]);
	WAVMetadata wav;

	FILE *file = fopen(argv[1], "rb");
	if (file == NULL) error(E01);

	parse_fmt(file, &wav);
	parse_data(file, &wav);

	return 0;
}
