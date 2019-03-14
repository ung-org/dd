#define _XOPEN_SOURCE 700

#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

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

size_t number(const char *s)
{
	return 0;
}

size_t blocksize(const char *s)
{
	size_t size = 0;
	char *end = NULL;
	while (end == NULL) {
		uintmax_t n = strtoumax(s, &end, 10);
		switch (*end) {
		case 'k':
			n *= 1024;
			break;
		case 'b':
			n *= 512;
			break;
		case '\0':
			break;
		default:
			fprintf(stderr, "Unrecognized character %c\n", *end);
			return 1;
		}
		size = n;
	}
	return size;
}

int
main(int argc, char *argv[])
{
	char *ifname = NULL;
	char *ofname = NULL;
	size_t ibs = 512;
	size_t obs = 512;
	size_t cbs = 0;
	size_t skip = 0;
	size_t seek = 0;
	size_t count = 0;
	size_t in_whole = 0, in_part = 0;
	size_t out_whole = 0, out_part = 0;
	size_t truncated = 0;

	/* TODO: conv */
	
	int c;
	while ((c = getopt(argc, argv, "")) != -1) {
		fprintf(stderr, "dd does not support -o style options\n");
		return 1;
	}

	for (int i = 1; i < argc; i++) {
		char *operand = argv[i];
		char *equals = strchr(operand, '=');
		if (!equals) {
			fprintf(stderr, "Unrecognized operand %s\n", operand);
			return 1;
		}
		
		*equals = '\0';
		if (!strcmp(operand, "if")) {
			ifname = equals + 1;
		} else if (!strcmp(operand, "of")) {
			ofname = equals + 1;
		} else if (!strcmp(operand, "ibs")) {
			ibs = blocksize(equals + 1);
		} else if (!strcmp(operand, "obs")) {
			obs = blocksize(equals + 1);
		} else if (!strcmp(operand, "bs")) {
			ibs = blocksize(equals + 1);
			obs = ibs;
		} else if (!strcmp(operand, "cbs")) {
			cbs = number(equals + 1);
		} else if (!strcmp(operand, "skip")) {
			skip = number(equals + 1);
		} else if (!strcmp(operand, "seek")) {
			seek = number(equals + 1);
		} else if (!strcmp(operand, "count")) {
			count = number(equals + 1);
		} else if (!strcmp(operand, "conv")) {
		} else {
			fprintf(stderr, "Unrecognized operand %s\n", operand);
			return 1;
		}
	}

	unsigned char *buf = malloc(ibs > obs ? ibs : obs);
	if (buf == NULL) {
		fprintf(stderr, "Unable to allocate buffer\n");
		return 1;
	}

	FILE *iff = ifname ? fopen(ifname, "rb") : stdin;
	if (!iff) {
		fprintf(stderr, "Error opening %s\n", ifname);
		return 1;
	}

	FILE *of = ofname ? fopen(ofname, "wb") : stdout;
	if (!of) {
		fprintf(stderr, "Error opening %s\n", ofname);
		return 1;
	}
	
	printf("if=%s\n", ifname);
	printf("of=%s\n", ofname);
	printf("ibs=%zd\n", ibs);
	printf("obs=%zd\n", obs);
	printf("cbs=%zd\n", cbs);
	printf("skip=%zd\n", skip);
	printf("seek=%zd\n", seek);
	printf("count=%zd\n", count);
	printf("conv=\n");

	for (size_t blocks = 0; blocks < count; blocks++) {
		size_t nread = fread(buf, 1, ibs, iff);
		if (/* sync && */ nread < ibs) {
			char pad = /* block || unblock ? ' ' : */ '\0';
			memset(buf + nread, pad, ibs - nread);
		}
		/* swab */
		/* block, unblock, lcase, ucase */
		fwrite(buf, 1, obs, of);
	}

	fprintf(stderr, "%zu+%zu records in\n", in_whole, in_part);
	fprintf(stderr, "%zu+%zu records out\n", out_whole, out_part);
	if (truncated) {
		fprintf(stderr, "%zu truncated %s\n", truncated,
			truncated == 1 ? "record" : "records");
	}
}
