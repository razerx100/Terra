#include <gtest/gtest.h>
#include <memory>

#include <VKInstanceManager.hpp>
#include <VkDeviceManager.hpp>
#include <ModelManager.hpp>

namespace Constants
{
	constexpr const char* appName      = "TerraTest";
	constexpr std::uint32_t frameCount = 2u;
}

class ModelManagerTest : public ::testing::Test
{
protected:
	static void SetUpTestSuite();
	static void TearDownTestSuite();

protected:
	inline static std::unique_ptr<VkInstanceManager> s_instanceManager;
	inline static std::unique_ptr<VkDeviceManager>   s_deviceManager;
};

void ModelManagerTest::SetUpTestSuite()
{
	const CoreVersion coreVersion = CoreVersion::V1_3;

	s_instanceManager = std::make_unique<VkInstanceManager>(Constants::appName);
	s_instanceManager->DebugLayers().AddDebugCallback(DebugCallbackType::standardError);
	s_instanceManager->CreateInstance(coreVersion);

	VkInstance vkInstance = s_instanceManager->GetVKInstance();

	s_deviceManager = std::make_unique<VkDeviceManager>();

	s_deviceManager->SetDeviceFeatures(coreVersion)
		.SetPhysicalDeviceAutomatic(vkInstance)
		.CreateLogicalDevice();
}

void ModelManagerTest::TearDownTestSuite()
{
	s_deviceManager.reset();
	s_instanceManager.reset();
}

class ModelDummy : public Model
{
public:
	[[nodiscard]]
	DirectX::XMMATRIX GetModelMatrix() const noexcept override { return {}; }
	[[nodiscard]]
	DirectX::XMFLOAT3 GetModelOffset() const noexcept override { return {}; }
	[[nodiscard]]
	bool IsLightSource() const noexcept override { return false; }
	[[nodiscard]]
	std::uint32_t GetMeshIndex() const noexcept override { return 0u; }
	[[nodiscard]]
	std::uint32_t GetMaterialIndex() const noexcept override { return 0u; }
};

TEST_F(ModelManagerTest, ModelBufferTest)
{
	VkDevice logicalDevice          = s_deviceManager->GetLogicalDevice();
	VkPhysicalDevice physicalDevice = s_deviceManager->GetPhysicalDevice();

	MemoryManager memoryManager{ physicalDevice, logicalDevice, 20_MB, 200_KB };

	// Single Model Once.
	{
		ModelBuffers modelBuffers{ logicalDevice, &memoryManager, Constants::frameCount };

		std::vector<std::shared_ptr<Model>> models{};
		for (size_t index = 0u; index < 6u; ++index)
			models.emplace_back(std::make_shared<ModelDummy>());

		for (size_t index = 0u; index < std::size(models); ++index)
		{
			const size_t modelIndex = modelBuffers.AddModel(std::move(models.at(index)));

			EXPECT_EQ(modelIndex, index) << "Model index doesn't match.";
		}
	}

	// Single Model with delete.
	{
		ModelBuffers modelBuffers{ logicalDevice, &memoryManager, Constants::frameCount };

		std::vector<std::shared_ptr<Model>> models{};
		for (size_t index = 0u; index < 6u; ++index)
			models.emplace_back(std::make_shared<ModelDummy>());

		{
			const size_t modelIndex = modelBuffers.AddModel(std::move(models.at(0u)));

			modelBuffers.RemoveModel(modelIndex);
		}

		for (size_t index = 1u; index < std::size(models); ++index)
		{
			const size_t modelIndex = modelBuffers.AddModel(std::move(models.at(index)));

			EXPECT_EQ(modelIndex, index - 1u) << "Model index doesn't match.";
		}
	}

	// Multiple Model Once.
	{
		ModelBuffers modelBuffers{ logicalDevice, &memoryManager, Constants::frameCount };

		{
			std::vector<std::shared_ptr<Model>> models{};
			for (size_t index = 0u; index < 6u; ++index)
				models.emplace_back(std::make_shared<ModelDummy>());

			std::vector<size_t> modelIndices = modelBuffers.AddModels(std::move(models));

			for (size_t index = 0u; index < std::size(modelIndices); ++index)
			{
				const size_t modelIndex = modelIndices.at(index);

				EXPECT_EQ(modelIndex, index) << "Model index doesn't match.";
			}
		}
		{
			std::vector<std::shared_ptr<Model>> models{};
			for (size_t index = 0u; index < 4u; ++index)
				models.emplace_back(std::make_shared<ModelDummy>());

			std::vector<size_t> modelIndices = modelBuffers.AddModels(std::move(models));

			for (size_t index = 6u; index < std::size(modelIndices); ++index)
			{
				const size_t modelIndex = modelIndices.at(index);

				EXPECT_EQ(modelIndex, index) << "Model index doesn't match.";
			}
		}
	}

	// Multiple Model with delete.
	{
		ModelBuffers modelBuffers{ logicalDevice, &memoryManager, Constants::frameCount };

		{
			std::vector<std::shared_ptr<Model>> models{};
			for (size_t index = 0u; index < 6u; ++index)
				models.emplace_back(std::make_shared<ModelDummy>());

			std::vector<size_t> modelIndices = modelBuffers.AddModels(std::move(models));

			for (size_t index = 0u; index < std::size(modelIndices); ++index)
			{
				const size_t modelIndex = modelIndices.at(index);

				EXPECT_EQ(modelIndex, index) << "Model index doesn't match.";
			}
		}
		modelBuffers.RemoveModel(5u);
		{
			std::vector<std::shared_ptr<Model>> models{};
			for (size_t index = 0u; index < 5u; ++index)
				models.emplace_back(std::make_shared<ModelDummy>());

			std::vector<size_t> modelIndices = modelBuffers.AddModels(std::move(models));

			for (size_t index = 5u; index < std::size(modelIndices); ++index)
			{
				const size_t modelIndex = modelIndices.at(index);

				EXPECT_EQ(modelIndex, index) << "Model index doesn't match.";
			}
		}
	}
}
