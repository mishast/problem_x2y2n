#pragma OPENCL EXTENSION cl_khr_global_int32_base_atomics : enable


__kernel void x2y2n(
   __global unsigned int* res_count,
   __global unsigned long* results,
   const unsigned long int N,
   const unsigned long grid_start_x,
   const unsigned long grid_start_y,
   const unsigned long kernel_step_width,
   const unsigned long max_xy)
{
	unsigned long step_x = get_global_id(0);
	unsigned long step_y = get_global_id(1);
	unsigned long start_x = step_x * kernel_step_width + grid_start_x;
	unsigned long start_y = step_y * kernel_step_width + grid_start_y;
	unsigned long end_x = start_x + kernel_step_width;
	unsigned long end_y = start_y + kernel_step_width;

	//printf("kern grid start %d %d kern step %d %d kern start %d %d - %d %d kern max_xy %d\n", grid_start_x, grid_start_y, step_x, step_y, start_x, start_y, end_x, end_y, max_xy);

	if (end_y > max_xy) end_y = max_xy;
	if (end_x > max_xy) end_x = max_xy;
	
	//start_x = end_x;
	//start_y = end_y;

	for (unsigned long y = start_y; y < end_y; y++)
	{
		for (unsigned long x = start_x; x < end_x; x++)
		{
			unsigned long n_res = x*x + y*y;

			if (n_res == N)
			{
				unsigned int res_cnt = atom_inc(res_count);
				unsigned long pos = res_cnt << 1;
				results[pos] = x;
				results[pos + 1] = y;
			}
		}
	}
}

