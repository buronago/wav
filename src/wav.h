/* This bitfield goes unused; it simply verifies that sizeof(short) is 2 and
   sizeof(int) is 4. If not, then the members have sizes of 0, which errors. */
typedef struct {
	int check_if_short_is_2_bytes: sizeof(short) == 2;
	int check_if_int_is_4_bytes: sizeof(int) == 4;
} __check_type_sizes;

typedef struct {
	int full;
	int fmt;
	int data;
} WAVMetadataSize;

typedef struct {
	short format;
	short channels;

	short bits_per_sample;
	int sample_rate;

	WAVMetadataSize size;
} WAVMetadata;
