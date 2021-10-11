static char version[] = "Version 1.1";
static char     sccsid[] = "@(#)cg6_lines.c 1.1 10/31/94 Copyright 1988 Sun Micro";

#include "cg6test.h"

char randvals[] = {
	0xbd, 
	0x2, 0x9b, 0xbe, 0x7f, 0x51, 0x96, 0xb, 0xcf, 0x9e, 0xdb, 
	0x2b, 0x61, 0xf0, 0x6f, 0xf, 0xeb, 0x5a, 0x38, 0xb6, 0x48, 
	0x1, 0x2, 0xa4, 0xbf, 0x89, 0xa9, 0x9b, 0xa6, 0xbb, 0x5c, 
	0x63, 0xbd, 0xf8, 0x22, 0x3d, 0x49, 0xb8, 0x48, 0x18, 0x56, 
	0x23, 0x43, 0xb7, 0x13, 0xb2, 0xc7, 0xff, 0xc, 0xff, 0xb5, 
	0x54, 0x0, 0xb8, 0xf8, 0xbf, 0x41, 0xa2, 0x5b, 0xe7, 0x5d, 
	0xb7, 0x4b, 0x1b, 0xaf, 0x6d, 0x58, 0xf8, 0x25, 0xa0, 0x11, 
	0x7c, 0xc4, 0x54, 0x33, 0xd7, 0x7, 0xfa, 0xd6, 0x13, 0xf9, 
	0x8c, 0x68, 0xfa, 0x44, 0x60, 0xb9, 0x85, 0x2, 0x14, 0x6d, 
	0x60, 0xcc, 0xb8, 0x7b, 0x7b, 0x25, 0xd3, 0x74, 0x4a, 0x73, 
	0x85, 0xc6, 0x37, 0xd9, 0xfa, 0xf, 0xe0, 0xf4, 0xe5, 0xf4, 
	0xee, 0x71, 0x5c, 0xe8, 0xb5, 0xbc, 0xa1, 0x3b, 0xbf, 0xb6, 
	0xa8, 0x1f, 0x82, 0x60, 0x9a, 0xfd, 0x85, 0x6d, 0x71, 0xcf, 
	0xe0, 0xf6, 0x96, 0x18, 0xd0, 0x90, 0x27, 0xb0, 0x84, 0xc, 
	0xa4, 0x72, 0x7e, 0x0, 0x5a, 0x33, 0xbd, 0xfc, 0x6e, 0x7c, 
	0xb2, 0x16, 0x9b, 0x34, 0x76, 0x35, 0x31, 0xfb, 0xa2, 0xa3, 
	0xcb, 0x82, 0x99, 0x61, 0x9a, 0x69, 0xf1, 0xc1, 0x1a, 0x75, 
	0xce, 0xbe, 0xe8, 0x4c, 0xbf, 0x42, 0x7f, 0x7c, 0x3e, 0xee, 
	0xf8, 0xf0, 0x4, 0x93, 0x24, 0x7b, 0xc8, 0x56, 0x76, 0x6a, 
	0xf9, 0x41, 0xec, 0x92, 0xa2, 0x87, 0xfc, 0x93, 0x48, 0x16, 
	0x9, 0x16, 0xd4, 0xf1, 0x62, 0x93, 0x33, 0xe2, 0xf, 0x72, 
	0xd0, 0x7, 0x62, 0xd4, 0x9a, 0x87, 0x4f, 0x62, 0xdd, 0xc6, 
	0xcc, 0xd6, 0x7, 0xb9, 0x68, 0xaa, 0x40, 0x64, 0x3d, 0x88, 
	0x7a, 0x46, 0x9f, 0x4f, 0x37, 0x1, 0xe2, 0x6b, 0xe3, 0xf2, 
	0xdd, 0xb3, 0xf9, 0x3f, 0x88, 0x94, 0xc6, 0xd7, 0xf6, 0xa3, 
	0x9d, 0xc3, 0x79, 0xa5, 0x7c, 0xe2, 0x4f, 0xbc, 0x46, 0x8c, 
	0x44, 0xc1, 0xd3, 0xe3, 0x10, 0xa, 0xe5, 0xf2, 0x75, 0xc8, 
	0xe4, 0x52, 0x7c, 0xde, 0x92, 0x4, 0x72, 0x58, 0xdb, 0x68, 
	0xfc, 0x79, 0x2b, 0x75, 0x1e, 0xa7, 0x57, 0x6d, 0x63, 0x9e, 
	0xf9, 0xa8, 0x5f, 0xcc, 0x8b, 0x6f, 0xd7, 0x70, 0x61, 0x4c, 
	0x39, 0x46, 0x9f, 0xb5, 0x24, 0x31, 0xb9, 0x96, 0x89, 0x94, 
	0xfe, 0x85, 0xd, 0x2a, 0xfb, 0x2b, 0xd1, 0x52, 0x98, 0x35, 
	0xf0, 0x92, 0xdd, 0x4f, 0x5e, 0x68, 0xbe, 0x35, 0xd9, 0x20, 
	0x82, 0x12, 0x66, 0x21, 0xc7, 0x8a, 0x52, 0x80, 0x20, 0xdb, 
	0x14, 0x1e, 0x61, 0x22, 0x48, 0x5c, 0x4d, 0x1a, 0xae, 0xe6, 
	0x4f, 0x9f, 0x78, 0x2c, 0xee, 0xd6, 0x94, 0xad, 0xc, 0x6d, 
	0xcd, 0x8e, 0x7f, 0x33, 0xaf, 0x46, 0xbd, 0x1, 0xc6, 0xdd, 
	0xdc, 0xdb, 0xfb, 0x3d, 0xfd, 0x44, 0x99, 0x4a, 0x5e, 0x48, 
	0x30, 0xad, 0xe7, 0xa8, 0xd9, 0xd5, 0x7f, 0x6d, 0x82, 0x8b, 
	0xdb, 0x4f, 0x19, 0x5a, 0x82, 0xc8, 0xa1, 0x3f, 0xc9, 0x67, 
	0x1c, 0xa5, 0x42, 0x18, 0xe3, 0x3f, 0x5c, 0x7c, 0x8a, 0xba, 
	0xc4, 0xba, 0x67, 0xab, 0x63, 0x40, 0x81, 0xe2, 0xad, 0x3, 
	0x6d, 0x88, 0x53, 0x86, 0xe3, 0xd5, 0x4e, 0x84, 0x15, 0x17, 
	0xeb, 0x31, 0xbc, 0x2e, 0x49, 0x9f, 0x6d, 0xa5, 0x1c, 0xf7, 
	0x5f, 0xe0, 0xb2, 0xc6, 0x8c, 0x15, 0x6, 0xd, 0xf7, 0xb4, 
	0x10, 0x64, 0x3c, 0x63, 0xea, 0x1f, 0x39, 0x38, 0xa3, 0x4e, 
	0x4f, 0x8f, 0x7f, 0xb, 0xbd, 0xc9, 0xab, 0x2a, 0x6e, 0xc7, 
	0x22, 0xce, 0xa7, 0xd4, 0x94, 0x33, 0xe9, 0x9b, 0x40, 0xe0, 
	0x4f, 0x51, 0x44, 0x8b, 0xb4, 0x2e, 0xab, 0xed, 0x66, 0x4e, 
	0x3b, 0xb5, 0xdd, 0xbb, 0xc0, 0x9a, 0x84, 0x6b, 0xc5, 0xf2, 
	0x32, 0xe7, 0xc0, 0xda, 0xbb, 0x55, 0xd, 0xa4, 0xf0, 0x4e, 
	0x84, 0x3f, 0x9f, 0xc8, 0xca, 0x53, 0xf6, 0x75, 0x41, 0x5c, 
	0xc4, 0x7c, 0x11, 0xa1, 0x37, 0xd1, 0x3c, 0xbb, 0x3d, 0x1, 
	0xae, 0x6f, 0xe8, 0x6e, 0x49, 0xa3, 0xc3, 0x57, 0x47, 0xb3, 
	0xa5, 0xcb, 0xf2, 0x44, 0x93, 0xbd, 0x97, 0x89, 0x32, 0xd8, 
	0xe5, 0xf6, 0x55, 0xf6, 0x98, 0x8c, 0xc7, 0xd4, 0x48, 0x4, 
	0xd5, 0xf6, 0x74, 0xbd, 0x64, 0xbd, 0x60, 0x28, 0x14, 0xa7, 
	0xdb, 0xb9, 0x72, 0xce, 0xfd, 0x5, 0x8b, 0x95, 0x8e, 0xbd, 
	0x6d, 0x73, 0xb4, 0xc2, 0x69, 0x4c, 0x4f, 0x30, 0x20, 0x97, 
	0x35, 0xf5, 0x8d, 0xa9, 0xb2, 0xf1, 0x66, 0x12, 0x19, 0x7b, 
	0xb9, 0xf5, 0x34, 0x2b, 0xc3, 0x32, 0x30, 0x4e, 0xc7, 0xbe, 
	0xb, 0x34, 0x31, 0xbf, 0xf7, 0x9a, 0xb, 0x46, 0xca, 0x2b, 
	0xdd, 0xff, 0x20, 0x6a, 0xa8, 0xd2, 0x5b, 0xf, 0xe4, 0x75, 
	0x8a, 0x9d, 0x6a, 0xbe, 0xc8, 0x2d, 0xf0, 0xf8, 0x7b, 0xb7, 
	0xb6, 0x86, 0xec, 0xe7, 0x46, 0xe3, 0x81, 0x51, 0x29, 0x4c, 
	0x7d, 0x6, 0x4b, 0x9d, 0x70, 0xf4, 0x70, 0xcb, 0x3, 0x54, 
	0x40, 0x8d, 0xf2, 0xaa, 0x4b, 0xba, 0xd7, 0x3c, 0xb3, 0x52, 
	0xf3, 0x69, 0xd9, 0xdf, 0x51, 0x1f, 0xc2, 0xd2, 0x70, 0xeb, 
	0x1e, 0xed, 0xf1, 0x6a, 0x8b, 0x61, 0x5e, 0xfb, 0x2d, 0x61, 
	0x4f, 0x6d, 0xee, 0x41, 0x18, 0x39, 0xfc, 0xef, 0x75, 
};

test_lines()
{
	int i, x0, y0, xmid, ymid;

	red1[0] = 0;
	grn1[0] = 0;
	blu1[0] = 0;
	for (i = 1; i <= 255; i++)
	{
		switch ( i % 3 ) {
		case 0:
			red1[i] = 0xff;
			grn1[i] = 0;
			blu1[i] = 0;
		break;
		case 1:
			red1[i] = 0x0;
			grn1[i] = 0xff;
			blu1[i] = 0;
		break;
		case 2:
			red1[i] = 0x0;
			grn1[i] = 0x0;
			blu1[i] = 0xff;
		break;
		}
	}
	pr_putcolormap(prfd, 0, 256, red1, grn1, blu1);

	clear_screen();
	test_lattice();
/*### does not work due to pixel imperfection
	clear_screen();
	test_all_lines();
##*/
	return 0;
}

test_lattice()
{
	int i, relx, rely, size;

	size = 50;
	i = 0;
	for (rely = 0; rely < height; rely += size) {
		for (relx = 0; relx < width; relx += size) {
			/* draw on screen */
			pr_vector (prfd, relx, rely, size+relx, size+rely,
							PIX_SRC, randvals[i] );
			pr_vector (prfd, size+relx, rely, relx, size+rely,
							PIX_SRC, randvals[i+1] );

			/* draw into memory comparitor */
			pr_vector (mexp, relx, rely, size+relx, size+rely,
							PIX_SRC, randvals[i] );
			pr_vector (mexp, size+relx, rely, relx, size+rely,
							PIX_SRC, randvals[i+1] );
			i++;
			if ( sizeof randvals - 4 - i <= 0 )
				i = 0;
		}
	}
	pr_rop(mobs,0, 0, width, height, PIX_SRC,prfd,0,0);
	ropcheck(0, 0, width, height, "test simple lines");

	return 0;
}

test_all_lines()
{
	int i, x0, y0, xmid, ymid;

	xmid = width/2;
	ymid = height/2;

	x0 = 0;
	y0 = 0;
	for (i = 0; i < 400; i++ ) {
		pr_vector (prfd, x0, y0, xmid, ymid, PIX_SRC, randvals[i] );
		pr_vector (mexp, x0, y0, xmid, ymid, PIX_SRC, randvals[i] );
		x0 += 3;
	}
	for (i = 0; i < 400; i++ ) {
		pr_vector (prfd, x0, y0, xmid, ymid, PIX_SRC, randvals[i] );
		pr_vector (mexp, x0, y0, xmid, ymid, PIX_SRC, randvals[i] );
		y0 += 3;
	}
	for (i = 0; i < 400; i++ ) {
		pr_vector (prfd, x0, y0, xmid, ymid, PIX_SRC, randvals[i] );
		pr_vector (mexp, x0, y0, xmid, ymid, PIX_SRC, randvals[i] );
		x0 -= 3;
	}
	for (i = 0; i < 400; i++ ) {
		pr_vector (prfd, x0, y0, xmid, ymid, PIX_SRC, randvals[i] );
		pr_vector (mexp, x0, y0, xmid, ymid, PIX_SRC, randvals[i] );
		y0 -= 3;
	}
	pr_rop(mobs,0, 0, width, height, PIX_SRC,prfd,0,0);

	ropcheck(0, 0, width, height, "test lines");

	/* now test the clipping */

	sprfd = pr_region ( prfd, 200, 200, 400, 400 );

	pr_rop (sprfd, -200, -200, width, height, PIX_SRC, prfd, xmid, ymid );
	pr_rop (sprfd, -200, -200, width, height, PIX_SRC | PIX_DONTCLIP,
							prfd, xmid, ymid );
	return 0;
}
