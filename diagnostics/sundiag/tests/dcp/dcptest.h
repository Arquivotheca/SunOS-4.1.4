
/*     @(#)dcptest.h 1.1 94/10/31 SMI      */

/* 
 * Copyright (c) 1987 by Sun Microsystems, Inc. 
 */
#define DCPTEST_ERROR           6
#define PROBE_DES_ERROR 	8
#define EEPROM_BOARDS           17
#define EEPROM_BYTES            8
#define DES_CBC 		0
#define DES_ECB 		1
#define DATA_BUFSIZ 		256
#define KEY_BUFSIZ 		8
#define LOOP_TESTS		400

enum parity {
   high_bit_parity=0,
   low_bit_parity=1
};
typedef enum parity Parity;

enum des_ioctl {
   block=0,
   quick=1
};
typedef enum des_ioctl Des_ioctl;

