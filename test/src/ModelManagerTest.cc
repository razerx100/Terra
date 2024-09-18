#include <gtest/gtest.h>
#include <memory>

#include <VKInstanceManager.hpp>
#include <VkDeviceManager.hpp>
#include <ModelManager.hpp>
#include <StagingBufferManager.hpp>
#include <VkDescriptorBuffer.hpp>

namespace Constants
{
	constexpr const char* appName              = "TerraTest";
	constexpr std::uint32_t frameCount         = 2u;
	constexpr std::uint32_t descSetLayoutCount = 2u;
	constexpr std::uint32_t vsSetLayoutIndex   = 0u;
	constexpr std::uint32_t csSetLayoutIndex   = 0u;
	constexpr std::uint32_t fsSetLayoutIndex   = 1u;
	constexpr std::uint32_t meshID             = 0u;
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
	s_instanceManager->DebugLayers().AddDebugCallback(DebugCallbackType::StandardError);
	s_instanceManager->CreateInstance(coreVersion);

	VkInstance vkInstance = s_instanceManager->GetVKInstance();

	s_deviceManager = std::make_unique<VkDeviceManager>();

	{
		VkDeviceExtensionManager& extensionManager = s_deviceManager->ExtensionManager();
		extensionManager.AddExtensions(MemoryManager::GetRequiredExtensions());
		extensionManager.AddExtensions(VkDescriptorBuffer::GetRequiredExtensions());
		extensionManager.AddExtensions(ModelBundleMSIndividual::GetRequiredExtensions());
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
	std::uint32_t GetMaterialIndex() const noexcept override { return 0u; }
	[[nodiscard]]
	std::uint32_t GetDiffuseIndex() const noexcept override { return 0u; }
	[[nodiscard]]
	UVInfo GetDiffuseUVInfo() const noexcept override { return UVInfo{}; }
	[[nodiscard]]
	std::uint32_t GetSpecularIndex() const noexcept { return 0u; }
	[[nodiscard]]
	UVInfo GetSpecularUVInfo() const noexcept { return UVInfo{}; }
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
	std::uint32_t GetMaterialIndex() const noexcept override { return 0u; }
	[[nodiscard]]
	MeshDetailsVS GetMeshDetailsVS() const noexcept override
	{
		return m_details;
	}
	[[nodiscard]]
	std::uint32_t GetDiffuseIndex() const noexcept override { return 0u; }
	[[nodiscard]]
	UVInfo GetDiffuseUVInfo() const noexcept override { return UVInfo{}; }
	[[nodiscard]]
	std::uint32_t GetSpecularIndex() const noexcept { return 0u; }
	[[nodiscard]]
	UVInfo GetSpecularUVInfo() const noexcept { return UVInfo{}; }
};

class ModelBundleDummyVS : public ModelBundleVS
{
	std::uint32_t                         m_meshID = Constants::meshID;
	std::vector<std::shared_ptr<ModelVS>> m_models;

public:
	void AddModel(std::shared_ptr<ModelVS> model) noexcept
	{
		m_models.emplace_back(std::move(model));
	}

	[[nodiscard]]
	std::uint32_t GetMeshIndex() const noexcept override { return m_meshID; }
	[[nodiscard]]
	const std::vector<std::shared_ptr<ModelVS>>& GetModels() const noexcept override
	{
		return m_models;
	}
};

class ModelBundleDummyMS : public ModelBundleMS
{
	std::uint32_t                         m_meshID = Constants::meshID;
	std::vector<std::shared_ptr<ModelMS>> m_models;

public:
	void AddModel(std::shared_ptr<ModelMS> model) noexcept
	{
		m_models.emplace_back(std::move(model));
	}

	[[nodiscard]]
	std::uint32_t GetMeshIndex() const noexcept override { return m_meshID; }
	[[nodiscard]]
	const std::vector<std::shared_ptr<ModelMS>>& GetModels() const noexcept override
	{
		return m_models;
	}
};

class ModelDummyMS : public ModelMS
{
	MeshDetailsMS m_details = {};

public:
	ModelDummyMS() : m_details{} {}

	[[nodiscard]]
	DirectX::XMMATRIX GetModelMatrix() const noexcept override { return {}; }
	[[nodiscard]]
	DirectX::XMFLOAT3 GetModelOffset() const noexcept override { return {}; }
	[[nodiscard]]
	std::uint32_t GetMaterialIndex() const noexcept override { return 0u; }
	[[nodiscard]]
	MeshDetailsMS GetMeshDetailsMS() const noexcept override
	{
		return m_details;
	}
	[[nodiscard]]
	std::uint32_t GetDiffuseIndex() const noexcept override { return 0u; }
	[[nodiscard]]
	UVInfo GetDiffuseUVInfo() const noexcept override { return UVInfo{}; }
	[[nodiscard]]
	std::uint32_t GetSpecularIndex() const noexcept { return 0u; }
	[[nodiscard]]
	UVInfo GetSpecularUVInfo() const noexcept { return UVInfo{}; }
};

class MeshDummyVS : public MeshBundleVS
{
	std::vector<MeshBound>     m_bounds   = { MeshBound{} };
	std::vector<Vertex>        m_vertices = { Vertex{} };
	std::vector<std::uint32_t> m_indices  = { 0u, 1u, 2u };

public:
	[[nodiscard]]
	const std::vector<MeshBound>& GetBounds() const noexcept override
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
	std::vector<Meshlet>       m_meshlets      = { Meshlet{}, Meshlet{} };
	std::vector<MeshBound>     m_bounds        = { MeshBound{} };
	std::vector<Vertex>        m_vertices      = { Vertex{} };
	std::vector<std::uint32_t> m_vertexIndices = { 0u, 1u, 2u };
	std::vector<std::uint32_t> m_primIndices   = { 0u };

public:
	[[nodiscard]]
	const std::vector<MeshBound>& GetBounds() const noexcept override
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
	[[nodiscard]]
	const std::vector<Meshlet>& GetMeshlets() const noexcept override
	{
		return m_meshlets;
	}
};

TEST_F(ModelManagerTest, ModelBufferTest)
{
	VkDevice logicalDevice          = s_deviceManager->GetLogicalDevice();
	VkPhysicalDevice physicalDevice = s_deviceManager->GetPhysicalDevice();

	MemoryManager memoryManager{ physicalDevice, logicalDevice, 20_MB, 200_KB };

	// Single Model Once.
	{
		ModelBuffers modelBuffers{ logicalDevice, &memoryManager, Constants::frameCount, {} };

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
		ModelBuffers modelBuffers{ logicalDevice, &memoryManager, Constants::frameCount, {} };

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
		ModelBuffers modelBuffers{ logicalDevice, &memoryManager, Constants::frameCount, {} };

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
		ModelBuffers modelBuffers{ logicalDevice, &memoryManager, Constants::frameCount, {} };

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

	ThreadPool threadPool{ 2u };

	StagingBufferManager stagingBufferManager{ logicalDevice, &memoryManager, &threadPool, &queueManager };

	VKRenderPass renderPass{ logicalDevice };
	renderPass.Create(RenderPassBuilder{});

	ModelManagerVSIndividual vsIndividual{
		logicalDevice, &memoryManager, queueManager.GetAllIndices(),
		Constants::frameCount
	};
	vsIndividual.SetRenderPass(&renderPass);

	std::vector<VkDescriptorBuffer> descBuffers{};

	for (size_t _ = 0u; _ < Constants::frameCount; ++_)
		descBuffers.emplace_back(
			VkDescriptorBuffer{ logicalDevice, &memoryManager, Constants::descSetLayoutCount }
		);

	vsIndividual.SetDescriptorBufferLayout(
		descBuffers, Constants::vsSetLayoutIndex, Constants::fsSetLayoutIndex
	);

	for (auto& descBuffer : descBuffers)
		descBuffer.CreateBuffer();

	vsIndividual.CreatePipelineLayout(descBuffers.front());

	TemporaryDataBufferGPU tempDataBuffer{};

	{
		auto meshVS = std::make_unique<MeshDummyVS>();
		std::uint32_t index = vsIndividual.AddMeshBundle(
			std::move(meshVS), stagingBufferManager, tempDataBuffer
		);
		EXPECT_EQ(index, 0u) << "Index isn't 0u";
	}
	{
		auto meshVS = std::make_unique<MeshDummyVS>();
		std::uint32_t index = vsIndividual.AddMeshBundle(
			std::move(meshVS), stagingBufferManager, tempDataBuffer
		);
		EXPECT_EQ(index, 1u) << "Index isn't 1u";
	}
	vsIndividual.RemoveMeshBundle(0u);
	{
		auto meshVS = std::make_unique<MeshDummyVS>();
		std::uint32_t index = vsIndividual.AddMeshBundle(
			std::move(meshVS), stagingBufferManager, tempDataBuffer
		);
		EXPECT_EQ(index, 0u) << "Index isn't 0u";
	}

	{
		auto modelVS       = std::make_shared<ModelDummyVS>();
		auto modelBundleVS = std::make_shared<ModelBundleDummyVS>();

		modelBundleVS->AddModel(std::move(modelVS));

		std::uint32_t index = vsIndividual.AddModelBundle(
			std::move(modelBundleVS), L"", tempDataBuffer
		);
		EXPECT_EQ(index, 0u) << "Index isn't 0.";
	}
	{
		auto modelVS       = std::make_shared<ModelDummyVS>();
		auto modelBundleVS = std::make_shared<ModelBundleDummyVS>();

		modelBundleVS->AddModel(std::move(modelVS));

		std::uint32_t index = vsIndividual.AddModelBundle(
			std::move(modelBundleVS), L"", tempDataBuffer
		);
		EXPECT_EQ(index, 1u) << "Index isn't 1.";
	}
	{
		auto modelBundleVS = std::make_shared<ModelBundleDummyVS>();

		for (size_t index = 0u; index < 5u; ++index)
			modelBundleVS->AddModel(std::make_shared<ModelDummyVS>());

		std::uint32_t index = vsIndividual.AddModelBundle(
			std::move(modelBundleVS), L"H", tempDataBuffer
		);
		EXPECT_EQ(index, 2u) << "Index isn't 2.";
	}
	vsIndividual.RemoveModelBundle(1u);
	{
		auto modelBundleVS = std::make_shared<ModelBundleDummyVS>();

		for (size_t index = 0u; index < 7u; ++index)
			modelBundleVS->AddModel(std::make_shared<ModelDummyVS>());

		std::uint32_t index = vsIndividual.AddModelBundle(
			std::move(modelBundleVS), L"H", tempDataBuffer
		);
		EXPECT_EQ(index, 1u) << "Index isn't 1.";
	}
}

TEST_F(ModelManagerTest, ModelManagerVSIndirectTest)
{
	VkDevice logicalDevice          = s_deviceManager->GetLogicalDevice();
	VkPhysicalDevice physicalDevice = s_deviceManager->GetPhysicalDevice();

	MemoryManager memoryManager{ physicalDevice, logicalDevice, 20_MB, 200_KB };

	const auto& queueManager = s_deviceManager->GetQueueFamilyManager();

	ThreadPool threadPool{ 2u };

	StagingBufferManager stagingBufferManager{ logicalDevice, &memoryManager,  &threadPool, &queueManager };

	VKRenderPass renderPass{ logicalDevice };
	renderPass.Create(RenderPassBuilder{});

	ModelManagerVSIndirect vsIndirect{
		logicalDevice, &memoryManager, &stagingBufferManager, queueManager.GetAllIndices(),
		Constants::frameCount
	};
	vsIndirect.SetRenderPass(&renderPass);

	std::vector<VkDescriptorBuffer> descBuffersVS{};
	std::vector<VkDescriptorBuffer> descBuffersCS{};

	for (size_t _ = 0u; _ < Constants::frameCount; ++_)
	{
		descBuffersVS.emplace_back(
			VkDescriptorBuffer{ logicalDevice, &memoryManager, Constants::descSetLayoutCount }
		);
		descBuffersCS.emplace_back(
			VkDescriptorBuffer{ logicalDevice, &memoryManager, Constants::descSetLayoutCount }
		);
	}

	vsIndirect.SetDescriptorBufferLayoutVS(
		descBuffersVS, Constants::vsSetLayoutIndex, Constants::fsSetLayoutIndex
	);
	vsIndirect.SetDescriptorBufferLayoutCS(descBuffersCS, Constants::csSetLayoutIndex);

	for (auto& descBuffer : descBuffersVS)
		descBuffer.CreateBuffer();
	for (auto& descBuffer : descBuffersCS)
		descBuffer.CreateBuffer();

	vsIndirect.CreatePipelineLayout(descBuffersVS.front());
	vsIndirect.CreatePipelineCS(descBuffersCS.front());

	TemporaryDataBufferGPU tempDataBuffer{};

	{
		auto meshVS = std::make_unique<MeshDummyVS>();
		std::uint32_t index = vsIndirect.AddMeshBundle(
			std::move(meshVS), stagingBufferManager, tempDataBuffer
		);
		EXPECT_EQ(index, 0u) << "Index isn't 0u";
	}
	{
		auto meshVS = std::make_unique<MeshDummyVS>();
		std::uint32_t index = vsIndirect.AddMeshBundle(
			std::move(meshVS), stagingBufferManager, tempDataBuffer
		);
		EXPECT_EQ(index, 1u) << "Index isn't 1u";
	}
	vsIndirect.RemoveMeshBundle(0u);
	{
		auto meshVS = std::make_unique<MeshDummyVS>();
		std::uint32_t index = vsIndirect.AddMeshBundle(
			std::move(meshVS), stagingBufferManager, tempDataBuffer
		);
		EXPECT_EQ(index, 0u) << "Index isn't 0u";
	}

	{
		auto modelVS       = std::make_shared<ModelDummyVS>();
		auto modelBundleVS = std::make_shared<ModelBundleDummyVS>();

		modelBundleVS->AddModel(std::move(modelVS));

		std::uint32_t index = vsIndirect.AddModelBundle(
			std::move(modelBundleVS), L"", tempDataBuffer
		);
		EXPECT_EQ(index, 0u) << "Index isn't 0.";
	}
	{
		auto modelVS       = std::make_shared<ModelDummyVS>();
		auto modelBundleVS = std::make_shared<ModelBundleDummyVS>();

		modelBundleVS->AddModel(std::move(modelVS));

		std::uint32_t index = vsIndirect.AddModelBundle(
			std::move(modelBundleVS), L"A", tempDataBuffer
		);
		EXPECT_EQ(index, 1u) << "Index isn't 1.";
	}
	{
		auto modelBundleVS = std::make_shared<ModelBundleDummyVS>();

		for (size_t index = 0u; index < 5u; ++index)
			modelBundleVS->AddModel(std::make_shared<ModelDummyVS>());

		std::uint32_t index = vsIndirect.AddModelBundle(
			std::move(modelBundleVS), L"H", tempDataBuffer
		);
		EXPECT_EQ(index, 2u) << "Index isn't 2.";
	}
	vsIndirect.RemoveModelBundle(1u);
	{
		auto modelBundleVS = std::make_shared<ModelBundleDummyVS>();

		for (size_t index = 0u; index < 7u; ++index)
			modelBundleVS->AddModel(std::make_shared<ModelDummyVS>());

		std::uint32_t index = vsIndirect.AddModelBundle(
			std::move(modelBundleVS), L"H", tempDataBuffer
		);
		EXPECT_EQ(index, 1u) << "Index isn't 1.";

		vsIndirect.ChangePSO(index, L"A");
	}
	{
		auto modelVS       = std::make_shared<ModelDummyVS>();
		auto modelBundleVS = std::make_shared<ModelBundleDummyVS>();

		modelBundleVS->AddModel(std::move(modelVS));

		std::uint32_t index = vsIndirect.AddModelBundle(
			std::move(modelBundleVS), L"H", tempDataBuffer
		);
		EXPECT_EQ(index, 13u) << "Index isn't 13.";
	}
	{
		auto modelVS       = std::make_shared<ModelDummyVS>();
		auto modelBundleVS = std::make_shared<ModelBundleDummyVS>();

		modelBundleVS->AddModel(std::move(modelVS));

		std::uint32_t index = vsIndirect.AddModelBundle(
			std::move(modelBundleVS), L"A", tempDataBuffer
		);
		EXPECT_EQ(index, 14u) << "Index isn't 14.";
	}
}

TEST_F(ModelManagerTest, ModelManagerMS)
{
	VkDevice logicalDevice          = s_deviceManager->GetLogicalDevice();
	VkPhysicalDevice physicalDevice = s_deviceManager->GetPhysicalDevice();

	MemoryManager memoryManager{ physicalDevice, logicalDevice, 20_MB, 200_KB };

	const auto& queueManager = s_deviceManager->GetQueueFamilyManager();

	ThreadPool threadPool{ 2u };

	StagingBufferManager stagingBufferManager{ logicalDevice, &memoryManager, &threadPool, &queueManager };

	VKRenderPass renderPass{ logicalDevice };
	renderPass.Create(RenderPassBuilder{});

	ModelManagerMS managerMS{
		logicalDevice, &memoryManager, &stagingBufferManager, queueManager.GetAllIndices(),
		Constants::frameCount
	};
	managerMS.SetRenderPass(&renderPass);

	std::vector<VkDescriptorBuffer> descBuffers{};

	for (size_t _ = 0u; _ < Constants::frameCount; ++_)
		descBuffers.emplace_back(
			VkDescriptorBuffer{ logicalDevice, &memoryManager, Constants::descSetLayoutCount }
		);

	managerMS.SetDescriptorBufferLayout(
		descBuffers, Constants::vsSetLayoutIndex, Constants::fsSetLayoutIndex
	);

	for (auto& descBuffer : descBuffers)
		descBuffer.CreateBuffer();

	managerMS.CreatePipelineLayout(descBuffers.front());

	TemporaryDataBufferGPU tempDataBuffer{};

	{
		auto meshMS = std::make_unique<MeshDummyMS>();
		std::uint32_t index = managerMS.AddMeshBundle(
			std::move(meshMS), stagingBufferManager, tempDataBuffer
		);
		EXPECT_EQ(index, 0u) << "Index isn't 0u";
	}
	{
		auto meshMS = std::make_unique<MeshDummyMS>();
		std::uint32_t index = managerMS.AddMeshBundle(
			std::move(meshMS), stagingBufferManager, tempDataBuffer
		);
		EXPECT_EQ(index, 1u) << "Index isn't 1u";
	}
	managerMS.RemoveMeshBundle(0u);
	{
		auto meshMS = std::make_unique<MeshDummyMS>();
		std::uint32_t index = managerMS.AddMeshBundle(
			std::move(meshMS), stagingBufferManager, tempDataBuffer
		);
		EXPECT_EQ(index, 0u) << "Index isn't 0u";
	}
	{
		auto meshMS = std::make_unique<MeshDummyMS>();
		std::uint32_t index = managerMS.AddMeshBundle(
			std::move(meshMS), stagingBufferManager, tempDataBuffer
		);
		EXPECT_EQ(index, 2u) << "Index isn't 2u";
	}

	{
		auto modelMS       = std::make_shared<ModelDummyMS>();
		auto modelBundleMS = std::make_shared<ModelBundleDummyMS>();

		modelBundleMS->AddModel(std::move(modelMS));

		std::uint32_t index = managerMS.AddModelBundle(
			std::move(modelBundleMS), L"", tempDataBuffer
		);
		EXPECT_EQ(index, 0u) << "Index isn't 0.";
	}
	{
		auto modelMS       = std::make_shared<ModelDummyMS>();
		auto modelBundleMS = std::make_shared<ModelBundleDummyMS>();

		modelBundleMS->AddModel(std::move(modelMS));

		std::uint32_t index = managerMS.AddModelBundle(
			std::move(modelBundleMS), L"", tempDataBuffer
		);
		EXPECT_EQ(index, 1u) << "Index isn't 1.";
	}
	{
		auto modelBundleMS = std::make_shared<ModelBundleDummyMS>();

		for (size_t index = 0u; index < 5u; ++index)
			modelBundleMS->AddModel(std::make_shared<ModelDummyMS>());

		std::uint32_t index = managerMS.AddModelBundle(
			std::move(modelBundleMS), L"H", tempDataBuffer
		);
		EXPECT_EQ(index, 2u) << "Index isn't 2.";
	}
	managerMS.RemoveModelBundle(1u);
	{
		auto modelBundleMS = std::make_shared<ModelBundleDummyMS>();

		for (size_t index = 0u; index < 7u; ++index)
			modelBundleMS->AddModel(std::make_shared<ModelDummyMS>());

		std::uint32_t index = managerMS.AddModelBundle(
			std::move(modelBundleMS), L"H", tempDataBuffer
		);
		EXPECT_EQ(index, 1u) << "Index isn't 1.";
	}
}

