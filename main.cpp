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
int KernelBumpToNormalMap(unsigned char *h_in_img, unsigned char *h_out_img);
void ShutdownBumpToNormalMap();

void CpuBumpToNormalMap(unsigned char *h_in_img, unsigned char *h_out_img);

static unsigned char *cpuIn;
static unsigned char *cpuOut;

static unsigned char *gpuIn;
static unsigned char *gpuOut;

static int imageWidth, imageHeight, imageChannels;

static bool s_ThreadAlive = false;

int main(int argc, char **)
{
	unsigned char *imgPtr = stbi_load("./res/test.png", &imageWidth, &imageHeight, &imageChannels, 1);

	{
		nic::ScopeTimerOStream<> t("GPU Init", std::cout);
		InitBumpToNormalMap(imageWidth, imageHeight);
		gpuIn = new unsigned char[imageWidth * imageHeight * 1];
		gpuOut = new unsigned char[imageWidth * imageHeight * 4];
	}
	{
		nic::ScopeTimerOStream<> t("CPU Init", std::cout);
		cpuIn = new unsigned char[(imageWidth + 2) * (imageHeight + 2) * 1];
		cpuOut = new unsigned char[(imageWidth + 2) * (imageHeight + 2) * 4];
	}
	std::cout << '\n';
	std::memcpy(gpuIn, imgPtr, imageWidth * imageHeight * 1 * sizeof(unsigned char));

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
	stbi_write_png("./res/GPUres.png", imageWidth, imageHeight, 4, gpuOut, imageWidth * 4);

	std::memcpy(cpuIn, imgPtr, imageWidth * imageHeight * 1 * sizeof(unsigned char));
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
	stbi_write_png("./res/CPUres.png", imageWidth, imageHeight, 4, cpuOut, imageWidth * 4);
	std::cout << '\n';

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
	std::cout << "\nProfile finished\n";

	stbi_image_free(imgPtr);
}

#define OFFSET(di,dj) (i + di) + (j + dj) * imageWidth
#define INPOS  (i + j * imageWidth)

void CpuBumpToNormalMap(unsigned char *h_in_img, unsigned char *h_out_img)
{
	float * s = new float[imageWidth * imageHeight];

	for (size_t i = 1; i < (imageWidth - 1); i++)
		for (size_t j = 1; j < (imageHeight - 1); j++)
			s[INPOS] = h_in_img[INPOS];

	for (size_t i = 1; i < (imageWidth - 1); i++)
		for (size_t j = 1; j < (imageHeight - 1); j++)
		{
			const float sobelX = (
														-1 * s[OFFSET(-1,-1)] +0 * s[OFFSET(0,-1)] +1 * s[OFFSET(1,-1)]
														-2 * s[OFFSET(-1, 0)] +0 * s[OFFSET(0, 0)] +2 * s[OFFSET(1, 0)]
														-1 * s[OFFSET(-1, 1)] +0 * s[OFFSET(0, 1)] +1 * s[OFFSET(1, 1)]
												) * 0.25f;
												
			const float sobelY = (
														 1 * s[OFFSET(-1,-1)] +2 * s[OFFSET(0,-1)] +1 * s[OFFSET(1,-1)]
														+0 * s[OFFSET(-1, 0)] +0 * s[OFFSET(0, 0)] +0 * s[OFFSET(1, 0)]
														-1 * s[OFFSET(-1, 1)] -2 * s[OFFSET(0, 1)] -1 * s[OFFSET(1, 1)]
												) * 0.25f;

			const float gradientLen = sqrt(sobelX*sobelX + sobelY*sobelY + 255.0f * 255.0f);

			const unsigned char xLen = (-sobelX/gradientLen) * 128.0f + 128.0f;
			const unsigned char yLen = (-sobelY/gradientLen) * 128.0f + 128.0f;
			const unsigned char zLen = (255.0f * 255.0f)/gradientLen;

			h_out_img[INPOS * 4 + 0] = xLen;
			h_out_img[INPOS * 4 + 1] = yLen;
			h_out_img[INPOS * 4 + 2] = zLen;
			h_out_img[INPOS * 4 + 3] = 255;
		}

		delete[] s;
}