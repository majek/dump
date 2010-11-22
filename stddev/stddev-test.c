#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>

#include "stddev.h"


int main() {
	uint64_t cnt;
	double avg, dev;
	struct stddev *sd = stddev_new();

	stddev_add(sd, 1);
	stddev_add(sd, 2);
	stddev_add(sd, 3);

	stddev_get(sd, &cnt, &avg, &dev);
	assert(cnt == 3);
	assert(avg == 2.0);
	assert(abs(dev - 0.18) < 0.001 );

	stddev_remove(sd, 3);
	stddev_get(sd, &cnt, &avg, &dev);
	assert(cnt == 2);
	assert(avg == 1.5);
	assert(dev == 0.5);

	stddev_remove(sd, 2);
	stddev_get(sd, NULL, &avg, &dev);
	assert(avg == 1);
	assert(dev == 0.0);

	stddev_get(sd, NULL, NULL, &dev);
	assert(dev == 0.0);
	stddev_get(sd, NULL, NULL, NULL);

	stddev_remove(sd, 1);
	stddev_get(sd, NULL, &avg, NULL);
	assert(avg == 0.0);

	return 0;
}
