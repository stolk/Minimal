// obcat.h
// Different physics object categories (Bullet version: only 16 bits!)

#ifndef OBCAT_H
#define OBCAT_H

#define OBCAT_TERR	(1<<0)  // terrain
#define OBCAT_PROP	(1<<1)  // props
#define OBCAT_TRUK	(1<<2)  // truck
#define OBCAT_CRAN	(1<<3)  // crane
#define OBCAT_GRAP	(1<<4)  // grapple
#define OBCAT_TRAC	(1<<5)  // track links
#define OBCAT_TYRE	(1<<6)  // tyres
#define OBCAT_SNSR	(1<<7)  // sensor
#define OBCAT_DIGR	(1<<8)  // digger
#define OBCAT_DIRT	(1<<9)  // dirt
#define OBCAT_SPKT	(1<<10) // sprocket of crawler
#define OBCAT_OTHR	(1<<11)	// other

#endif

