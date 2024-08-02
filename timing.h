// I'm not sure who the author of this was. I grabbed it from a repo of a
// course assignment. I don't remember writing it, so my guess is that
// it was Troels Henriksen, associate professor at the University of Copenhagen.

#ifndef TIMING_H
#define TIMING_H

#include <stdint.h>
#include <sys/time.h>

static uint64_t microseconds() {
  static struct timeval t;
  gettimeofday(&t, NULL);
  return ((uint64_t)t.tv_sec*1000000)+t.tv_usec;
}

#endif
