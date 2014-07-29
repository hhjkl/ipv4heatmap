/*
 * IPv4 Heatmap
 * (C) 2007 The Measurement Factory, Inc
 * Licensed under the GPL, version 2
 * http://maps.measurement-factory.com/
 */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <memory.h>
#include <err.h>
#include <assert.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "cidr.h"
#include "bbox.h"
#include "xy_from_ip.h"

#ifndef MIN
#define MIN(a,b) (a<b?a:b)
#define MAX(a,b) (a>b?a:b)
#endif

extern int debug;
extern int morton_flag;
extern unsigned int addr_space_first_addr;
extern unsigned int addr_space_last_addr;


/*
 * Find the "bounding box" for the IPv4 netblock starting at 'first' and having
 * 'slash' netmask bits.
 *
 * For square areas this is pretty easy.  We know how to find the point diagonally
 * opposite the first value (add 1010..1010). Its a little harder for
 * rectangular areas, so I cheat a little and divide it into the two smaller
 * squares.
 */
static bbox bounding_box(unsigned int first, int slash) {
    bbox box;
    unsigned int diag = 0xAAAAAAAA;
    if (morton_flag)
  diag = 0xFFFFFFFF;
    unsigned int x1 = 0, y1 = 0, x2 = 0, y2 = 0;

    if (slash > 31) {
	/*
	 * treat /32 as a special case
	 */
	xy_from_ip(first, &x1, &y1);
	box.xmin = x1;
	box.ymin = y1;
	box.xmax = x1;
	box.ymax = y1;
    } else if (0 == (slash & 1)) {
	/*
	 * square
	 */
	diag >>= slash;
	xy_from_ip(first, &x1, &y1);
	xy_from_ip(first + diag, &x2, &y2);
	box.xmin = MIN(x1, x2);
	box.ymin = MIN(y1, y2);
	box.xmax = MAX(x1, x2);
	box.ymax = MAX(y1, y2);
    } else {
	/*
	 * rectangle: divide, conquer
	 */
	bbox b1 = bounding_box(first, slash + 1);
	bbox b2 = bounding_box(first + (1 << (32 - (slash + 1))), slash + 1);
	box.xmin = MIN(b1.xmin, b2.xmin);
	box.ymin = MIN(b1.ymin, b2.ymin);
	box.xmax = MAX(b1.xmax, b2.xmax);
	box.ymax = MAX(b1.ymax, b2.ymax);
    }
    return box;
}

/*
 * Calculate the bounding box of a CIDR prefix string
 */
bbox bbox_from_cidr(const char *cidr) {
    int slash;
    unsigned int first;
    unsigned int last;
    bbox bbox;
    cidr_parse(cidr, &first, &last, &slash);
    if (first < addr_space_first_addr || last > addr_space_last_addr) {
	bbox.xmin = bbox.ymin = bbox.xmax = bbox.ymax = -1;
	return bbox;
    }
    memset(&bbox, '\0', sizeof(bbox));
    bbox = bounding_box(first, slash);

    return bbox;
}
