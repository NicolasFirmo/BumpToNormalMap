#include <cuda_runtime.h>
#include <device_launch_parameters.h> // threadIdx
#include <device_functions.h>
#include <cstdio>

static unsigned char* d_in;
static unsigned char* d_out;

static unsigned int h_Width;
static unsigned int h_Height;

static unsigned int h_BlockWidth;
static unsigned int h_BlockHeight;

#define THREAD_TOTAL_X_LEN 16
#define THREAD_AUX_X_LEN 2
#define THREAD_WORKING_X_LEN (THREAD_TOTAL_X_LEN - THREAD_AUX_X_LEN)

#define THREAD_TOTAL_Y_LEN 16
#define THREAD_AUX_Y_LEN 2
#define THREAD_WORKING_Y_LEN (THREAD_TOTAL_Y_LEN - THREAD_AUX_Y_LEN)

#define OFFSET(x,y) sIdx + y * THREAD_TOTAL_X_LEN + x

__global__ void Sobel(const unsigned char* in,unsigned char* out, const unsigned int width, const unsigned int height)
{
	extern __shared__ short s[];

	const unsigned int xPos = (blockIdx.x * THREAD_WORKING_X_LEN + threadIdx.x) - (THREAD_AUX_X_LEN / 2);
	const unsigned int yPos = (blockIdx.y * THREAD_WORKING_Y_LEN + threadIdx.y) - (THREAD_AUX_Y_LEN / 2);
	const unsigned int inPos = (xPos + yPos * width);
	const unsigned int sIdx = (threadIdx.x + threadIdx.y * THREAD_TOTAL_X_LEN);

	if (xPos < width && yPos < height)
		s[sIdx] = in[inPos];
	else
		s[sIdx] = 0;

	__syncthreads();

	if ((threadIdx.x - (THREAD_AUX_X_LEN / 2)) < THREAD_WORKING_X_LEN && (threadIdx.y - (THREAD_AUX_X_LEN / 2)) < THREAD_WORKING_Y_LEN)
	{
		const float sobelX = (
													-1 * s[OFFSET(-1,-1)] +0 * s[OFFSET(0,-1)] +1 * s[OFFSET(1,-1)]
													-2 * s[OFFSET(-1, 0)] +0 * s[OFFSET(0, 0)] +2 * s[OFFSET(1, 0)]
													-1 * s[OFFSET(-1, 1)] +0 * s[OFFSET(0, 1)] +1 * s[OFFSET(1, 1)]
												)/(0.08f * 255.0f);

		const float sobelY =	(
													+1 * s[OFFSET(-1,-1)] +2 * s[OFFSET(0,-1)] +1 * s[OFFSET(1,-1)]
													+0 * s[OFFSET(-1, 0)] +0 * s[OFFSET(0, 0)] +0 * s[OFFSET(1, 0)]
													-1 * s[OFFSET(-1, 1)] -2 * s[OFFSET(0, 1)] -1 * s[OFFSET(1, 1)]
												)/(0.08f * 255.0f);

		const float gradientLen = sqrt(sobelX*sobelX + sobelY*sobelY + 1.0f);

		const unsigned char xLen = (-sobelX/gradientLen) * 128.0f + 128.0f;
		const unsigned char yLen = (-sobelY/gradientLen) * 128.0f + 128.0f;
		const unsigned char zLen = 255.0f/gradientLen;

		out[inPos * 4 + 0] = xLen;
		out[inPos * 4 + 1] = yLen;
		out[inPos * 4 + 2] = zLen;
		out[inPos * 4 + 3] = 255;
	}
}

void InitBumpToNormalMap(const unsigned int width, const unsigned int height)
{
	h_Width = width;
	h_Height = height;

	h_BlockWidth = (h_Width / THREAD_WORKING_X_LEN);
	h_BlockHeight = (h_Height / THREAD_WORKING_Y_LEN);

	cudaMalloc(&d_in, h_Width * h_Height * 1 * sizeof(unsigned char));
	cudaMalloc(&d_out, h_Width * h_Height * 4 * sizeof(unsigned char));
}

int KernelBumpToNormalMap(unsigned char* h_in_img,unsigned char* h_out_img)
{
	cudaError_t error = cudaMemcpy(d_in, h_in_img, h_Width * h_Height * 1 * sizeof(unsigned char), cudaMemcpyHostToDevice);
	if (error != cudaSuccess)
		return error;

	Sobel<<<dim3(h_BlockWidth, h_BlockHeight, 1), dim3(THREAD_TOTAL_X_LEN, THREAD_TOTAL_Y_LEN, 1), THREAD_TOTAL_X_LEN * THREAD_TOTAL_Y_LEN * sizeof(short)>>>(d_in, d_out, h_Width, h_Height);
	error = cudaGetLastError();
	if (error != cudaSuccess)
		return error;

	error = cudaMemcpy(h_out_img, d_out, h_Width * h_Height * 4 * sizeof(unsigned char), cudaMemcpyDeviceToHost);
	if (error != cudaSuccess)
		return error;

	return cudaSuccess;
}

void ShutdownBumpToNormalMap()
{
	cudaFree(d_in);
	cudaFree(d_out);
}

