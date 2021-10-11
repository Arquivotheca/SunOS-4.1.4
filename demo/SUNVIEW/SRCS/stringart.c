/*	stringart.c	1.1	94/10/31	*/

#include <stdio.h>
#include <usercore.h>
#include "demolib.h"

/*
	This demo creates random vector designs.  This is accomplished by
	randomly choosing a function for each coordinate halve of the two
	points describing a vector that moves through two dimensional
	space. Both x coordinate halves cannot be the same since the design
	would simply be a collection of vertical lines. Similarly both
	y coordinate halves cannot be the same.

	The functions are:

	function[0][x] = 2 * abs( x - (NUMLINES/2) - 1 )
	function[1][x] = sin( 2*pi*x/NUMLINES )
	function[2][x] = -sin( 2*pi*x/NUMLINES )
	function[3][x] = cos( 2*pi*x/NUMLINES ) 
	function[4][x] = -cos( 2*pi*x/NUMLINES ) 
	function[5][x] = sin( 4*pi*x/NUMLINES )
	function[6][x] = -sin( 4*pi*x/NUMLINES )
	function[7][x] = cos( 4*pi*x/NUMLINES ) 
	function[8][x] = -cos( 4*pi*x/NUMLINES ) 
	function[9][x] = sin( 6*pi*x/NUMLINES )
	function[10][x] = -sin( 6*pi*x/NUMLINES )
	function[11][x] = cos( 6*pi*x/NUMLINES ) 
	function[12][x] = -cos( 6*pi*x/NUMLINES ) 

	The values of the functions were pre computed to have the demo
	run as fast as possible.  The program will only terminate on 
	interrupt since it is in an endless loop.
*/

#define NUMLINES	343	/* number of vectors in a design */
#define NUM_FUNCTIONS	13	/* number of functions */
#define MAPSIZE		114


static float red[MAPSIZE+1], grn[MAPSIZE+1], blu[MAPSIZE+1];

static float function[13][NUMLINES] = {

	{
	1.0000,0.9883,0.9767,0.9650,0.9534,0.9417,0.9300,
	0.9184,0.9067,0.8950,0.8834,0.8717,0.8601,0.8484,
	0.8367,0.8251,0.8134,0.8017,0.7901,0.7784,0.7668,
	0.7551,0.7434,0.7318,0.7201,0.7085,0.6968,0.6851,
	0.6735,0.6618,0.6501,0.6385,0.6268,0.6152,0.6035,
	0.5918,0.5802,0.5685,0.5569,0.5452,0.5335,0.5219,
	0.5102,0.4985,0.4869,0.4752,0.4636,0.4519,0.4402,
	0.4286,0.4169,0.4052,0.3936,0.3819,0.3703,0.3586,
	0.3469,0.3353,0.3236,0.3120,0.3003,0.2886,0.2770,
	0.2653,0.2536,0.2420,0.2303,0.2187,0.2070,0.1953,
	0.1837,0.1720,0.1603,0.1487,0.1370,0.1254,0.1137,
	0.1020,0.0904,0.0787,0.0671,0.0554,0.0437,0.0321,
	0.0204,0.0087,-0.0029,-0.0146,-0.0262,-0.0379,-0.0496,
	-0.0612,-0.0729,-0.0845,-0.0962,-0.1079,-0.1195,-0.1312,
	-0.1429,-0.1545,-0.1662,-0.1778,-0.1895,-0.2012,-0.2128,
	-0.2245,-0.2362,-0.2478,-0.2595,-0.2711,-0.2828,-0.2945,
	-0.3061,-0.3178,-0.3294,-0.3411,-0.3528,-0.3644,-0.3761,
	-0.3878,-0.3994,-0.4111,-0.4227,-0.4344,-0.4461,-0.4577,
	-0.4694,-0.4810,-0.4927,-0.5044,-0.5160,-0.5277,-0.5394,
	-0.5510,-0.5627,-0.5743,-0.5860,-0.5977,-0.6093,-0.6210,
	-0.6327,-0.6443,-0.6560,-0.6676,-0.6793,-0.6910,-0.7026,
	-0.7143,-0.7259,-0.7376,-0.7493,-0.7609,-0.7726,-0.7843,
	-0.7959,-0.8076,-0.8192,-0.8309,-0.8426,-0.8542,-0.8659,
	-0.8776,-0.8892,-0.9009,-0.9125,-0.9242,-0.9359,-0.9475,
	-0.9592,-0.9708,-0.9825,-0.9942,-0.9942,-0.9825,-0.9708,
	-0.9592,-0.9475,-0.9359,-0.9242,-0.9125,-0.9009,-0.8892,
	-0.8776,-0.8659,-0.8542,-0.8426,-0.8309,-0.8192,-0.8076,
	-0.7959,-0.7843,-0.7726,-0.7609,-0.7493,-0.7376,-0.7259,
	-0.7143,-0.7026,-0.6910,-0.6793,-0.6676,-0.6560,-0.6443,
	-0.6327,-0.6210,-0.6093,-0.5977,-0.5860,-0.5743,-0.5627,
	-0.5510,-0.5394,-0.5277,-0.5160,-0.5044,-0.4927,-0.4810,
	-0.4694,-0.4577,-0.4461,-0.4344,-0.4227,-0.4111,-0.3994,
	-0.3878,-0.3761,-0.3644,-0.3528,-0.3411,-0.3294,-0.3178,
	-0.3061,-0.2945,-0.2828,-0.2711,-0.2595,-0.2478,-0.2362,
	-0.2245,-0.2128,-0.2012,-0.1895,-0.1778,-0.1662,-0.1545,
	-0.1429,-0.1312,-0.1195,-0.1079,-0.0962,-0.0845,-0.0729,
	-0.0612,-0.0496,-0.0379,-0.0262,-0.0146,-0.0029,0.0087,
	0.0204,0.0321,0.0437,0.0554,0.0671,0.0787,0.0904,
	0.1020,0.1137,0.1254,0.1370,0.1487,0.1603,0.1720,
	0.1837,0.1953,0.2070,0.2187,0.2303,0.2420,0.2536,
	0.2653,0.2770,0.2886,0.3003,0.3120,0.3236,0.3353,
	0.3469,0.3586,0.3703,0.3819,0.3936,0.4052,0.4169,
	0.4286,0.4402,0.4519,0.4636,0.4752,0.4869,0.4985,
	0.5102,0.5219,0.5335,0.5452,0.5569,0.5685,0.5802,
	0.5918,0.6035,0.6152,0.6268,0.6385,0.6501,0.6618,
	0.6735,0.6851,0.6968,0.7085,0.7201,0.7318,0.7434,
	0.7551,0.7668,0.7784,0.7901,0.8017,0.8134,0.8251,
	0.8367,0.8484,0.8601,0.8717,0.8834,0.8950,0.9067,
	0.9184,0.9300,0.9417,0.9534,0.9650,0.9767,0.9883
	},

	{
	0.0000,0.0184,0.0367,0.0551,0.0734,0.0917,0.1100,
	0.1282,0.1464,0.1646,0.1827,0.2007,0.2187,0.2366,
	0.2544,0.2721,0.2897,0.3073,0.3247,0.3420,0.3592,
	0.3763,0.3933,0.4101,0.4268,0.4433,0.4597,0.4759,
	0.4920,0.5079,0.5237,0.5392,0.5546,0.5698,0.5848,
	0.5996,0.6142,0.6286,0.6428,0.6568,0.6705,0.6840,
	0.6973,0.7103,0.7232,0.7357,0.7480,0.7601,0.7719,
	0.7835,0.7947,0.8058,0.8165,0.8270,0.8372,0.8471,
	0.8567,0.8660,0.8751,0.8838,0.8923,0.9004,0.9082,
	0.9158,0.9230,0.9299,0.9365,0.9428,0.9488,0.9544,
	0.9597,0.9647,0.9694,0.9737,0.9778,0.9815,0.9848,
	0.9878,0.9905,0.9929,0.9949,0.9966,0.9979,0.9989,
	0.9996,1.0000,1.0000,0.9996,0.9989,0.9979,0.9966,
	0.9949,0.9929,0.9905,0.9878,0.9848,0.9815,0.9778,
	0.9737,0.9694,0.9647,0.9597,0.9544,0.9488,0.9428,
	0.9365,0.9299,0.9230,0.9158,0.9082,0.9004,0.8923,
	0.8838,0.8751,0.8660,0.8567,0.8471,0.8372,0.8270,
	0.8165,0.8058,0.7947,0.7835,0.7719,0.7601,0.7480,
	0.7357,0.7232,0.7103,0.6973,0.6840,0.6705,0.6568,
	0.6428,0.6286,0.6142,0.5996,0.5848,0.5698,0.5546,
	0.5392,0.5237,0.5079,0.4920,0.4759,0.4597,0.4433,
	0.4268,0.4101,0.3933,0.3763,0.3592,0.3420,0.3247,
	0.3073,0.2897,0.2721,0.2544,0.2366,0.2187,0.2007,
	0.1827,0.1646,0.1464,0.1282,0.1100,0.0917,0.0734,
	0.0551,0.0367,0.0184,0.0000,-0.0184,-0.0367,-0.0551,
	-0.0734,-0.0917,-0.1100,-0.1282,-0.1464,-0.1646,-0.1827,
	-0.2007,-0.2187,-0.2366,-0.2544,-0.2721,-0.2897,-0.3073,
	-0.3247,-0.3420,-0.3592,-0.3763,-0.3933,-0.4101,-0.4268,
	-0.4433,-0.4597,-0.4759,-0.4920,-0.5079,-0.5237,-0.5392,
	-0.5546,-0.5698,-0.5848,-0.5996,-0.6142,-0.6286,-0.6428,
	-0.6568,-0.6705,-0.6840,-0.6973,-0.7103,-0.7232,-0.7357,
	-0.7480,-0.7601,-0.7719,-0.7835,-0.7947,-0.8058,-0.8165,
	-0.8270,-0.8372,-0.8471,-0.8567,-0.8660,-0.8751,-0.8838,
	-0.8923,-0.9004,-0.9082,-0.9158,-0.9230,-0.9299,-0.9365,
	-0.9428,-0.9488,-0.9544,-0.9597,-0.9647,-0.9694,-0.9737,
	-0.9778,-0.9815,-0.9848,-0.9878,-0.9905,-0.9929,-0.9949,
	-0.9966,-0.9979,-0.9989,-0.9996,-1.0000,-1.0000,-0.9996,
	-0.9989,-0.9979,-0.9966,-0.9949,-0.9929,-0.9905,-0.9878,
	-0.9848,-0.9815,-0.9778,-0.9737,-0.9694,-0.9647,-0.9597,
	-0.9544,-0.9488,-0.9428,-0.9365,-0.9299,-0.9230,-0.9158,
	-0.9082,-0.9004,-0.8923,-0.8838,-0.8751,-0.8660,-0.8567,
	-0.8471,-0.8372,-0.8270,-0.8165,-0.8058,-0.7947,-0.7835,
	-0.7719,-0.7601,-0.7480,-0.7357,-0.7232,-0.7103,-0.6973,
	-0.6840,-0.6705,-0.6568,-0.6428,-0.6286,-0.6142,-0.5996,
	-0.5848,-0.5698,-0.5546,-0.5392,-0.5237,-0.5079,-0.4920,
	-0.4759,-0.4597,-0.4433,-0.4268,-0.4101,-0.3933,-0.3763,
	-0.3592,-0.3420,-0.3247,-0.3073,-0.2897,-0.2721,-0.2544,
	-0.2366,-0.2187,-0.2007,-0.1827,-0.1646,-0.1464,-0.1282,
	-0.1100,-0.0917,-0.0734,-0.0551,-0.0367,-0.0184,0.0000
	},

	{
	0.0000,-0.0184,-0.0367,-0.0551,-0.0734,-0.0917,-0.1100,
	-0.1282,-0.1464,-0.1646,-0.1827,-0.2007,-0.2187,-0.2366,
	-0.2544,-0.2721,-0.2897,-0.3073,-0.3247,-0.3420,-0.3592,
	-0.3763,-0.3933,-0.4101,-0.4268,-0.4433,-0.4597,-0.4759,
	-0.4920,-0.5079,-0.5237,-0.5392,-0.5546,-0.5698,-0.5848,
	-0.5996,-0.6142,-0.6286,-0.6428,-0.6568,-0.6705,-0.6840,
	-0.6973,-0.7103,-0.7232,-0.7357,-0.7480,-0.7601,-0.7719,
	-0.7835,-0.7947,-0.8058,-0.8165,-0.8270,-0.8372,-0.8471,
	-0.8567,-0.8660,-0.8751,-0.8838,-0.8923,-0.9004,-0.9082,
	-0.9158,-0.9230,-0.9299,-0.9365,-0.9428,-0.9488,-0.9544,
	-0.9597,-0.9647,-0.9694,-0.9737,-0.9778,-0.9815,-0.9848,
	-0.9878,-0.9905,-0.9929,-0.9949,-0.9966,-0.9979,-0.9989,
	-0.9996,-1.0000,-1.0000,-0.9996,-0.9989,-0.9979,-0.9966,
	-0.9949,-0.9929,-0.9905,-0.9878,-0.9848,-0.9815,-0.9778,
	-0.9737,-0.9694,-0.9647,-0.9597,-0.9544,-0.9488,-0.9428,
	-0.9365,-0.9299,-0.9230,-0.9158,-0.9082,-0.9004,-0.8923,
	-0.8838,-0.8751,-0.8660,-0.8567,-0.8471,-0.8372,-0.8270,
	-0.8165,-0.8058,-0.7947,-0.7835,-0.7719,-0.7601,-0.7480,
	-0.7357,-0.7232,-0.7103,-0.6973,-0.6840,-0.6705,-0.6568,
	-0.6428,-0.6286,-0.6142,-0.5996,-0.5848,-0.5698,-0.5546,
	-0.5392,-0.5237,-0.5079,-0.4920,-0.4759,-0.4597,-0.4433,
	-0.4268,-0.4101,-0.3933,-0.3763,-0.3592,-0.3420,-0.3247,
	-0.3073,-0.2897,-0.2721,-0.2544,-0.2366,-0.2187,-0.2007,
	-0.1827,-0.1646,-0.1464,-0.1282,-0.1100,-0.0917,-0.0734,
	-0.0551,-0.0367,-0.0184,0.0000,0.0184,0.0367,0.0551,
	0.0734,0.0917,0.1100,0.1282,0.1464,0.1646,0.1827,
	0.2007,0.2187,0.2366,0.2544,0.2721,0.2897,0.3073,
	0.3247,0.3420,0.3592,0.3763,0.3933,0.4101,0.4268,
	0.4433,0.4597,0.4759,0.4920,0.5079,0.5237,0.5392,
	0.5546,0.5698,0.5848,0.5996,0.6142,0.6286,0.6428,
	0.6568,0.6705,0.6840,0.6973,0.7103,0.7232,0.7357,
	0.7480,0.7601,0.7719,0.7835,0.7947,0.8058,0.8165,
	0.8270,0.8372,0.8471,0.8567,0.8660,0.8751,0.8838,
	0.8923,0.9004,0.9082,0.9158,0.9230,0.9299,0.9365,
	0.9428,0.9488,0.9544,0.9597,0.9647,0.9694,0.9737,
	0.9778,0.9815,0.9848,0.9878,0.9905,0.9929,0.9949,
	0.9966,0.9979,0.9989,0.9996,1.0000,1.0000,0.9996,
	0.9989,0.9979,0.9966,0.9949,0.9929,0.9905,0.9878,
	0.9848,0.9815,0.9778,0.9737,0.9694,0.9647,0.9597,
	0.9544,0.9488,0.9428,0.9365,0.9299,0.9230,0.9158,
	0.9082,0.9004,0.8923,0.8838,0.8751,0.8660,0.8567,
	0.8471,0.8372,0.8270,0.8165,0.8058,0.7947,0.7835,
	0.7719,0.7601,0.7480,0.7357,0.7232,0.7103,0.6973,
	0.6840,0.6705,0.6568,0.6428,0.6286,0.6142,0.5996,
	0.5848,0.5698,0.5546,0.5392,0.5237,0.5079,0.4920,
	0.4759,0.4597,0.4433,0.4268,0.4101,0.3933,0.3763,
	0.3592,0.3420,0.3247,0.3073,0.2897,0.2721,0.2544,
	0.2366,0.2187,0.2007,0.1827,0.1646,0.1464,0.1282,
	0.1100,0.0917,0.0734,0.0551,0.0367,0.0184,0.0000
	},

	{
	1.0000,0.9998,0.9993,0.9985,0.9973,0.9958,0.9939,
	0.9917,0.9892,0.9864,0.9832,0.9796,0.9758,0.9716,
	0.9671,0.9623,0.9571,0.9516,0.9458,0.9397,0.9333,
	0.9265,0.9194,0.9120,0.9044,0.8964,0.8881,0.8795,
	0.8706,0.8614,0.8519,0.8422,0.8321,0.8218,0.8112,
	0.8003,0.7891,0.7777,0.7660,0.7541,0.7419,0.7295,
	0.7168,0.7039,0.6907,0.6773,0.6637,0.6498,0.6357,
	0.6214,0.6069,0.5922,0.5773,0.5622,0.5469,0.5315,
	0.5158,0.5000,0.4840,0.4678,0.4515,0.4351,0.4185,
	0.4017,0.3848,0.3678,0.3506,0.3334,0.3160,0.2985,
	0.2809,0.2633,0.2455,0.2276,0.2097,0.1917,0.1736,
	0.1555,0.1374,0.1191,0.1009,0.0826,0.0643,0.0459,
	0.0276,0.0092,-0.0092,-0.0276,-0.0459,-0.0643,-0.0826,
	-0.1009,-0.1191,-0.1374,-0.1555,-0.1736,-0.1917,-0.2097,
	-0.2276,-0.2455,-0.2633,-0.2809,-0.2985,-0.3160,-0.3334,
	-0.3506,-0.3678,-0.3848,-0.4017,-0.4185,-0.4351,-0.4515,
	-0.4678,-0.4840,-0.5000,-0.5158,-0.5315,-0.5469,-0.5622,
	-0.5773,-0.5922,-0.6069,-0.6214,-0.6357,-0.6498,-0.6637,
	-0.6773,-0.6907,-0.7039,-0.7168,-0.7295,-0.7419,-0.7541,
	-0.7660,-0.7777,-0.7891,-0.8003,-0.8112,-0.8218,-0.8321,
	-0.8422,-0.8519,-0.8614,-0.8706,-0.8795,-0.8881,-0.8964,
	-0.9044,-0.9120,-0.9194,-0.9265,-0.9333,-0.9397,-0.9458,
	-0.9516,-0.9571,-0.9623,-0.9671,-0.9716,-0.9758,-0.9796,
	-0.9832,-0.9864,-0.9892,-0.9917,-0.9939,-0.9958,-0.9973,
	-0.9985,-0.9993,-0.9998,-1.0000,-0.9998,-0.9993,-0.9985,
	-0.9973,-0.9958,-0.9939,-0.9917,-0.9892,-0.9864,-0.9832,
	-0.9796,-0.9758,-0.9716,-0.9671,-0.9623,-0.9571,-0.9516,
	-0.9458,-0.9397,-0.9333,-0.9265,-0.9194,-0.9120,-0.9044,
	-0.8964,-0.8881,-0.8795,-0.8706,-0.8614,-0.8519,-0.8422,
	-0.8321,-0.8218,-0.8112,-0.8003,-0.7891,-0.7777,-0.7660,
	-0.7541,-0.7419,-0.7295,-0.7168,-0.7039,-0.6907,-0.6773,
	-0.6637,-0.6498,-0.6357,-0.6214,-0.6069,-0.5922,-0.5773,
	-0.5622,-0.5469,-0.5315,-0.5158,-0.5000,-0.4840,-0.4678,
	-0.4515,-0.4351,-0.4185,-0.4017,-0.3848,-0.3678,-0.3506,
	-0.3334,-0.3160,-0.2985,-0.2809,-0.2633,-0.2455,-0.2276,
	-0.2097,-0.1917,-0.1736,-0.1555,-0.1374,-0.1191,-0.1009,
	-0.0826,-0.0643,-0.0459,-0.0276,-0.0092,0.0092,0.0276,
	0.0459,0.0643,0.0826,0.1009,0.1191,0.1374,0.1555,
	0.1736,0.1917,0.2097,0.2276,0.2455,0.2633,0.2809,
	0.2985,0.3160,0.3334,0.3506,0.3678,0.3848,0.4017,
	0.4185,0.4351,0.4515,0.4678,0.4840,0.5000,0.5158,
	0.5315,0.5469,0.5622,0.5773,0.5922,0.6069,0.6214,
	0.6357,0.6498,0.6637,0.6773,0.6907,0.7039,0.7168,
	0.7295,0.7419,0.7541,0.7660,0.7777,0.7891,0.8003,
	0.8112,0.8218,0.8321,0.8422,0.8519,0.8614,0.8706,
	0.8795,0.8881,0.8964,0.9044,0.9120,0.9194,0.9265,
	0.9333,0.9397,0.9458,0.9516,0.9571,0.9623,0.9671,
	0.9716,0.9758,0.9796,0.9832,0.9864,0.9892,0.9917,
	0.9939,0.9958,0.9973,0.9985,0.9993,0.9998,1.0000
	},

	{
	-1.0000,-0.9998,-0.9993,-0.9985,-0.9973,-0.9958,-0.9939,
	-0.9917,-0.9892,-0.9864,-0.9832,-0.9796,-0.9758,-0.9716,
	-0.9671,-0.9623,-0.9571,-0.9516,-0.9458,-0.9397,-0.9333,
	-0.9265,-0.9194,-0.9120,-0.9044,-0.8964,-0.8881,-0.8795,
	-0.8706,-0.8614,-0.8519,-0.8422,-0.8321,-0.8218,-0.8112,
	-0.8003,-0.7891,-0.7777,-0.7660,-0.7541,-0.7419,-0.7295,
	-0.7168,-0.7039,-0.6907,-0.6773,-0.6637,-0.6498,-0.6357,
	-0.6214,-0.6069,-0.5922,-0.5773,-0.5622,-0.5469,-0.5315,
	-0.5158,-0.5000,-0.4840,-0.4678,-0.4515,-0.4351,-0.4185,
	-0.4017,-0.3848,-0.3678,-0.3506,-0.3334,-0.3160,-0.2985,
	-0.2809,-0.2633,-0.2455,-0.2276,-0.2097,-0.1917,-0.1736,
	-0.1555,-0.1374,-0.1191,-0.1009,-0.0826,-0.0643,-0.0459,
	-0.0276,-0.0092,0.0092,0.0276,0.0459,0.0643,0.0826,
	0.1009,0.1191,0.1374,0.1555,0.1736,0.1917,0.2097,
	0.2276,0.2455,0.2633,0.2809,0.2985,0.3160,0.3334,
	0.3506,0.3678,0.3848,0.4017,0.4185,0.4351,0.4515,
	0.4678,0.4840,0.5000,0.5158,0.5315,0.5469,0.5622,
	0.5773,0.5922,0.6069,0.6214,0.6357,0.6498,0.6637,
	0.6773,0.6907,0.7039,0.7168,0.7295,0.7419,0.7541,
	0.7660,0.7777,0.7891,0.8003,0.8112,0.8218,0.8321,
	0.8422,0.8519,0.8614,0.8706,0.8795,0.8881,0.8964,
	0.9044,0.9120,0.9194,0.9265,0.9333,0.9397,0.9458,
	0.9516,0.9571,0.9623,0.9671,0.9716,0.9758,0.9796,
	0.9832,0.9864,0.9892,0.9917,0.9939,0.9958,0.9973,
	0.9985,0.9993,0.9998,1.0000,0.9998,0.9993,0.9985,
	0.9973,0.9958,0.9939,0.9917,0.9892,0.9864,0.9832,
	0.9796,0.9758,0.9716,0.9671,0.9623,0.9571,0.9516,
	0.9458,0.9397,0.9333,0.9265,0.9194,0.9120,0.9044,
	0.8964,0.8881,0.8795,0.8706,0.8614,0.8519,0.8422,
	0.8321,0.8218,0.8112,0.8003,0.7891,0.7777,0.7660,
	0.7541,0.7419,0.7295,0.7168,0.7039,0.6907,0.6773,
	0.6637,0.6498,0.6357,0.6214,0.6069,0.5922,0.5773,
	0.5622,0.5469,0.5315,0.5158,0.5000,0.4840,0.4678,
	0.4515,0.4351,0.4185,0.4017,0.3848,0.3678,0.3506,
	0.3334,0.3160,0.2985,0.2809,0.2633,0.2455,0.2276,
	0.2097,0.1917,0.1736,0.1555,0.1374,0.1191,0.1009,
	0.0826,0.0643,0.0459,0.0276,0.0092,-0.0092,-0.0276,
	-0.0459,-0.0643,-0.0826,-0.1009,-0.1191,-0.1374,-0.1555,
	-0.1736,-0.1917,-0.2097,-0.2276,-0.2455,-0.2633,-0.2809,
	-0.2985,-0.3160,-0.3334,-0.3506,-0.3678,-0.3848,-0.4017,
	-0.4185,-0.4351,-0.4515,-0.4678,-0.4840,-0.5000,-0.5158,
	-0.5315,-0.5469,-0.5622,-0.5773,-0.5922,-0.6069,-0.6214,
	-0.6357,-0.6498,-0.6637,-0.6773,-0.6907,-0.7039,-0.7168,
	-0.7295,-0.7419,-0.7541,-0.7660,-0.7777,-0.7891,-0.8003,
	-0.8112,-0.8218,-0.8321,-0.8422,-0.8519,-0.8614,-0.8706,
	-0.8795,-0.8881,-0.8964,-0.9044,-0.9120,-0.9194,-0.9265,
	-0.9333,-0.9397,-0.9458,-0.9516,-0.9571,-0.9623,-0.9671,
	-0.9716,-0.9758,-0.9796,-0.9832,-0.9864,-0.9892,-0.9917,
	-0.9939,-0.9958,-0.9973,-0.9985,-0.9993,-0.9998,-1.0000
	},

	{
	0.0000,0.0367,0.0734,0.1100,0.1464,0.1827,0.2187,
	0.2544,0.2897,0.3247,0.3592,0.3933,0.4268,0.4597,
	0.4920,0.5237,0.5546,0.5848,0.6142,0.6428,0.6705,
	0.6973,0.7232,0.7480,0.7719,0.7947,0.8165,0.8372,
	0.8567,0.8751,0.8923,0.9082,0.9230,0.9365,0.9488,
	0.9597,0.9694,0.9778,0.9848,0.9905,0.9949,0.9979,
	0.9996,1.0000,0.9989,0.9966,0.9929,0.9878,0.9815,
	0.9737,0.9647,0.9544,0.9428,0.9299,0.9158,0.9004,
	0.8838,0.8660,0.8471,0.8270,0.8058,0.7835,0.7601,
	0.7357,0.7103,0.6840,0.6568,0.6286,0.5996,0.5698,
	0.5392,0.5079,0.4759,0.4433,0.4101,0.3763,0.3420,
	0.3073,0.2721,0.2366,0.2007,0.1646,0.1282,0.0917,
	0.0551,0.0184,-0.0184,-0.0551,-0.0917,-0.1282,-0.1646,
	-0.2007,-0.2366,-0.2721,-0.3073,-0.3420,-0.3763,-0.4101,
	-0.4433,-0.4759,-0.5079,-0.5392,-0.5698,-0.5996,-0.6286,
	-0.6568,-0.6840,-0.7103,-0.7357,-0.7601,-0.7835,-0.8058,
	-0.8270,-0.8471,-0.8660,-0.8838,-0.9004,-0.9158,-0.9299,
	-0.9428,-0.9544,-0.9647,-0.9737,-0.9815,-0.9878,-0.9929,
	-0.9966,-0.9989,-1.0000,-0.9996,-0.9979,-0.9949,-0.9905,
	-0.9848,-0.9778,-0.9694,-0.9597,-0.9488,-0.9365,-0.9230,
	-0.9082,-0.8923,-0.8751,-0.8567,-0.8372,-0.8165,-0.7947,
	-0.7719,-0.7480,-0.7232,-0.6973,-0.6705,-0.6428,-0.6142,
	-0.5848,-0.5546,-0.5237,-0.4920,-0.4597,-0.4268,-0.3933,
	-0.3592,-0.3247,-0.2897,-0.2544,-0.2187,-0.1827,-0.1464,
	-0.1100,-0.0734,-0.0367,0.0000,0.0367,0.0734,0.1100,
	0.1464,0.1827,0.2187,0.2544,0.2897,0.3247,0.3592,
	0.3933,0.4268,0.4597,0.4920,0.5237,0.5546,0.5848,
	0.6142,0.6428,0.6705,0.6973,0.7232,0.7480,0.7719,
	0.7947,0.8165,0.8372,0.8567,0.8751,0.8923,0.9082,
	0.9230,0.9365,0.9488,0.9597,0.9694,0.9778,0.9848,
	0.9905,0.9949,0.9979,0.9996,1.0000,0.9989,0.9966,
	0.9929,0.9878,0.9815,0.9737,0.9647,0.9544,0.9428,
	0.9299,0.9158,0.9004,0.8838,0.8660,0.8471,0.8270,
	0.8058,0.7835,0.7601,0.7357,0.7103,0.6840,0.6568,
	0.6286,0.5996,0.5698,0.5392,0.5079,0.4759,0.4433,
	0.4101,0.3763,0.3420,0.3073,0.2721,0.2366,0.2007,
	0.1646,0.1282,0.0917,0.0551,0.0184,-0.0184,-0.0551,
	-0.0917,-0.1282,-0.1646,-0.2007,-0.2366,-0.2721,-0.3073,
	-0.3420,-0.3763,-0.4101,-0.4433,-0.4759,-0.5079,-0.5392,
	-0.5698,-0.5996,-0.6286,-0.6568,-0.6840,-0.7103,-0.7357,
	-0.7601,-0.7835,-0.8058,-0.8270,-0.8471,-0.8660,-0.8838,
	-0.9004,-0.9158,-0.9299,-0.9428,-0.9544,-0.9647,-0.9737,
	-0.9815,-0.9878,-0.9929,-0.9966,-0.9989,-1.0000,-0.9996,
	-0.9979,-0.9949,-0.9905,-0.9848,-0.9778,-0.9694,-0.9597,
	-0.9488,-0.9365,-0.9230,-0.9082,-0.8923,-0.8751,-0.8567,
	-0.8372,-0.8165,-0.7947,-0.7719,-0.7480,-0.7232,-0.6973,
	-0.6705,-0.6428,-0.6142,-0.5848,-0.5546,-0.5237,-0.4920,
	-0.4597,-0.4268,-0.3933,-0.3592,-0.3247,-0.2897,-0.2544,
	-0.2187,-0.1827,-0.1464,-0.1100,-0.0734,-0.0367,0.0000,
	},

	{
	0.0000,-0.0367,-0.0734,-0.1100,-0.1464,-0.1827,-0.2187,
	-0.2544,-0.2897,-0.3247,-0.3592,-0.3933,-0.4268,-0.4597,
	-0.4920,-0.5237,-0.5546,-0.5848,-0.6142,-0.6428,-0.6705,
	-0.6973,-0.7232,-0.7480,-0.7719,-0.7947,-0.8165,-0.8372,
	-0.8567,-0.8751,-0.8923,-0.9082,-0.9230,-0.9365,-0.9488,
	-0.9597,-0.9694,-0.9778,-0.9848,-0.9905,-0.9949,-0.9979,
	-0.9996,-1.0000,-0.9989,-0.9966,-0.9929,-0.9878,-0.9815,
	-0.9737,-0.9647,-0.9544,-0.9428,-0.9299,-0.9158,-0.9004,
	-0.8838,-0.8660,-0.8471,-0.8270,-0.8058,-0.7835,-0.7601,
	-0.7357,-0.7103,-0.6840,-0.6568,-0.6286,-0.5996,-0.5698,
	-0.5392,-0.5079,-0.4759,-0.4433,-0.4101,-0.3763,-0.3420,
	-0.3073,-0.2721,-0.2366,-0.2007,-0.1646,-0.1282,-0.0917,
	-0.0551,-0.0184,0.0184,0.0551,0.0917,0.1282,0.1646,
	0.2007,0.2366,0.2721,0.3073,0.3420,0.3763,0.4101,
	0.4433,0.4759,0.5079,0.5392,0.5698,0.5996,0.6286,
	0.6568,0.6840,0.7103,0.7357,0.7601,0.7835,0.8058,
	0.8270,0.8471,0.8660,0.8838,0.9004,0.9158,0.9299,
	0.9428,0.9544,0.9647,0.9737,0.9815,0.9878,0.9929,
	0.9966,0.9989,1.0000,0.9996,0.9979,0.9949,0.9905,
	0.9848,0.9778,0.9694,0.9597,0.9488,0.9365,0.9230,
	0.9082,0.8923,0.8751,0.8567,0.8372,0.8165,0.7947,
	0.7719,0.7480,0.7232,0.6973,0.6705,0.6428,0.6142,
	0.5848,0.5546,0.5237,0.4920,0.4597,0.4268,0.3933,
	0.3592,0.3247,0.2897,0.2544,0.2187,0.1827,0.1464,
	0.1100,0.0734,0.0367,0.0000,-0.0367,-0.0734,-0.1100,
	-0.1464,-0.1827,-0.2187,-0.2544,-0.2897,-0.3247,-0.3592,
	-0.3933,-0.4268,-0.4597,-0.4920,-0.5237,-0.5546,-0.5848,
	-0.6142,-0.6428,-0.6705,-0.6973,-0.7232,-0.7480,-0.7719,
	-0.7947,-0.8165,-0.8372,-0.8567,-0.8751,-0.8923,-0.9082,
	-0.9230,-0.9365,-0.9488,-0.9597,-0.9694,-0.9778,-0.9848,
	-0.9905,-0.9949,-0.9979,-0.9996,-1.0000,-0.9989,-0.9966,
	-0.9929,-0.9878,-0.9815,-0.9737,-0.9647,-0.9544,-0.9428,
	-0.9299,-0.9158,-0.9004,-0.8838,-0.8660,-0.8471,-0.8270,
	-0.8058,-0.7835,-0.7601,-0.7357,-0.7103,-0.6840,-0.6568,
	-0.6286,-0.5996,-0.5698,-0.5392,-0.5079,-0.4759,-0.4433,
	-0.4101,-0.3763,-0.3420,-0.3073,-0.2721,-0.2366,-0.2007,
	-0.1646,-0.1282,-0.0917,-0.0551,-0.0184,0.0184,0.0551,
	0.0917,0.1282,0.1646,0.2007,0.2366,0.2721,0.3073,
	0.3420,0.3763,0.4101,0.4433,0.4759,0.5079,0.5392,
	0.5698,0.5996,0.6286,0.6568,0.6840,0.7103,0.7357,
	0.7601,0.7835,0.8058,0.8270,0.8471,0.8660,0.8838,
	0.9004,0.9158,0.9299,0.9428,0.9544,0.9647,0.9737,
	0.9815,0.9878,0.9929,0.9966,0.9989,1.0000,0.9996,
	0.9979,0.9949,0.9905,0.9848,0.9778,0.9694,0.9597,
	0.9488,0.9365,0.9230,0.9082,0.8923,0.8751,0.8567,
	0.8372,0.8165,0.7947,0.7719,0.7480,0.7232,0.6973,
	0.6705,0.6428,0.6142,0.5848,0.5546,0.5237,0.4920,
	0.4597,0.4268,0.3933,0.3592,0.3247,0.2897,0.2544,
	0.2187,0.1827,0.1464,0.1100,0.0734,0.0367,0.0000
	},

	{
	1.0000,0.9993,0.9973,0.9939,0.9892,0.9832,0.9758,
	0.9671,0.9571,0.9458,0.9333,0.9194,0.9044,0.8881,
	0.8706,0.8519,0.8321,0.8112,0.7891,0.7660,0.7419,
	0.7168,0.6907,0.6637,0.6357,0.6069,0.5773,0.5469,
	0.5158,0.4840,0.4515,0.4185,0.3848,0.3506,0.3160,
	0.2809,0.2455,0.2097,0.1736,0.1374,0.1009,0.0643,
	0.0276,-0.0092,-0.0459,-0.0826,-0.1191,-0.1555,-0.1917,
	-0.2276,-0.2633,-0.2985,-0.3334,-0.3678,-0.4017,-0.4351,
	-0.4678,-0.5000,-0.5315,-0.5622,-0.5922,-0.6214,-0.6498,
	-0.6773,-0.7039,-0.7295,-0.7541,-0.7777,-0.8003,-0.8218,
	-0.8422,-0.8614,-0.8795,-0.8964,-0.9120,-0.9265,-0.9397,
	-0.9516,-0.9623,-0.9716,-0.9796,-0.9864,-0.9917,-0.9958,
	-0.9985,-0.9998,-0.9998,-0.9985,-0.9958,-0.9917,-0.9864,
	-0.9796,-0.9716,-0.9623,-0.9516,-0.9397,-0.9265,-0.9120,
	-0.8964,-0.8795,-0.8614,-0.8422,-0.8218,-0.8003,-0.7777,
	-0.7541,-0.7295,-0.7039,-0.6773,-0.6498,-0.6214,-0.5922,
	-0.5622,-0.5315,-0.5000,-0.4678,-0.4351,-0.4017,-0.3678,
	-0.3334,-0.2985,-0.2633,-0.2276,-0.1917,-0.1555,-0.1191,
	-0.0826,-0.0459,-0.0092,0.0276,0.0643,0.1009,0.1374,
	0.1736,0.2097,0.2455,0.2809,0.3160,0.3506,0.3848,
	0.4185,0.4515,0.4840,0.5158,0.5469,0.5773,0.6069,
	0.6357,0.6637,0.6907,0.7168,0.7419,0.7660,0.7891,
	0.8112,0.8321,0.8519,0.8706,0.8881,0.9044,0.9194,
	0.9333,0.9458,0.9571,0.9671,0.9758,0.9832,0.9892,
	0.9939,0.9973,0.9993,1.0000,0.9993,0.9973,0.9939,
	0.9892,0.9832,0.9758,0.9671,0.9571,0.9458,0.9333,
	0.9194,0.9044,0.8881,0.8706,0.8519,0.8321,0.8112,
	0.7891,0.7660,0.7419,0.7168,0.6907,0.6637,0.6357,
	0.6069,0.5773,0.5469,0.5158,0.4840,0.4515,0.4185,
	0.3848,0.3506,0.3160,0.2809,0.2455,0.2097,0.1736,
	0.1374,0.1009,0.0643,0.0276,-0.0092,-0.0459,-0.0826,
	-0.1191,-0.1555,-0.1917,-0.2276,-0.2633,-0.2985,-0.3334,
	-0.3678,-0.4017,-0.4351,-0.4678,-0.5000,-0.5315,-0.5622,
	-0.5922,-0.6214,-0.6498,-0.6773,-0.7039,-0.7295,-0.7541,
	-0.7777,-0.8003,-0.8218,-0.8422,-0.8614,-0.8795,-0.8964,
	-0.9120,-0.9265,-0.9397,-0.9516,-0.9623,-0.9716,-0.9796,
	-0.9864,-0.9917,-0.9958,-0.9985,-0.9998,-0.9998,-0.9985,
	-0.9958,-0.9917,-0.9864,-0.9796,-0.9716,-0.9623,-0.9516,
	-0.9397,-0.9265,-0.9120,-0.8964,-0.8795,-0.8614,-0.8422,
	-0.8218,-0.8003,-0.7777,-0.7541,-0.7295,-0.7039,-0.6773,
	-0.6498,-0.6214,-0.5922,-0.5622,-0.5315,-0.5000,-0.4678,
	-0.4351,-0.4017,-0.3678,-0.3334,-0.2985,-0.2633,-0.2276,
	-0.1917,-0.1555,-0.1191,-0.0826,-0.0459,-0.0092,0.0276,
	0.0643,0.1009,0.1374,0.1736,0.2097,0.2455,0.2809,
	0.3160,0.3506,0.3848,0.4185,0.4515,0.4840,0.5158,
	0.5469,0.5773,0.6069,0.6357,0.6637,0.6907,0.7168,
	0.7419,0.7660,0.7891,0.8112,0.8321,0.8519,0.8706,
	0.8881,0.9044,0.9194,0.9333,0.9458,0.9571,0.9671,
	0.9758,0.9832,0.9892,0.9939,0.9973,0.9993,1.0000
	},

	{
	-1.0000,-0.9993,-0.9973,-0.9939,-0.9892,-0.9832,-0.9758,
	-0.9671,-0.9571,-0.9458,-0.9333,-0.9194,-0.9044,-0.8881,
	-0.8706,-0.8519,-0.8321,-0.8112,-0.7891,-0.7660,-0.7419,
	-0.7168,-0.6907,-0.6637,-0.6357,-0.6069,-0.5773,-0.5469,
	-0.5158,-0.4840,-0.4515,-0.4185,-0.3848,-0.3506,-0.3160,
	-0.2809,-0.2455,-0.2097,-0.1736,-0.1374,-0.1009,-0.0643,
	-0.0276,0.0092,0.0459,0.0826,0.1191,0.1555,0.1917,
	0.2276,0.2633,0.2985,0.3334,0.3678,0.4017,0.4351,
	0.4678,0.5000,0.5315,0.5622,0.5922,0.6214,0.6498,
	0.6773,0.7039,0.7295,0.7541,0.7777,0.8003,0.8218,
	0.8422,0.8614,0.8795,0.8964,0.9120,0.9265,0.9397,
	0.9516,0.9623,0.9716,0.9796,0.9864,0.9917,0.9958,
	0.9985,0.9998,0.9998,0.9985,0.9958,0.9917,0.9864,
	0.9796,0.9716,0.9623,0.9516,0.9397,0.9265,0.9120,
	0.8964,0.8795,0.8614,0.8422,0.8218,0.8003,0.7777,
	0.7541,0.7295,0.7039,0.6773,0.6498,0.6214,0.5922,
	0.5622,0.5315,0.5000,0.4678,0.4351,0.4017,0.3678,
	0.3334,0.2985,0.2633,0.2276,0.1917,0.1555,0.1191,
	0.0826,0.0459,0.0092,-0.0276,-0.0643,-0.1009,-0.1374,
	-0.1736,-0.2097,-0.2455,-0.2809,-0.3160,-0.3506,-0.3848,
	-0.4185,-0.4515,-0.4840,-0.5158,-0.5469,-0.5773,-0.6069,
	-0.6357,-0.6637,-0.6907,-0.7168,-0.7419,-0.7660,-0.7891,
	-0.8112,-0.8321,-0.8519,-0.8706,-0.8881,-0.9044,-0.9194,
	-0.9333,-0.9458,-0.9571,-0.9671,-0.9758,-0.9832,-0.9892,
	-0.9939,-0.9973,-0.9993,-1.0000,-0.9993,-0.9973,-0.9939,
	-0.9892,-0.9832,-0.9758,-0.9671,-0.9571,-0.9458,-0.9333,
	-0.9194,-0.9044,-0.8881,-0.8706,-0.8519,-0.8321,-0.8112,
	-0.7891,-0.7660,-0.7419,-0.7168,-0.6907,-0.6637,-0.6357,
	-0.6069,-0.5773,-0.5469,-0.5158,-0.4840,-0.4515,-0.4185,
	-0.3848,-0.3506,-0.3160,-0.2809,-0.2455,-0.2097,-0.1736,
	-0.1374,-0.1009,-0.0643,-0.0276,0.0092,0.0459,0.0826,
	0.1191,0.1555,0.1917,0.2276,0.2633,0.2985,0.3334,
	0.3678,0.4017,0.4351,0.4678,0.5000,0.5315,0.5622,
	0.5922,0.6214,0.6498,0.6773,0.7039,0.7295,0.7541,
	0.7777,0.8003,0.8218,0.8422,0.8614,0.8795,0.8964,
	0.9120,0.9265,0.9397,0.9516,0.9623,0.9716,0.9796,
	0.9864,0.9917,0.9958,0.9985,0.9998,0.9998,0.9985,
	0.9958,0.9917,0.9864,0.9796,0.9716,0.9623,0.9516,
	0.9397,0.9265,0.9120,0.8964,0.8795,0.8614,0.8422,
	0.8218,0.8003,0.7777,0.7541,0.7295,0.7039,0.6773,
	0.6498,0.6214,0.5922,0.5622,0.5315,0.5000,0.4678,
	0.4351,0.4017,0.3678,0.3334,0.2985,0.2633,0.2276,
	0.1917,0.1555,0.1191,0.0826,0.0459,0.0092,-0.0276,
	-0.0643,-0.1009,-0.1374,-0.1736,-0.2097,-0.2455,-0.2809,
	-0.3160,-0.3506,-0.3848,-0.4185,-0.4515,-0.4840,-0.5158,
	-0.5469,-0.5773,-0.6069,-0.6357,-0.6637,-0.6907,-0.7168,
	-0.7419,-0.7660,-0.7891,-0.8112,-0.8321,-0.8519,-0.8706,
	-0.8881,-0.9044,-0.9194,-0.9333,-0.9458,-0.9571,-0.9671,
	-0.9758,-0.9832,-0.9892,-0.9939,-0.9973,-0.9993,-1.0000
	},

	{
	0.0000,0.0551,0.1100,0.1646,0.2187,0.2721,0.3247,
	0.3763,0.4268,0.4759,0.5237,0.5698,0.6142,0.6568,
	0.6973,0.7357,0.7719,0.8058,0.8372,0.8660,0.8923,
	0.9158,0.9365,0.9544,0.9694,0.9815,0.9905,0.9966,
	0.9996,0.9996,0.9966,0.9905,0.9815,0.9694,0.9544,
	0.9365,0.9158,0.8923,0.8660,0.8372,0.8058,0.7719,
	0.7357,0.6973,0.6568,0.6142,0.5698,0.5237,0.4759,
	0.4268,0.3763,0.3247,0.2721,0.2187,0.1646,0.1100,
	0.0551,0.0000,-0.0551,-0.1100,-0.1646,-0.2187,-0.2721,
	-0.3247,-0.3763,-0.4268,-0.4759,-0.5237,-0.5698,-0.6142,
	-0.6568,-0.6973,-0.7357,-0.7719,-0.8058,-0.8372,-0.8660,
	-0.8923,-0.9158,-0.9365,-0.9544,-0.9694,-0.9815,-0.9905,
	-0.9966,-0.9996,-0.9996,-0.9966,-0.9905,-0.9815,-0.9694,
	-0.9544,-0.9365,-0.9158,-0.8923,-0.8660,-0.8372,-0.8058,
	-0.7719,-0.7357,-0.6973,-0.6568,-0.6142,-0.5698,-0.5237,
	-0.4759,-0.4268,-0.3763,-0.3247,-0.2721,-0.2187,-0.1646,
	-0.1100,-0.0551,0.0000,0.0551,0.1100,0.1646,0.2187,
	0.2721,0.3247,0.3763,0.4268,0.4759,0.5237,0.5698,
	0.6142,0.6568,0.6973,0.7357,0.7719,0.8058,0.8372,
	0.8660,0.8923,0.9158,0.9365,0.9544,0.9694,0.9815,
	0.9905,0.9966,0.9996,0.9996,0.9966,0.9905,0.9815,
	0.9694,0.9544,0.9365,0.9158,0.8923,0.8660,0.8372,
	0.8058,0.7719,0.7357,0.6973,0.6568,0.6142,0.5698,
	0.5237,0.4759,0.4268,0.3763,0.3247,0.2721,0.2187,
	0.1646,0.1100,0.0551,0.0000,-0.0551,-0.1100,-0.1646,
	-0.2187,-0.2721,-0.3247,-0.3763,-0.4268,-0.4759,-0.5237,
	-0.5698,-0.6142,-0.6568,-0.6973,-0.7357,-0.7719,-0.8058,
	-0.8372,-0.8660,-0.8923,-0.9158,-0.9365,-0.9544,-0.9694,
	-0.9815,-0.9905,-0.9966,-0.9996,-0.9996,-0.9966,-0.9905,
	-0.9815,-0.9694,-0.9544,-0.9365,-0.9158,-0.8923,-0.8660,
	-0.8372,-0.8058,-0.7719,-0.7357,-0.6973,-0.6568,-0.6142,
	-0.5698,-0.5237,-0.4759,-0.4268,-0.3763,-0.3247,-0.2721,
	-0.2187,-0.1646,-0.1100,-0.0551,0.0000,0.0551,0.1100,
	0.1646,0.2187,0.2721,0.3247,0.3763,0.4268,0.4759,
	0.5237,0.5698,0.6142,0.6568,0.6973,0.7357,0.7719,
	0.8058,0.8372,0.8660,0.8923,0.9158,0.9365,0.9544,
	0.9694,0.9815,0.9905,0.9966,0.9996,0.9996,0.9966,
	0.9905,0.9815,0.9694,0.9544,0.9365,0.9158,0.8923,
	0.8660,0.8372,0.8058,0.7719,0.7357,0.6973,0.6568,
	0.6142,0.5698,0.5237,0.4759,0.4268,0.3763,0.3247,
	0.2721,0.2187,0.1646,0.1100,0.0551,0.0000,-0.0551,
	-0.1100,-0.1646,-0.2187,-0.2721,-0.3247,-0.3763,-0.4268,
	-0.4759,-0.5237,-0.5698,-0.6142,-0.6568,-0.6973,-0.7357,
	-0.7719,-0.8058,-0.8372,-0.8660,-0.8923,-0.9158,-0.9365,
	-0.9544,-0.9694,-0.9815,-0.9905,-0.9966,-0.9996,-0.9996,
	-0.9966,-0.9905,-0.9815,-0.9694,-0.9544,-0.9365,-0.9158,
	-0.8923,-0.8660,-0.8372,-0.8058,-0.7719,-0.7357,-0.6973,
	-0.6568,-0.6142,-0.5698,-0.5237,-0.4759,-0.4268,-0.3763,
	-0.3247,-0.2721,-0.2187,-0.1646,-0.1100,-0.0551,0.0000
	},

	{
	0.0000,-0.0551,-0.1100,-0.1646,-0.2187,-0.2721,-0.3247,
	-0.3763,-0.4268,-0.4759,-0.5237,-0.5698,-0.6142,-0.6568,
	-0.6973,-0.7357,-0.7719,-0.8058,-0.8372,-0.8660,-0.8923,
	-0.9158,-0.9365,-0.9544,-0.9694,-0.9815,-0.9905,-0.9966,
	-0.9996,-0.9996,-0.9966,-0.9905,-0.9815,-0.9694,-0.9544,
	-0.9365,-0.9158,-0.8923,-0.8660,-0.8372,-0.8058,-0.7719,
	-0.7357,-0.6973,-0.6568,-0.6142,-0.5698,-0.5237,-0.4759,
	-0.4268,-0.3763,-0.3247,-0.2721,-0.2187,-0.1646,-0.1100,
	-0.0551,0.0000,0.0551,0.1100,0.1646,0.2187,0.2721,
	0.3247,0.3763,0.4268,0.4759,0.5237,0.5698,0.6142,
	0.6568,0.6973,0.7357,0.7719,0.8058,0.8372,0.8660,
	0.8923,0.9158,0.9365,0.9544,0.9694,0.9815,0.9905,
	0.9966,0.9996,0.9996,0.9966,0.9905,0.9815,0.9694,
	0.9544,0.9365,0.9158,0.8923,0.8660,0.8372,0.8058,
	0.7719,0.7357,0.6973,0.6568,0.6142,0.5698,0.5237,
	0.4759,0.4268,0.3763,0.3247,0.2721,0.2187,0.1646,
	0.1100,0.0551,0.0000,-0.0551,-0.1100,-0.1646,-0.2187,
	-0.2721,-0.3247,-0.3763,-0.4268,-0.4759,-0.5237,-0.5698,
	-0.6142,-0.6568,-0.6973,-0.7357,-0.7719,-0.8058,-0.8372,
	-0.8660,-0.8923,-0.9158,-0.9365,-0.9544,-0.9694,-0.9815,
	-0.9905,-0.9966,-0.9996,-0.9996,-0.9966,-0.9905,-0.9815,
	-0.9694,-0.9544,-0.9365,-0.9158,-0.8923,-0.8660,-0.8372,
	-0.8058,-0.7719,-0.7357,-0.6973,-0.6568,-0.6142,-0.5698,
	-0.5237,-0.4759,-0.4268,-0.3763,-0.3247,-0.2721,-0.2187,
	-0.1646,-0.1100,-0.0551,0.0000,0.0551,0.1100,0.1646,
	0.2187,0.2721,0.3247,0.3763,0.4268,0.4759,0.5237,
	0.5698,0.6142,0.6568,0.6973,0.7357,0.7719,0.8058,
	0.8372,0.8660,0.8923,0.9158,0.9365,0.9544,0.9694,
	0.9815,0.9905,0.9966,0.9996,0.9996,0.9966,0.9905,
	0.9815,0.9694,0.9544,0.9365,0.9158,0.8923,0.8660,
	0.8372,0.8058,0.7719,0.7357,0.6973,0.6568,0.6142,
	0.5698,0.5237,0.4759,0.4268,0.3763,0.3247,0.2721,
	0.2187,0.1646,0.1100,0.0551,0.0000,-0.0551,-0.1100,
	-0.1646,-0.2187,-0.2721,-0.3247,-0.3763,-0.4268,-0.4759,
	-0.5237,-0.5698,-0.6142,-0.6568,-0.6973,-0.7357,-0.7719,
	-0.8058,-0.8372,-0.8660,-0.8923,-0.9158,-0.9365,-0.9544,
	-0.9694,-0.9815,-0.9905,-0.9966,-0.9996,-0.9996,-0.9966,
	-0.9905,-0.9815,-0.9694,-0.9544,-0.9365,-0.9158,-0.8923,
	-0.8660,-0.8372,-0.8058,-0.7719,-0.7357,-0.6973,-0.6568,
	-0.6142,-0.5698,-0.5237,-0.4759,-0.4268,-0.3763,-0.3247,
	-0.2721,-0.2187,-0.1646,-0.1100,-0.0551,0.0000,0.0551,
	0.1100,0.1646,0.2187,0.2721,0.3247,0.3763,0.4268,
	0.4759,0.5237,0.5698,0.6142,0.6568,0.6973,0.7357,
	0.7719,0.8058,0.8372,0.8660,0.8923,0.9158,0.9365,
	0.9544,0.9694,0.9815,0.9905,0.9966,0.9996,0.9996,
	0.9966,0.9905,0.9815,0.9694,0.9544,0.9365,0.9158,
	0.8923,0.8660,0.8372,0.8058,0.7719,0.7357,0.6973,
	0.6568,0.6142,0.5698,0.5237,0.4759,0.4268,0.3763,
	0.3247,0.2721,0.2187,0.1646,0.1100,0.0551,0.0000
	},

	{
	1.0000,0.9985,0.9939,0.9864,0.9758,0.9623,0.9458,
	0.9265,0.9044,0.8795,0.8519,0.8218,0.7891,0.7541,
	0.7168,0.6773,0.6357,0.5922,0.5469,0.5000,0.4515,
	0.4017,0.3506,0.2985,0.2455,0.1917,0.1374,0.0826,
	0.0276,-0.0276,-0.0826,-0.1374,-0.1917,-0.2455,-0.2985,
	-0.3506,-0.4017,-0.4515,-0.5000,-0.5469,-0.5922,-0.6357,
	-0.6773,-0.7168,-0.7541,-0.7891,-0.8218,-0.8519,-0.8795,
	-0.9044,-0.9265,-0.9458,-0.9623,-0.9758,-0.9864,-0.9939,
	-0.9985,-1.0000,-0.9985,-0.9939,-0.9864,-0.9758,-0.9623,
	-0.9458,-0.9265,-0.9044,-0.8795,-0.8519,-0.8218,-0.7891,
	-0.7541,-0.7168,-0.6773,-0.6357,-0.5922,-0.5469,-0.5000,
	-0.4515,-0.4017,-0.3506,-0.2985,-0.2455,-0.1917,-0.1374,
	-0.0826,-0.0276,0.0276,0.0826,0.1374,0.1917,0.2455,
	0.2985,0.3506,0.4017,0.4515,0.5000,0.5469,0.5922,
	0.6357,0.6773,0.7168,0.7541,0.7891,0.8218,0.8519,
	0.8795,0.9044,0.9265,0.9458,0.9623,0.9758,0.9864,
	0.9939,0.9985,1.0000,0.9985,0.9939,0.9864,0.9758,
	0.9623,0.9458,0.9265,0.9044,0.8795,0.8519,0.8218,
	0.7891,0.7541,0.7168,0.6773,0.6357,0.5922,0.5469,
	0.5000,0.4515,0.4017,0.3506,0.2985,0.2455,0.1917,
	0.1374,0.0826,0.0276,-0.0276,-0.0826,-0.1374,-0.1917,
	-0.2455,-0.2985,-0.3506,-0.4017,-0.4515,-0.5000,-0.5469,
	-0.5922,-0.6357,-0.6773,-0.7168,-0.7541,-0.7891,-0.8218,
	-0.8519,-0.8795,-0.9044,-0.9265,-0.9458,-0.9623,-0.9758,
	-0.9864,-0.9939,-0.9985,-1.0000,-0.9985,-0.9939,-0.9864,
	-0.9758,-0.9623,-0.9458,-0.9265,-0.9044,-0.8795,-0.8519,
	-0.8218,-0.7891,-0.7541,-0.7168,-0.6773,-0.6357,-0.5922,
	-0.5469,-0.5000,-0.4515,-0.4017,-0.3506,-0.2985,-0.2455,
	-0.1917,-0.1374,-0.0826,-0.0276,0.0276,0.0826,0.1374,
	0.1917,0.2455,0.2985,0.3506,0.4017,0.4515,0.5000,
	0.5469,0.5922,0.6357,0.6773,0.7168,0.7541,0.7891,
	0.8218,0.8519,0.8795,0.9044,0.9265,0.9458,0.9623,
	0.9758,0.9864,0.9939,0.9985,1.0000,0.9985,0.9939,
	0.9864,0.9758,0.9623,0.9458,0.9265,0.9044,0.8795,
	0.8519,0.8218,0.7891,0.7541,0.7168,0.6773,0.6357,
	0.5922,0.5469,0.5000,0.4515,0.4017,0.3506,0.2985,
	0.2455,0.1917,0.1374,0.0826,0.0276,-0.0276,-0.0826,
	-0.1374,-0.1917,-0.2455,-0.2985,-0.3506,-0.4017,-0.4515,
	-0.5000,-0.5469,-0.5922,-0.6357,-0.6773,-0.7168,-0.7541,
	-0.7891,-0.8218,-0.8519,-0.8795,-0.9044,-0.9265,-0.9458,
	-0.9623,-0.9758,-0.9864,-0.9939,-0.9985,-1.0000,-0.9985,
	-0.9939,-0.9864,-0.9758,-0.9623,-0.9458,-0.9265,-0.9044,
	-0.8795,-0.8519,-0.8218,-0.7891,-0.7541,-0.7168,-0.6773,
	-0.6357,-0.5922,-0.5469,-0.5000,-0.4515,-0.4017,-0.3506,
	-0.2985,-0.2455,-0.1917,-0.1374,-0.0826,-0.0276,0.0276,
	0.0826,0.1374,0.1917,0.2455,0.2985,0.3506,0.4017,
	0.4515,0.5000,0.5469,0.5922,0.6357,0.6773,0.7168,
	0.7541,0.7891,0.8218,0.8519,0.8795,0.9044,0.9265,
	0.9458,0.9623,0.9758,0.9864,0.9939,0.9985,1.0000
	},

	{
	-1.0000,-0.9985,-0.9939,-0.9864,-0.9758,-0.9623,-0.9458,
	-0.9265,-0.9044,-0.8795,-0.8519,-0.8218,-0.7891,-0.7541,
	-0.7168,-0.6773,-0.6357,-0.5922,-0.5469,-0.5000,-0.4515,
	-0.4017,-0.3506,-0.2985,-0.2455,-0.1917,-0.1374,-0.0826,
	-0.0276,0.0276,0.0826,0.1374,0.1917,0.2455,0.2985,
	0.3506,0.4017,0.4515,0.5000,0.5469,0.5922,0.6357,
	0.6773,0.7168,0.7541,0.7891,0.8218,0.8519,0.8795,
	0.9044,0.9265,0.9458,0.9623,0.9758,0.9864,0.9939,
	0.9985,1.0000,0.9985,0.9939,0.9864,0.9758,0.9623,
	0.9458,0.9265,0.9044,0.8795,0.8519,0.8218,0.7891,
	0.7541,0.7168,0.6773,0.6357,0.5922,0.5469,0.5000,
	0.4515,0.4017,0.3506,0.2985,0.2455,0.1917,0.1374,
	0.0826,0.0276,-0.0276,-0.0826,-0.1374,-0.1917,-0.2455,
	-0.2985,-0.3506,-0.4017,-0.4515,-0.5000,-0.5469,-0.5922,
	-0.6357,-0.6773,-0.7168,-0.7541,-0.7891,-0.8218,-0.8519,
	-0.8795,-0.9044,-0.9265,-0.9458,-0.9623,-0.9758,-0.9864,
	-0.9939,-0.9985,-1.0000,-0.9985,-0.9939,-0.9864,-0.9758,
	-0.9623,-0.9458,-0.9265,-0.9044,-0.8795,-0.8519,-0.8218,
	-0.7891,-0.7541,-0.7168,-0.6773,-0.6357,-0.5922,-0.5469,
	-0.5000,-0.4515,-0.4017,-0.3506,-0.2985,-0.2455,-0.1917,
	-0.1374,-0.0826,-0.0276,0.0276,0.0826,0.1374,0.1917,
	0.2455,0.2985,0.3506,0.4017,0.4515,0.5000,0.5469,
	0.5922,0.6357,0.6773,0.7168,0.7541,0.7891,0.8218,
	0.8519,0.8795,0.9044,0.9265,0.9458,0.9623,0.9758,
	0.9864,0.9939,0.9985,1.0000,0.9985,0.9939,0.9864,
	0.9758,0.9623,0.9458,0.9265,0.9044,0.8795,0.8519,
	0.8218,0.7891,0.7541,0.7168,0.6773,0.6357,0.5922,
	0.5469,0.5000,0.4515,0.4017,0.3506,0.2985,0.2455,
	0.1917,0.1374,0.0826,0.0276,-0.0276,-0.0826,-0.1374,
	-0.1917,-0.2455,-0.2985,-0.3506,-0.4017,-0.4515,-0.5000,
	-0.5469,-0.5922,-0.6357,-0.6773,-0.7168,-0.7541,-0.7891,
	-0.8218,-0.8519,-0.8795,-0.9044,-0.9265,-0.9458,-0.9623,
	-0.9758,-0.9864,-0.9939,-0.9985,-1.0000,-0.9985,-0.9939,
	-0.9864,-0.9758,-0.9623,-0.9458,-0.9265,-0.9044,-0.8795,
	-0.8519,-0.8218,-0.7891,-0.7541,-0.7168,-0.6773,-0.6357,
	-0.5922,-0.5469,-0.5000,-0.4515,-0.4017,-0.3506,-0.2985,
	-0.2455,-0.1917,-0.1374,-0.0826,-0.0276,0.0276,0.0826,
	0.1374,0.1917,0.2455,0.2985,0.3506,0.4017,0.4515,
	0.5000,0.5469,0.5922,0.6357,0.6773,0.7168,0.7541,
	0.7891,0.8218,0.8519,0.8795,0.9044,0.9265,0.9458,
	0.9623,0.9758,0.9864,0.9939,0.9985,1.0000,0.9985,
	0.9939,0.9864,0.9758,0.9623,0.9458,0.9265,0.9044,
	0.8795,0.8519,0.8218,0.7891,0.7541,0.7168,0.6773,
	0.6357,0.5922,0.5469,0.5000,0.4515,0.4017,0.3506,
	0.2985,0.2455,0.1917,0.1374,0.0826,0.0276,-0.0276,
	-0.0826,-0.1374,-0.1917,-0.2455,-0.2985,-0.3506,-0.4017,
	-0.4515,-0.5000,-0.5469,-0.5922,-0.6357,-0.6773,-0.7168,
	-0.7541,-0.7891,-0.8218,-0.8519,-0.8795,-0.9044,-0.9265,
	-0.9458,-0.9623,-0.9758,-0.9864,-0.9939,-0.9985,-1.0000
	}
		};

main(argc,argv)
	int argc;
	char *argv[];
{
	int i,j,k,l,m;
	int quick_flag;
	int counter,counter_limit;
	quick_flag=quick_test(argc,argv);
	get_view_surface(our_surface,argv);
	our_surface->cmapsize = 128;
	our_surface->cmapname[0] = '\0';
	if(initialize_core(BASIC, SYNCHRONOUS, TWOD))
		exit(1);
	initialize_device(KEYBOARD, 1);
	if(initialize_view_surface(our_surface,FALSE))
		exit(2);
	if(select_view_surface(our_surface))
		exit(3);
	set_window(-1.1,1.1,-1.1,1.1);
	make_maps();
	srand(getpid());
	create_temporary_segment();
	if(quick_flag)
		counter_limit=12;
	else
		counter_limit=1000000;
	for(counter=0;counter<counter_limit;counter++) {
tryagain:
		i=(rand()>>5)%NUM_FUNCTIONS;
		j=(rand()>>5)%NUM_FUNCTIONS;
		k=(rand()>>5)%NUM_FUNCTIONS;
		l=(rand()>>5)%NUM_FUNCTIONS;

		if ( (i == j) || (k == l) || ((i == l) && (k == j)) )
			goto tryagain;


		/* only do NUMLINES-1 lines ... dont redraw first line */
		for(m=0;m<NUMLINES-1;m++) {
			set_line_index(m/3 + 1);
			move_abs_2(function[i][m],function[k][m]);
			line_abs_2(function[j][m],function[l][m]);
		}

		sleep(4);
		new_frame();
	}

	close_temporary_segment();
	deselect_view_surface(our_surface);
	terminate_core();
	return 0;
}

int quick_test(argc,argv) int argc; char *argv[];
	{
	while (--argc > 0) {
		if(!strncmp(argv[argc],"-q",2))
			return(TRUE);
		}
	return(FALSE);
	}

make_maps()
{
	int i;

	red[0] = 0.0;				/* background color */
	grn[0] = 0.0;
	blu[0] = 0.0;

	for (i=0; i<19; i++) {
		red[i +   1] = 0.99;		/* ramp to yellow */
		grn[i +   1] = 0.055*i;
		blu[i +   1] = 0.0;

		red[i +  20] = 0.99-0.055*i;	/* ramp to green */
		grn[i +  20] = 0.99;
		blu[i +  20] = 0.0;

		red[i +  39] = 0.0;		/* ramp to turqouise */
		grn[i +  39] = 0.99;
		blu[i +  39] = 0.055*i;

		red[i +  58] = 0.0;		/* ramp to blue */
		grn[i +  58] = 0.99-0.055*i;
		blu[i +  58] = 0.99;

		red[i +  77] = 0.055*i;		/* ramp to violet */
		grn[i +  77] = 0.0;
		blu[i +  77] = 0.99;

		red[i +  96] = 0.99;		/* ramp to red */
		grn[i +  96] = 0.0;
		blu[i +  96] = 0.99-0.055*i;
	}

	define_color_indices(our_surface,0,MAPSIZE,red,grn,blu);

} /* end of make_maps() */