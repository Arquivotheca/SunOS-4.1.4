/*	@(#)report.h 1.1 94/10/31 SMI	*/

/* report structure used in demosubs, demotest */
struct test_report{
	char oper[80];
	int iter;
	int total_time;
	int oper_time;
} report;

/*
 * The TEST specifies that just testing is done NO benchmarking,
 * and NO_TEST specifies that benchmarking should be done.
 */
#define TEST			0x1
#define NO_TEST			0x0
