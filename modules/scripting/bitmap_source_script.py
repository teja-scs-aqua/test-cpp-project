#python

import k3d

context.output.reset(64, 64)

for x in range(64):
	for y in range(64):
		if x < 32:
			if y < 32:
				context.output.set_pixel(x, y, (1, 0, 0, 1))
			else:
				context.output.set_pixel(x, y, (0, 0, 1, 1))
		else:
			if y < 32:
				context.output.set_pixel(x, y, (0, 1, 0, 1))
			else:
				context.output.set_pixel(x, y, (1, 1, 1, 1))

