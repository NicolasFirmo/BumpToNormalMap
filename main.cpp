#include <iostream>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <vector>
#include <thread>

#define STB_IMAGE_IMPLEMENTATION
#include "vendor/stb/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "vendor/stb/stb_image_write.h"

#include "Instrumentation/ScopeTimer.h"

void InitBumpToNormalMap(const unsigned int width, const unsigned int height);
int KernelBumpToNormalMap(float *h_in_img, float *h_out_img);
void ShutdownBumpToNormalMap();

void CpuBumpToNormalMap(float *h_in_img, float *h_out_img);

static float *cpuIn;
static float *cpuOut;

static float *gpuIn;
static float *gpuOut;

static int imageWidth, imageHeight, imageChannels;

static bool s_ThreadAlive = false;

#define INPOS  (i + j * imageWidth)

int main(int argc, char **)
{
	unsigned char *imgPtr = stbi_load("./res/test.png", &imageWidth, &imageHeight, &imageChannels, 1);
	unsigned char *outImg = new unsigned char[imageWidth * imageHeight * 4];

	{
		nic::ScopeTimerOStream<> t("GPU Init", std::cout);
		InitBumpToNormalMap(imageWidth, imageHeight);
		gpuIn = new float[imageWidth * imageHeight * 1];
		gpuOut = new float[imageWidth * imageHeight * 4];
	}
	{
		nic::ScopeTimerOStream<> t("CPU Init", std::cout);
		cpuIn = new float[(imageWidth + 2) * (imageHeight + 2) * 1];
		cpuOut = new float[(imageWidth + 2) * (imageHeight + 2) * 4];
	}
	std::cout << '\n';

	for (size_t i = 0; i < (imageWidth * 1); i++)
		for (size_t j = 0; j < (imageHeight); j++)
		{
			cpuIn[i + j * imageWidth] = float(imgPtr[i + j * imageWidth])/255.0f;
			gpuIn[i + j * imageWidth] = float(imgPtr[i + j * imageWidth])/255.0f;
		}

	s_ThreadAlive = true;
	std::thread t{[]() {
		for (size_t i = 0; i < 1000; i++)
			KernelBumpToNormalMap(gpuIn, gpuOut);

		s_ThreadAlive = false;
	}};
	{
		nic::ScopeTimerOStream<nic::milliseconds> t("1000 loops", std::cout);
		while (s_ThreadAlive)
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	t.join();

	for (size_t i = 0; i < (imageWidth * 4); i++)
		for (size_t j = 0; j < (imageHeight); j++)
			outImg[i + j * imageWidth * 4] = gpuOut[i + j * imageWidth * 4] * 255.0f;
	stbi_write_png("./res/GPUres.png", imageWidth, imageHeight, 4, outImg, imageWidth * 4);

	s_ThreadAlive = true;
	std::thread t2{[]() {
		for (size_t i = 0; i < 1000; i++)
			CpuBumpToNormalMap(cpuIn, cpuOut);

		s_ThreadAlive = false;
	}};
	{
		nic::ScopeTimerOStream<nic::milliseconds> t("1000 loops", std::cout);
		while (s_ThreadAlive)
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	t2.join();
	
	for (size_t i = 0; i < (imageWidth * 4); i++)
		for (size_t j = 0; j < (imageHeight); j++)
			outImg[i + j * imageWidth * 4] = cpuOut[i + j * imageWidth * 4] * 255.0f;
	stbi_write_png("./res/CPUres.png", imageWidth, imageHeight, 4, outImg, imageWidth * 4);

	{
		nic::ScopeTimerOStream<> t("GPU Shutdown", std::cout);
		ShutdownBumpToNormalMap();
		delete[] gpuIn;
		delete[] gpuOut;
	}
	{
		nic::ScopeTimerOStream<> t("CPU shutdow", std::cout);
		delete[] cpuIn;
		delete[] cpuOut;
	}

	stbi_image_free(imgPtr);
	delete[] outImg;

	std::cin.ignore();
}

#define OFFSET(di,dj) (i + di) + (j + dj) * imageWidth

void CpuBumpToNormalMap(float *h_in_img, float *h_out_img)
{

	for (size_t i = 1; i < (imageWidth - 1); i++)
		for (size_t j = 1; j < (imageHeight - 1); j++)
		{
			const float sobelX = (
														-1 * h_in_img[OFFSET(-1,-1)] +0 * h_in_img[OFFSET(0,-1)] +1 * h_in_img[OFFSET(1,-1)]
														-2 * h_in_img[OFFSET(-1, 0)] +0 * h_in_img[OFFSET(0, 0)] +2 * h_in_img[OFFSET(1, 0)]
														-1 * h_in_img[OFFSET(-1, 1)] +0 * h_in_img[OFFSET(0, 1)] +1 * h_in_img[OFFSET(1, 1)]
												) * 0.25f;
												
			const float sobelY = (
														 1 * h_in_img[OFFSET(-1,-1)] +2 * h_in_img[OFFSET(0,-1)] +1 * h_in_img[OFFSET(1,-1)]
														+0 * h_in_img[OFFSET(-1, 0)] +0 * h_in_img[OFFSET(0, 0)] +0 * h_in_img[OFFSET(1, 0)]
														-1 * h_in_img[OFFSET(-1, 1)] -2 * h_in_img[OFFSET(0, 1)] -1 * h_in_img[OFFSET(1, 1)]
												) * 0.25f;

			const float gradientLen = sqrt(sobelX*sobelX + sobelY*sobelY + 1.0f);

			const float xLen = (-sobelX/gradientLen) * 0.5f + 0.5f;
			const float yLen = (-sobelY/gradientLen) * 0.5f + 0.5f;
			const float zLen = 1.0f/gradientLen;

			h_out_img[INPOS * 4 + 0] = xLen;
			h_out_img[INPOS * 4 + 1] = yLen;
			h_out_img[INPOS * 4 + 2] = zLen;
			h_out_img[INPOS * 4 + 3] = 1.0f;
		}

}