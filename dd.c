/*
 * UNG's Not GNU
 *
 * Copyright (c) 2011-2019, Jakob Kaivo <jkk@ung.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#define _XOPEN_SOURCE 700
#include <errno.h>
#include <inttypes.h>
#include <locale.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

enum {
	IF,
	OF,
	IBS,
	OBS,
	BS,
	CBS,
	SKIP,
	SEEK,
	COUNT,
};

static struct {
	const char *name;
	enum { STRING, EXPR, NUMBER } type;
	int isset;
	union {
		char *s;
		size_t i;
	} value;
} opts[] = {
	[IF] =		{ "if",		STRING, 0, { .s = NULL } },
	[OF] =		{ "of",		STRING, 0, { .s = NULL } },
	[IBS] =		{ "ibs",	EXPR,	0, { .i = 512 } },
	[OBS] =		{ "obs",	EXPR,	0, { .i = 512 } },
	[BS] =		{ "bs",		EXPR,	0, { .i = 0} },
	[CBS] =		{ "cbs",	EXPR,	0, { .i = 0} },
	[SKIP] =	{ "skip",	NUMBER,	0, { .i = 0} },
	[SEEK] =	{ "seek",	NUMBER,	0, { .i = 0} },
	[COUNT] =	{ "count",	NUMBER,	0, { .i = 0} },
};
static size_t nopts = sizeof(opts) / sizeof(opts[0]);

enum {
	ASCII	= 1 << 0,
	EBCDIC	= 1 << 1,
	IBM	= 1 << 2,
	BLOCK	= 1 << 3,
	UNBLOCK = 1 << 4,
	LCASE	= 1 << 5,
	UCASE	= 1 << 6,
	SWAB	= 1 << 7,
	NOERROR	= 1 << 8,
	NOTRUNC	= 1 << 9,
	SYNC	= 1 << 10,
};

static struct {
	const char *name;
	unsigned int flag;
} convs[] = {
	{ "ascii",	ASCII },
	{ "ebcdic",	EBCDIC },
	{ "ibm",	IBM },
	{ "block",	BLOCK },
	{ "unblock",	UNBLOCK },
	{ "lcase",	LCASE },
	{ "ucase",	UCASE },
	{ "swab",	SWAB },
	{ "noerror",	NOERROR },
	{ "notrunc",	NOTRUNC },
	{ "sync",	SYNC },
};
static size_t nconvs = sizeof(convs) / sizeof(convs[0]);

static unsigned char ebcdic[] = {
	0000, 0001, 0002, 0003, 0067, 0055, 0056, 0057,
	0026, 0005, 0045, 0013, 0014, 0015, 0016, 0017,
	0020, 0021, 0022, 0023, 0074, 0075, 0062, 0046,
	0030, 0031, 0077, 0047, 0034, 0035, 0036, 0037,
	0100, 0132, 0177, 0173, 0133, 0154, 0120, 0175,
	0115, 0135, 0134, 0116, 0153, 0140, 0113, 0141,
	0360, 0361, 0362, 0363, 0364, 0365, 0366, 0367,

	0174, 0301, 0302, 0303, 0304, 0305, 0306, 0307,
	0310, 0311, 0321, 0322, 0323, 0324, 0325, 0326,
	0327, 0330, 0331, 0342, 0343, 0344, 0345, 0346,
	0347, 0350, 0351, 0255, 0340, 0275, 0232, 0155,
	0171, 0201, 0202, 0203, 0204, 0205, 0206, 0207,
	0210, 0211, 0221, 0222, 0223, 0224, 0225, 0226,
	0227, 0230, 0231, 0242, 0243, 0244, 0245, 0246,
	0247, 0250, 0251, 0300, 0117, 0320, 0137, 0007,

	0040, 0041, 0042, 0043, 0044, 0025, 0006, 0027,
	0050, 0051, 0052, 0053, 0054, 0011, 0012, 0033,
	0060, 0061, 0032, 0063, 0064, 0065, 0066, 0010,
	0070, 0071, 0072, 0073, 0004, 0024, 0076, 0341,
	0101, 0102, 0103, 0104, 0105, 0106, 0107, 0110,
	0111, 0121, 0122, 0123, 0124, 0125, 0126, 0127,
	0130, 0131, 0142, 0143, 0144, 0145, 0146, 0147,
	0150, 0151, 0160, 0161, 0162, 0163, 0164, 0165,

	0166, 0167, 0170, 0200, 0212, 0213, 0214, 0215,
	0216, 0217, 0220, 0152, 0233, 0234, 0235, 0236,
	0237, 0240, 0252, 0253, 0254, 0112, 0256, 0257,
	0260, 0261, 0262, 0263, 0264, 0265, 0266, 0267,
	0270, 0271, 0272, 0273, 0274, 0241, 0276, 0277,
	0312, 0313, 0314, 0315, 0316, 0317, 0332, 0333,
	0334, 0335, 0336, 0337, 0352, 0353, 0354, 0355,
	0356, 0357, 0372, 0373, 0374, 0375, 0376, 0377,
};

int setopt(size_t n, char *value)
{
	char *end = NULL;
	intmax_t i = strtoimax(value, &end, 10);

	switch (opts[n].type) {
	case STRING:
		opts[n].value.s = value;
		break;

	case EXPR:
		/* TODO: handle expr x expr */
		switch (*end) {
		case 'k':
			i *= 1024;
			break;

		case 'b':
			i *= 512;
			break;

		case '\0':
			break;

		default:
			fprintf(stderr, "dd: invalid size: %s\n", value);
			return 1;
		}
		opts[n].value.i = i;
		break;

	case NUMBER:	/* TODO: check for extra stuff */
		opts[n].value.i = i;
		if (*end != '\0') {
			fprintf(stderr, "dd: invalid count: %s\n", value);
			return 1;
		}
		break;
	}

	opts[n].isset = 1;

	return 0;
}

unsigned int getconv(const char *v)
{
	for (size_t i = 0; i < nconvs; i++) {
		if (!strcmp(v, convs[i].name)) {
			return convs[i].flag;
		}
	}
	return 0;
}

unsigned int setconvs(char *values)
{
	unsigned int conversions = 0;

	for (char *v = strtok(values, ","); v != NULL; v = strtok(NULL, ",")) {
		unsigned int flag = getconv(v);
		if (flag == 0) {
			printf("dd: unknown conversion %s\n", v);
			return 0;
		}
		conversions |= flag;
	}

	return conversions;
}

int nsetbits(unsigned int i)
{
	int n = 0;
	while (i) {
		if (i & 1) {
			n++;
		}
		i = i >> 1;
	}
	return n;
}

int mutexconv(unsigned int flags, unsigned int mask)
{
	if (nsetbits(flags & mask) <= 1) {
		return 0;
	}

	fprintf(stderr, "dd: mutually exclusive conversions:");
	for (size_t i = 0; i < nconvs; i++) {
		if (mask & convs[i].flag & flags) {
			fprintf(stderr, " %s", convs[i].name);
		}
	}
	fprintf(stderr, "\n");
	return 1;
}

int main(int argc, char *argv[])
{
	setlocale(LC_ALL, "");

	unsigned int conversions = 0;

	int c;
	while ((c = getopt(argc, argv, "")) != -1) {
		fprintf(stderr, "dd: -o style options not supported\n");
		return 1;
	}

	for (int i = optind; i < argc; i++) {
		char *operand = argv[i];
		char *equals = strchr(operand, '=');
		if (!equals) {
			fprintf(stderr, "dd: unknown operand %s\n", operand);
			return 1;
		}
		
		*equals = '\0';
		if (!strcmp(operand, "conv")) {
			conversions = setconvs(equals + 1);
			if (conversions == 0) {
				return 1;
			}
			continue;
		}

		for (size_t j = 0; j < nopts; j++) {
			if (!strcmp(operand, opts[j].name)) {
				if (setopt(j, equals + 1) != 0) {
					return 1;
				}
				break;
			}
			if (j == nopts - 1) {
				fprintf(stderr, "dd: unknown operand %s\n", operand);
				return 1;
			}
		}
	}

	if (opts[BS].isset) {
		opts[IBS].value.i = opts[BS].value.i;
		opts[OBS].value.i = opts[BS].value.i;
	}

	if (mutexconv(conversions, ASCII | EBCDIC | IBM) != 0) {
		return 1;
	}

	if (mutexconv(conversions, BLOCK | UNBLOCK) != 0) {
		return 1;
	}

	if (mutexconv(conversions, LCASE | UCASE) != 0) {
		return 1;
	}

	FILE *in = opts[IF].isset ? fopen(opts[IF].value.s, "rb") : stdin;
	if (in == NULL) {
		fprintf(stderr, "dd: %s: %s\n", opts[IF].value.s, strerror(errno));
		return 1;
	}

	FILE *out = opts[OF].isset ? fopen(opts[OF].value.s, "wb") : stdout;
	if (out == NULL) {
		fprintf(stderr, "dd: %s: %s\n", opts[OF].value.s, strerror(errno));
		return 1;
	}

	size_t wholein = 0;
	size_t partialin = 0;
	size_t wholeout = 0;
	size_t partialout = 0;
	size_t truncated = 0;

	/* TODO */

	fprintf(stderr, "%zu+%zu records in\n", wholein, partialin);
	fprintf(stderr, "%zu+%zu records out\n", wholeout, partialout);
	if (truncated != 0) {
		fprintf(stderr, "%zu truncated %s\n", truncated,
			truncated == 1 ? "record" : "records");
	}
	return 0;
}
