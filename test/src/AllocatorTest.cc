#include <gtest/gtest.h>
#include <memory>

#include <VKInstanceManager.hpp>
#include <SurfaceManagerWin32.hpp>
#include <VkDeviceManager.hpp>
#include <VkResources.hpp>

namespace Constants
{
	constexpr std::uint32_t width  = 1920u;
	constexpr std::uint32_t height = 1920u;
	constexpr const char* appName  = "TerraTest";
}

/*class AllocatorTest : public ::testing::Test
{
protected:
	static void SetUpTestSuite();
	static void TearDownTestSuite();

protected:
	inline static std::unique_ptr<VkInstanceManager>   s_instanceManager;
	inline static std::unique_ptr<VkDeviceManager>     s_deviceManager;
	inline static std::unique_ptr<SurfaceManagerWin32> s_surface;
	inline static SimpleWindow                         s_window{
		Constants::width, Constants::height, Constants::appName
	};
};

void AllocatorTest::SetUpTestSuite()
{
	s_instanceManager = std::make_unique<VkInstanceManager>(Constants::appName);
	s_instanceManager->CreateInstance();
	VkInstance vkInstance = s_instanceManager->GetVKInstance();

	s_surface = std::make_unique<SurfaceManagerWin32>(
		vkInstance, s_window.GetWindowHandle(), s_window.GetModuleInstance()
	);

	s_deviceManager = std::make_unique<VkDeviceManager>();
	s_deviceManager->FindPhysicalDevice(vkInstance, s_surface->GetSurface()).CreateLogicalDevice(false);
}

void AllocatorTest::TearDownTestSuite()
{
	s_deviceManager.reset();
	s_surface.reset();
	s_instanceManager.reset();
}

TEST_F(AllocatorTest, VkAllocatorTest)
{
	VkDevice logicalDevice = s_deviceManager->GetLogicalDevice();
	VkPhysicalDevice physicalDevice = s_deviceManager->GetPhysicalDevice();

	MemoryManager memoryManager{ physicalDevice, logicalDevice, 2_MB, 200_KB };

	Buffer buffer{ logicalDevice, memoryManager, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT };
	buffer.Create(logicalDevice, 1_KB, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, {});

	EXPECT_NE(buffer.Get(), VK_NULL_HANDLE) << "Buffer wasn't initialised";
	EXPECT_EQ(buffer.CPUHandle(), nullptr) << "CPU Pointer isn't null.";
	EXPECT_EQ(buffer.Size(), 1_KB) << "BufferSize doesn't match.";
}*/
