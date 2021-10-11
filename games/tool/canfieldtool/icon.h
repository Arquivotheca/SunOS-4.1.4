
/* @(#)icon.h 1.1 94/10/31 SMI	 */
/*
 * Copyright (c) 1985 by Sun Microsystems, Inc. 
 */
unsigned short  icon_image[256] = {
#include <images/cardback.icon>
};
DEFINE_ICON_FROM_IMAGE(backicon, icon_image);

unsigned short  icon2_image[256] = {
#include <images/canfield.icon>
};
DEFINE_ICON_FROM_IMAGE(toolicon, icon2_image);
