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

static unsigned char *gpuIn;
static unsigned char *gpuOut;
static unsigned char *gpuIn2;
static unsigned char *gpuOut2;

static bool s_ThreadAlive = false;

int main(int argc, char **)
{
	int imageWidth, imageHeight, imageChannels;
	unsigned char *imgPtr = stbi_load("./res/test.png", &imageWidth, &imageHeight, &imageChannels, 1);
	unsigned char *imgPtr2 = stbi_load("./res/test2.png", &imageWidth, &imageHeight, &imageChannels, 1);

	unsigned char *cpuIn;
	unsigned char *cpuOut;

	{
		nic::ScopeTimerOStream<> t("GPU Init", std::cout);
		InitBumpToNormalMap(imageWidth, imageHeight);
		gpuIn = new unsigned char[imageWidth * imageHeight * 1];
		gpuIn2 = new unsigned char[imageWidth * imageHeight * 1];
		gpuOut = new unsigned char[imageWidth * imageHeight * 4];
		gpuOut2 = new unsigned char[imageWidth * imageHeight * 4];
	}
	{
		nic::ScopeTimerOStream<> t("CPU Init", std::cout);
		cpuIn = new unsigned char[(imageWidth + 2) * (imageHeight + 2) * 1];
		cpuOut = new unsigned char[(imageWidth + 2) * (imageHeight + 2) * 4];
	}
	std::cout << '\n';
	std::memcpy(gpuIn, imgPtr, imageWidth * imageHeight * 1 * sizeof(unsigned char));
	std::memcpy(gpuIn2, imgPtr2, imageWidth * imageHeight * 1 * sizeof(unsigned char));

	s_ThreadAlive = true;
	std::thread t{[]() {
		for (size_t i = 0; i < 1000; i++)
		{
			KernelBumpToNormalMap(gpuIn, gpuOut);
			KernelBumpToNormalMap(gpuIn2, gpuOut2);
		}
		s_ThreadAlive = false;
	}};
	{
		nic::ScopeTimerOStream<nic::milliseconds> t("1000 loops", std::cout);
		while (s_ThreadAlive)
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	t.join();
	stbi_write_png("./res/GPUres.png", imageWidth, imageHeight, 4, gpuOut, imageWidth * 4);
	stbi_write_png("./res/GPUres2.png", imageWidth, imageHeight, 4, gpuOut2, imageWidth * 4);
	// {
	// 	nic::ScopeTimerOStream<> t("CPU Routine", std::cout);
	// 	for (size_t n = 0; n < 50; n++)
	// 	{
	// 		for (size_t j = 0; j < imageHeight; j++)
	// 			std::memcpy(&cpuIO[((j + 2) * imageWidth + 2) * 3], &imgPtr[(j * imageWidth) * 3], imageWidth * 3 * sizeof(unsigned char));
	// 		for (size_t k = 0; k < 3; k++)
	// 			for (size_t i = 2; i < (imageHeight + 4 / 2); i++)
	// 				for (size_t j = 2; j < (imageWidth + 4 / 2); j++)
	// 				{
	// 					const auto sum = cpuIO[(imageWidth * (i - 2) + j - 2) * 3 + k] * 1 + cpuIO[(imageWidth * (i - 2) + j - 1) * 3 + k] * 4 + cpuIO[(imageWidth * (i - 2) + j + 0) * 3 + k] * 6 + cpuIO[(imageWidth * (i - 2) + j + 1) * 3 + k] * 4 + cpuIO[(imageWidth * (i - 2) + j + 2) * 3 + k] * 1 +
	// 													 cpuIO[(imageWidth * (i - 1) + j - 2) * 3 + k] * 4 + cpuIO[(imageWidth * (i - 1) + j - 1) * 3 + k] * 15 + cpuIO[(imageWidth * (i - 1) + j + 0) * 3 + k] * 24 + cpuIO[(imageWidth * (i - 1) + j + 1) * 3 + k] * 15 + cpuIO[(imageWidth * (i - 1) + j + 2) * 3 + k] * 4 +
	// 													 cpuIO[(imageWidth * (i + 0) + j - 2) * 3 + k] * 6 + cpuIO[(imageWidth * (i + 0) + j - 1) * 3 + k] * 24 + cpuIO[(imageWidth * (i + 0) + j + 0) * 3 + k] * 40 + cpuIO[(imageWidth * (i + 0) + j + 1) * 3 + k] * 24 + cpuIO[(imageWidth * (i + 0) + j + 2) * 3 + k] * 6 +
	// 													 cpuIO[(imageWidth * (i + 1) + j - 2) * 3 + k] * 4 + cpuIO[(imageWidth * (i + 1) + j - 1) * 3 + k] * 15 + cpuIO[(imageWidth * (i + 1) + j + 0) * 3 + k] * 24 + cpuIO[(imageWidth * (i + 1) + j + 1) * 3 + k] * 15 + cpuIO[(imageWidth * (i + 1) + j + 2) * 3 + k] * 4 +
	// 													 cpuIO[(imageWidth * (i + 2) + j - 2) * 3 + k] * 1 + cpuIO[(imageWidth * (i + 2) + j - 1) * 3 + k] * 4 + cpuIO[(imageWidth * (i + 2) + j + 0) * 3 + k] * 6 + cpuIO[(imageWidth * (i + 2) + j + 1) * 3 + k] * 4 + cpuIO[(imageWidth * (i + 2) + j + 2) * 3 + k] * 1;

	// 					imgPtr[(imageWidth * (i - 2) + (j - 2)) * 3 + k] = sum / 256;
	// 				}
	// 	}
	// }
	// stbi_write_jpg("./res/CPUres.jpg", imageWidth, imageHeight, 3, imgPtr, 100);
	// std::cout << '\n';
	// {
	// 	nic::ScopeTimerOStream<> t("GPU Shutdown", std::cout);
	// 	Shutdown();
	// 	delete[] gpuIO;
	// }
	// {
	// 	nic::ScopeTimerOStream<> t("CPU shutdow", std::cout);
	// 	delete[] cpuIO;
	// }

	stbi_image_free(imgPtr);

	std::cin.ignore();
}