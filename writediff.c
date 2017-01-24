/************************************************************************\
*                                                                        *
*  Copyright 2017 Barry Biletch                                          *
*                                                                        *
*  This file is part of writediff.                                       *
*                                                                        *
*  writediff is free software: you can redistribute it and/or modify     *
*  it under the terms of the GNU General Public License as published by  *
*  the Free Software Foundation, either version 3 of the License, or     *
*  (at your option) any later version.                                   *
*                                                                        *
*  writediff is distributed in the hope that it will be useful,          *
*  but WITHOUT ANY WARRANTY; without even the implied warranty of        *
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
*  GNU General Public License for more details.                          *
*                                                                        *
*  You should have received a copy of the GNU General Public License     *
*  along with writediff.  If not, see <http://www.gnu.org/licenses/>.    *
*                                                                        *
\************************************************************************/

#define _XOPEN_SOURCE 500

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/stat.h>

#define fail(...) (fprintf(stderr, __VA_ARGS__), exit(1))

int quiet = 0;

size_t parseSize(char *str)
{
	size_t size;
	char *end;

	size = strtol(str, &end, 0);

	if(size<=0)
		fail("Invalid size %s\n", str);

	if(end[0] != '\0')
	{
		if(end[1] != '\0')
			fail("Invalid size %s\n", str);

		switch(end[0])
		{
			case 'k':
			case 'K':
				size *= 1024;
				break;
			case 'm':
			case 'M':
				size *= 1024*1024;
				break;
			case 'g':
			case 'G':
				size *= 1024*1024*1024;
				break;
			default:
				fail("Unknown size modifier %c\n", end[0]);
		}
	}

	return size;
}

void writeDiff(int input, int output, size_t size)
{
	ssize_t inSize;
	char inBuf[size];
	int truncated = 0;
	off_t offset = 0;

	while((inSize = read(input, inBuf, size)) > 0)
	{
		char outBuf[size];
		ssize_t outSize;

		if(!truncated)
		{
			outSize = read(output, outBuf, inSize);
			if(outSize < inSize)
			{
				if(!quiet)
					fputs("warning: output file too short; extending\n", stderr);
				truncated = 1;
			}
		}

		if(truncated || (memcmp(inBuf, outBuf, inSize) != 0))
		{
			ssize_t writeSize;
			writeSize = pwrite(output, inBuf, inSize, offset);

			if(writeSize != inSize)
				fail("Cannot write output file\n");
		}

		offset += inSize;
	}

	if(inSize == -1)
		fail("Cannot read input file\n");


	struct stat stat;
	fstat(output, &stat);
	if((stat.st_size > offset) && S_ISREG(stat.st_mode))
	{
		if(!quiet)
			fputs("warning: output file too large; truncating\n", stderr);
		if(ftruncate(output, offset))
			fail("Cannot truncate output file\n");
	}
}

struct option longops[] = {
	{"input", required_argument, NULL, 'i'},
	{"quiet", no_argument,       NULL, 'q'},
	{"size",  required_argument, NULL, 's'},
	{NULL,    0,                 NULL, 0}
};

int main(int argc, char *argv[])
{
	int opt;
	char *inputFile = NULL, *outputFile;
	size_t size = 512;
	int input, output;

	while((opt = getopt_long(argc, argv, "i:qs:", longops, NULL)) != -1)
	{
		switch(opt)
		{
			case 'i':
				inputFile = optarg;
				break;
			case 'q':
				quiet = 1;
			case 's':
				size = parseSize(optarg);
				break;
			case '?':
				return 1;
			default:
				fail("Unexpected return value from getopt\n");
		}
	}

	if(optind == argc)
		fail("No output file specified\n");
	else if(argc-optind != 1)
		fail("Too many arguments given\n");
	else
		outputFile = argv[optind];

	if(inputFile)
		input = open(inputFile, O_RDONLY);
	else
		input = 0;
	if(input == -1)
		fail("Cannot open input file\n");

	output = open(outputFile, O_RDWR);
	if(output == -1)
		fail("Cannot open output file\n");

	writeDiff(input, output, size);

	return 0;
}
