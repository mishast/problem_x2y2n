# Problem X2 + Y2 = N


Application solves problem: Given two positive integers. Find the sum of their squares. Further, assuming that we only know this amount, it is necessary to find two initial numbers.

OpenCL and GPU version.

## About the task and its implementation

The task turned out to be more interesting than it seemed at first glance. It was expected that there is either a well-known way to solve this problem, or the problem can be solved only by searching. In fact, it turned out that algorithms with less computational complexity than brute force exist, but nowhere have they been published explicitly, that it is necessary to invent such an algorithm on the basis of number theory and well-known algorithms from number theory, I finally understood how these problems are solved using the theory of numbers, but the algorithms are difficult to implement and are not very suitable for parallelization because iterative, one of them, for example, requires decomposition of the total factor into factors. About these algorithms there is information below (in the Algorithms section). As a result, the problem is solved by sorting numbers.
It was also expected that as a result of solving the problem, only one pair of numbers is obtained by analogy with the numbers in the Pythagorean trio, but it turned out that there could be many solutions.

It turned out that in OpenCL there is no built-in support for simultaneous launch on all available devices, that it is necessary to manually launch the program on each device.
For parallelizing on different devices, two strategies are obvious:
1. Run the task on each device sequentially, by 1, by 2, by 3, by 1, by 2, by 3, by 1 ...
2. Run the task on the first free device

Strategy 1 is very bad, because the CPU is too slow, and it turns out the total time to solve the problem when using strategy 1 is several times longer even if you don’t split into devices, and solve only on 1 GPU.

The 2nd strategy was chosen. It was necessary to catch the moment when one of the devices had finished. In OpenCL there is no analogue of the WaitForMultipleObjects function that would wait for completion on any of the devices, there is a function clWaitForEvents, but it is waiting for all events to complete, so this function is not suitable for solving the problem. At first I tried to use the clSetEventCallback function, which sets the callback to complete the event, but it worked for a long time, I had to use this solution: spinning in a loop, polling the state of the event.

At first there was a variant of the program with unsigned int, but the maximum number for which it would be possible to carry out a calculation without overflow is 40000, and for such a number the counting took place very quickly. To make the result more representative, I decided to switch to unsigned long.

The variant OpenCL program with unsigned long works twice as long for the same number. Probably arithmetic operations with unsigned long occur in two steps.

It also turned out that modern GPUs perform operations with vectors in an element-by-element manner, so there would be no gain from using vectors.

To solve the problem, OpenCL C ++ Wrapper, from the developer of the OpenCL Khronos standard, was used.

# About results

I have the speed of one "task" for the device (work 128x128 cores):

GeForce 840M = 18.75 ms
Intel HDGraphics 5500 = 55.32 ms
CPU i7-5500U (mobile version!) = 291.683ms

Since HDGraphics 5500 is 3 times slower than the GeForce 840M, the speed gain when adding Intel HDGraphics 5500 is only 1/3, there is no speed gain when adding a CPU.

### Execution Example:

```
Use values: x = 160001 y = 160002
Available devices:
Id = 00770954 Device = Intel (R) HD Graphics 5500
Id = 00794FA0 Device = Intel (R) Core (TM) i7-5500U CPU @ 2.40GHz
Id = 007F03D0 Device = GeForce 840M
Start tasks
Elapsed = 3406 ms 535 microsec.
device = 0 result count = 4 usage count = 54
x = 32001 y = 224002
x = 92097 y = 206686
x = 86154 y = 209233
x = 131311 y = 184278
device = 1 result count = 0 usage count = 9
device = 2 result count = 12 usage count = 133
x = 5518 y = 226209
x = 50334 y = 220607
x = 115694 y = 194463
x = 160002 y = 160001
x = 160001 y = 160002
x = 194463 y = 115694
x = 184278 y = 131311
x = 209233 y = 86154
x = 206686 y = 92097
x = 226209 y = 5518
x = 224002 y = 32001
x = 220607 y = 50334
Press enter to exit ...
```

### Calculation on CPU:

```
Use values: x = 60001 y = 60002
Elapsed = 67585 ms 471 microsec.
Results count = 16
x = 12001 y = 84002
x = 13282 y = 83809
x = 22367 y = 81854
x = 23614 y = 81503
x = 51034 y = 67793
x = 52063 y = 67006
x = 59078 y = 60911
x = 60001 y = 60002
x = 60002 y = 60001
x = 60911 y = 59078
x = 67006 y = 52063
x = 67793 y = 51034
x = 81503 y = 23614
x = 81854 y = 22367
x = 83809 y = 13282
x = 84002 y = 12001
Press enter to exit ...
```

### Calculation on all devices:

```
Use values: x = 60001 y = 60002
Available devices:
Id = 0070F6CC Device = Intel (R) HD Graphics 5500
Id = 006F7A20 Device = Intel (R) Core (TM) i7-5500U CPU @ 2.40GHz
Id = 0073C390 Device = GeForce 840M
Start tasks
Elapsed = 783 ms 477 microsec.
device = 0 result count = 6 usage count = 9
x = 12001 y = 84002
x = 13282 y = 83809
x = 23614 y = 81503
x = 22367 y = 81854
x = 52063 y = 67006
x = 51034 y = 67793
device = 1 result count = 0 usage count = 2
device = 2 result count = 10 usage count = 25
x = 60911 y = 59078
x = 60002 y = 60001
x = 60001 y = 60002
x = 59078 y = 60911
x = 81854 y = 22367
x = 81503 y = 23614
x = 67006 y = 52063
x = 67793 y = 51034
x = 83809 y = 13282
x = 84002 y = 12001
Press enter to exit ...
```

# Build the application

The program was compiled under Microsoft Visual Studio 2015 Community Editon (you can download it for free).
Most likely the program can be compiled for other versions of Visual Studio, and for gcc and under clang.

NVIDIA CUDA Toolkit was used as OpenCL, but most likely the program can be compiled and run using the AMD SDK, and possibly the Intel SDK.

# Download links

Microsoft Visual Studio 2015 Community Edition
https://www.microsoft.com/en-US/download/details.aspx?id=48146

Visual C ++ Redistributable for Visual Studio 2015
https://www.microsoft.com/en-us/download/details.aspx?id=48145

Microsoft Visual Studio 2012 Community Edition
http://www.microsoft.com/en-us/Download/details.aspx?id=30679

Visaul C ++ Redistributable for Visual Studio 2012
http://www.microsoft.com/en-us/Download/details.aspx?id=30679

Cuda toolkit
https://developer.nvidia.com/cuda-toolkit

APP SDK
http://developer.amd.com/tools-and-sdks/opencl-zone/amd-accelerated-parallel-processing-app-sdk/

Intel SDK
https://software.intel.com/en-us/intel-opencl

# Algorithms for solving the diafant equation x ^ 2 + y ^ 2 = A

1. The algorithm for finding a solution using the factorization of the number A:

1. Represent the number A in the form p1 * p2 * p3 ... pn, pi are prime factors
2. Present the number A as p1 * p2 * ... pn * m ^ 2, where m is the maximum possible number
3. The equation reduces to x ^ 2 + y ^ 2 = A / (m ^ 2)
4. Equations of the form x ^ 2 + y ^ 2 = p1 ... x ^ 2 + y ^ 2 = pn are solved.
5. Using the transformation (a ^ 2 + b ^ 2) (c ^ 2 + d ^ 2) = (ac - bd) ^ 2 + (ad + bc) ^ 2 we find the solution to the original equation

Example for A = 29250
A = 29250 = 2 * 5 * 13 * (15) ^ 2
2 = 1 ^ 2 + 1 ^ 2
5 = 1 ^ 2 + 2 ^ 2
13 = 2 ^ 2 + 3 ^ 2

2 = 1 ^ 2 + 1 ^ 2
2 * 5 = (1 * 1 - 1 * 2) ^ 2 + (1 * 2 + 1 * 1) ^ 2 = 1 ^ 2 + 3 ^ 2
2 * 5 * 13 = (1 * 2 - 3 * 3) ^ 2 + (1 * 3 + 3 * 2) ^ 2 = 7 ^ 2 + 9 ^ 2
29250 = 2 * 5 * (15) ^ 2 = (7 * 15) ^ 2 + (9 * 15) ^ 2 = 105 ^ 2 + 135 ^ 2

The factorization is difficult to parallelize, so this algorithm is not implemented.

2. Algorithm for finding a solution using the algorithm Cornacchia
1. The algorithm works only for the case when the roots of the equation x, y are mutually simple. If the roots of the equation are not mutually simple, then the equation can be converted to such an equation, where x, y are mutually simple.

x ^ 2 + y ^ 2 = A
x1 ^ 2 + y1 ^ 2 = m * c ^ 2
The roots of the equation x ^ 2 + y ^ 2 = m are mutually simple (omit the proof), and the roots of the original equation can be found by the formula x = x1 * c; y = y1 * c

2. Find a r0 such that: r0 ^ 2 = -1 (mod m)
3. Find r1 = m (mod r0), r2 = r0 (mod r1), etc. the algorithm stops when ri <sqrt (m), if s = sqrt ((m-rk ^ 2) / d), then there are solutions, and x = rk, y = s

The algorithm is difficult to implement (it is necessary to implement the decomposition on A = m * c ^ 2) and also the search for r0, and is not very suitable for parallelization

You can see the implementation in python here: https://github.com/sympy/sympy/blob/master/sympy/solvers/diophantine.py
Here is a comment about the implementation: https://thilinaatsympy.wordpress.com/2013/08/18/improving-the-solving-process-by-using-cornacchia/

And here to look at: http://docs.sympy.org/latest/install.html#run-sympy
