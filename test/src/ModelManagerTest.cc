#include <gtest/gtest.h>
#include <memory>

#include <VKInstanceManager.hpp>
#include <VkDeviceManager.hpp>
#include <ModelManager.hpp>
#include <StagingBufferManager.hpp>
#include <VkDescriptorBuffer.hpp>

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

	{
		VkDeviceExtensionManager& extensionManager = s_deviceManager->ExtensionManager();
		extensionManager.AddExtensions(ModelBundleMS::GetRequiredExtensions());
		extensionManager.AddExtensions(VkDescriptorBuffer::GetRequiredExtensions());
	}

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

class ModelDummyVS : public ModelVS
{
	MeshDetailsVS m_details = {};

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
	[[nodiscard]]
	const MeshDetailsVS& GetMeshDetailsVS() const noexcept override
	{
		return m_details;
	}
};

class ModelDummyMS : public ModelMS
{
	MeshDetailsMS m_details = { { Meshlet{}, Meshlet{} } };

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
	[[nodiscard]]
	MeshDetailsMS& GetMeshDetailsMS() noexcept override
	{
		return m_details;
	}
};

class MeshDummyVS : public MeshBundleVS
{
	std::vector<MeshBounds>    m_bounds   = { MeshBounds{} };
	std::vector<Vertex>        m_vertices = { Vertex{} };
	std::vector<std::uint32_t> m_indices  = { 0u, 1u, 2u };

public:
	[[nodiscard]]
	const std::vector<MeshBounds>& GetBounds() const noexcept override
	{
		return m_bounds;
	}
	[[nodiscard]]
	const std::vector<Vertex>& GetVertices() const noexcept override
	{
		return m_vertices;
	}

	[[nodiscard]]
	const std::vector<std::uint32_t>& GetIndices() const noexcept override
	{
		return m_indices;
	}

	void CleanUpVertices() noexcept
	{
		m_vertices = std::vector<Vertex>{};
	}
};

class MeshDummyMS : public MeshBundleMS
{
	std::vector<MeshBounds>    m_bounds        = { MeshBounds{} };
	std::vector<Vertex>        m_vertices      = { Vertex{} };
	std::vector<std::uint32_t> m_vertexIndices = { 0u, 1u, 2u };
	std::vector<std::uint32_t> m_primIndices   = { 0u };

public:
	[[nodiscard]]
	const std::vector<MeshBounds>& GetBounds() const noexcept override
	{
		return m_bounds;
	}
	[[nodiscard]]
	const std::vector<Vertex>& GetVertices() const noexcept override
	{
		return m_vertices;
	}

	void CleanUpVertices() noexcept
	{
		m_vertices = std::vector<Vertex>{};
	}

	[[nodiscard]]
	const std::vector<std::uint32_t>& GetVertexIndices() const noexcept override
	{
		return m_vertexIndices;
	}
	[[nodiscard]]
	const std::vector<std::uint32_t>& GetPrimIndices() const noexcept override
	{
		return m_primIndices;
	}
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
			const size_t modelIndex = modelBuffers.Add(std::move(models.at(index)));

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
			const size_t modelIndex = modelBuffers.Add(std::move(models.at(0u)));

			modelBuffers.Remove(modelIndex);
		}

		for (size_t index = 1u; index < std::size(models); ++index)
		{
			const size_t modelIndex = modelBuffers.Add(std::move(models.at(index)));

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

			std::vector<size_t> modelIndices = modelBuffers.AddMultiple(std::move(models));

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

			std::vector<size_t> modelIndices = modelBuffers.AddMultiple(std::move(models));

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

			std::vector<size_t> modelIndices = modelBuffers.AddMultiple(std::move(models));

			for (size_t index = 0u; index < std::size(modelIndices); ++index)
			{
				const size_t modelIndex = modelIndices.at(index);

				EXPECT_EQ(modelIndex, index) << "Model index doesn't match.";
			}
		}
		modelBuffers.Remove(5u);
		{
			std::vector<std::shared_ptr<Model>> models{};
			for (size_t index = 0u; index < 5u; ++index)
				models.emplace_back(std::make_shared<ModelDummy>());

			std::vector<size_t> modelIndices = modelBuffers.AddMultiple(std::move(models));

			for (size_t index = 5u; index < std::size(modelIndices); ++index)
			{
				const size_t modelIndex = modelIndices.at(index);

				EXPECT_EQ(modelIndex, index) << "Model index doesn't match.";
			}
		}
	}
}

TEST_F(ModelManagerTest, ModelManagerVSIndividualTest)
{
	VkDevice logicalDevice          = s_deviceManager->GetLogicalDevice();
	VkPhysicalDevice physicalDevice = s_deviceManager->GetPhysicalDevice();

	MemoryManager memoryManager{ physicalDevice, logicalDevice, 20_MB, 200_KB };

	const auto& queueManager = s_deviceManager->GetQueueFamilyManager();

	VkCommandQueue commandQueue{
		logicalDevice, queueManager.GetQueue(QueueType::GraphicsQueue),
		queueManager.GetIndex(QueueType::GraphicsQueue)
	};
	commandQueue.CreateCommandBuffers(Constants::frameCount);

	ThreadPool threadPool{ 2u };

	StagingBufferManager stagingBufferManager{
		logicalDevice, &memoryManager, &commandQueue, &threadPool, &queueManager
	};

	VKRenderPass renderPass{ logicalDevice };
	renderPass.Create(RenderPassBuilder{});

	ModelManagerVSIndividual vsIndividual{ logicalDevice, &memoryManager, Constants::frameCount };
	vsIndividual.SetRenderPass(&renderPass);

	std::vector<VkDescriptorBuffer> descBuffers{};

	for (size_t _ = 0u; _ < Constants::frameCount; ++_)
		descBuffers.emplace_back(VkDescriptorBuffer{ logicalDevice, &memoryManager });

	vsIndividual.SetDescriptorBufferLayout(descBuffers);

	for (auto& descBuffer : descBuffers)
		descBuffer.CreateBuffer();

	vsIndividual.CreatePipelineLayout(descBuffers.front());

	{
		auto meshVS = std::make_unique<MeshDummyVS>();
		vsIndividual.AddMeshBundle(std::move(meshVS), stagingBufferManager);
	}

	{
		auto modelVS = std::make_shared<ModelDummyVS>();

		std::uint32_t index = vsIndividual.AddModel(std::move(modelVS), L"");
		EXPECT_EQ(index, 0u) << "Index isn't 0.";
	}
	{
		auto modelVS = std::make_shared<ModelDummyVS>();

		std::uint32_t index = vsIndividual.AddModel(std::move(modelVS), L"");
		EXPECT_EQ(index, 1u) << "Index isn't 1.";
	}
	{
		std::vector<std::shared_ptr<ModelVS>> modelsVS{};

		for (size_t index = 0u; index < 5u; ++index)
			modelsVS.emplace_back(std::make_shared<ModelDummyVS>());

		std::uint32_t index = vsIndividual.AddModelBundle(std::move(modelsVS), L"H");
		EXPECT_EQ(index, 2u) << "Index isn't 2.";
	}
	vsIndividual.RemoveBundle(1u);
	{
		std::vector<std::shared_ptr<ModelVS>> modelsVS{};

		for (size_t index = 0u; index < 7u; ++index)
			modelsVS.emplace_back(std::make_shared<ModelDummyVS>());

		std::uint32_t index = vsIndividual.AddModelBundle(std::move(modelsVS), L"H");
		EXPECT_EQ(index, 1u) << "Index isn't 1.";
	}
}

TEST_F(ModelManagerTest, ModelManagerVSIndirectTest)
{
	VkDevice logicalDevice          = s_deviceManager->GetLogicalDevice();
	VkPhysicalDevice physicalDevice = s_deviceManager->GetPhysicalDevice();

	MemoryManager memoryManager{ physicalDevice, logicalDevice, 20_MB, 200_KB };

	const auto& queueManager = s_deviceManager->GetQueueFamilyManager();

	VkCommandQueue commandQueue{
		logicalDevice, queueManager.GetQueue(QueueType::GraphicsQueue),
		queueManager.GetIndex(QueueType::GraphicsQueue)
	};
	commandQueue.CreateCommandBuffers(Constants::frameCount);

	ThreadPool threadPool{ 2u };

	StagingBufferManager stagingBufferManager{
		logicalDevice, &memoryManager, &commandQueue, &threadPool, &queueManager
	};

	VKRenderPass renderPass{ logicalDevice };
	renderPass.Create(RenderPassBuilder{});

	ModelManagerVSIndirect vsIndirect{
		logicalDevice, &memoryManager, &stagingBufferManager, queueManager.GetAllIndices(),
		Constants::frameCount
	};
	vsIndirect.SetRenderPass(&renderPass);

	std::vector<VkDescriptorBuffer> descBuffers{};

	for (size_t _ = 0u; _ < Constants::frameCount; ++_)
		descBuffers.emplace_back(VkDescriptorBuffer{ logicalDevice, &memoryManager });

	vsIndirect.SetDescriptorBufferLayoutVS(descBuffers);

	for (auto& descBuffer : descBuffers)
		descBuffer.CreateBuffer();

	vsIndirect.CreatePipelineLayout(descBuffers.front());

	{
		auto meshVS = std::make_unique<MeshDummyVS>();
		vsIndirect.AddMeshBundle(std::move(meshVS), stagingBufferManager);
	}

	{
		auto modelVS = std::make_shared<ModelDummyVS>();

		std::uint32_t index = vsIndirect.AddModel(std::move(modelVS), L"");
		EXPECT_EQ(index, 0u) << "Index isn't 0.";
	}
	{
		auto modelVS = std::make_shared<ModelDummyVS>();

		std::uint32_t index = vsIndirect.AddModel(std::move(modelVS), L"A");
		EXPECT_EQ(index, 1u) << "Index isn't 1.";
	}
	{
		std::vector<std::shared_ptr<ModelVS>> modelsVS{};

		for (size_t index = 0u; index < 5u; ++index)
			modelsVS.emplace_back(std::make_shared<ModelDummyVS>());

		std::uint32_t index = vsIndirect.AddModelBundle(std::move(modelsVS), L"H");
		EXPECT_EQ(index, 2u) << "Index isn't 2.";
	}
	vsIndirect.RemoveBundle(1u);
	{
		std::vector<std::shared_ptr<ModelVS>> modelsVS{};

		for (size_t index = 0u; index < 7u; ++index)
			modelsVS.emplace_back(std::make_shared<ModelDummyVS>());

		std::uint32_t index = vsIndirect.AddModelBundle(std::move(modelsVS), L"H");
		EXPECT_EQ(index, 1u) << "Index isn't 1.";

		vsIndirect.ChangePSO(index, L"A");
	}
	{
		auto modelVS = std::make_shared<ModelDummyVS>();

		std::uint32_t index = vsIndirect.AddModel(std::move(modelVS), L"H");
		EXPECT_EQ(index, 11u) << "Index isn't 11.";
	}
	{
		auto modelVS = std::make_shared<ModelDummyVS>();

		std::uint32_t index = vsIndirect.AddModel(std::move(modelVS), L"A");
		EXPECT_EQ(index, 12u) << "Index isn't 12.";
	}
}

TEST_F(ModelManagerTest, ModelManagerMS)
{
	VkDevice logicalDevice          = s_deviceManager->GetLogicalDevice();
	VkPhysicalDevice physicalDevice = s_deviceManager->GetPhysicalDevice();

	MemoryManager memoryManager{ physicalDevice, logicalDevice, 20_MB, 200_KB };

	const auto& queueManager = s_deviceManager->GetQueueFamilyManager();

	VkCommandQueue commandQueue{
		logicalDevice, queueManager.GetQueue(QueueType::GraphicsQueue),
		queueManager.GetIndex(QueueType::GraphicsQueue)
	};
	commandQueue.CreateCommandBuffers(Constants::frameCount);

	ThreadPool threadPool{ 2u };

	StagingBufferManager stagingBufferManager{
		logicalDevice, &memoryManager, &commandQueue, &threadPool, &queueManager
	};

	VKRenderPass renderPass{ logicalDevice };
	renderPass.Create(RenderPassBuilder{});

	ModelManagerMS managerMS{
		logicalDevice, &memoryManager, &stagingBufferManager, Constants::frameCount
	};
	managerMS.SetRenderPass(&renderPass);

	std::vector<VkDescriptorBuffer> descBuffers{};

	for (size_t _ = 0u; _ < Constants::frameCount; ++_)
		descBuffers.emplace_back(VkDescriptorBuffer{ logicalDevice, &memoryManager });

	managerMS.SetDescriptorBufferLayout(descBuffers);

	for (auto& descBuffer : descBuffers)
		descBuffer.CreateBuffer();

	managerMS.CreatePipelineLayout(descBuffers.front());

	{
		auto meshMS = std::make_unique<MeshDummyMS>();
		managerMS.AddMeshBundle(std::move(meshMS), stagingBufferManager);
	}

	{
		auto modelMS = std::make_shared<ModelDummyMS>();

		std::uint32_t index = managerMS.AddModel(std::move(modelMS), L"");
		EXPECT_EQ(index, 0u) << "Index isn't 0.";
	}
	{
		auto modelMS = std::make_shared<ModelDummyMS>();

		std::uint32_t index = managerMS.AddModel(std::move(modelMS), L"");
		EXPECT_EQ(index, 1u) << "Index isn't 1.";
	}
	{
		std::vector<std::shared_ptr<ModelMS>> modelsMS{};

		for (size_t index = 0u; index < 5u; ++index)
			modelsMS.emplace_back(std::make_shared<ModelDummyMS>());

		std::uint32_t index = managerMS.AddModelBundle(std::move(modelsMS), L"H");
		EXPECT_EQ(index, 2u) << "Index isn't 2.";
	}
	managerMS.RemoveBundle(1u);
	{
		std::vector<std::shared_ptr<ModelMS>> modelsMS{};

		for (size_t index = 0u; index < 7u; ++index)
			modelsMS.emplace_back(std::make_shared<ModelDummyMS>());

		std::uint32_t index = managerMS.AddModelBundle(std::move(modelsMS), L"H");
		EXPECT_EQ(index, 1u) << "Index isn't 1.";
	}
}

