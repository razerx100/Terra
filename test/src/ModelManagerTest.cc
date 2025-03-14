#include <gtest/gtest.h>
#include <memory>

#include <VKInstanceManager.hpp>
#include <VkDeviceManager.hpp>
#include <ModelManager.hpp>
#include <VkModelBuffer.hpp>
#include <StagingBufferManager.hpp>
#include <VkDescriptorBuffer.hpp>
#include <VKRenderPass.hpp>

namespace Constants
{
	constexpr const char* appName              = "TerraTest";
	constexpr std::uint32_t frameCount         = 2u;
	constexpr std::uint32_t descSetLayoutCount = 2u;
	constexpr std::uint32_t vsSetLayoutIndex   = 0u;
	constexpr std::uint32_t csSetLayoutIndex   = 0u;
	constexpr std::uint32_t fsSetLayoutIndex   = 1u;
	constexpr std::uint32_t meshBundleID       = 0u;
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
		extensionManager.AddExtensions(PipelineModelsMSIndividual::GetRequiredExtensions());
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
	std::uint32_t m_psoIndex = 0u;
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
	std::uint32_t GetMeshIndex() const noexcept override { return 0u; }
	[[nodiscard]]
	UVInfo GetDiffuseUVInfo() const noexcept override { return UVInfo{}; }
	[[nodiscard]]
	std::uint32_t GetSpecularIndex() const noexcept override { return 0u; }
	[[nodiscard]]
	UVInfo GetSpecularUVInfo() const noexcept override { return UVInfo{}; }
	[[nodiscard]]
	float GetModelScale() const noexcept override { return 1.f; }
	[[nodiscard]]
	std::uint32_t GetPipelineIndex() const noexcept override { return m_psoIndex; }
	[[nodiscard]]
	bool IsVisible() const noexcept override { return true; }

	void SetPipelineIndex(std::uint32_t psoIndex) noexcept { m_psoIndex = psoIndex; }
};

class ModelBundleDummy : public ModelBundle
{
	std::uint32_t                       m_meshBundleID = Constants::meshBundleID;
	std::vector<std::shared_ptr<Model>> m_models;

public:
	void AddModel(std::shared_ptr<Model> model) noexcept
	{
		m_models.emplace_back(std::move(model));
	}

	[[nodiscard]]
	std::uint32_t GetMeshBundleIndex() const noexcept override { return m_meshBundleID; }
	[[nodiscard]]
	const std::vector<std::shared_ptr<Model>>& GetModels() const noexcept override
	{
		return m_models;
	}
};

class MeshBundleTemporaryDummy : public MeshBundleTemporary
{
	std::vector<MeshletDetails> m_meshletDetails = { MeshletDetails{}, MeshletDetails{} };
	std::vector<Vertex>         m_vertices       = { Vertex{} };
	std::vector<std::uint32_t>  m_vertexIndices  = { 0u, 1u, 2u };
	std::vector<std::uint32_t>  m_primIndices    = { 0u };
	MeshBundleTemporaryDetails  m_bundleDetails{ .meshTemporaryDetailsVS = { MeshTemporaryDetailsVS{} } };

public:
	void GenerateTemporaryData(bool) override {}

	// Vertex and Mesh
	[[nodiscard]]
	const std::vector<Vertex>& GetVertices() const noexcept override
	{
		return m_vertices;
	}
	[[nodiscard]]
	const std::vector<std::uint32_t>& GetVertexIndices() const noexcept override
	{
		return m_vertexIndices;
	}
	[[nodiscard]]
	const MeshBundleTemporaryDetails& GetTemporaryBundleDetails() const noexcept override
	{
		return m_bundleDetails;
	}
	[[nodiscard]]
	MeshBundleTemporaryDetails&& GetTemporaryBundleDetails() noexcept override
	{
		return std::move(m_bundleDetails);
	}

	// Mesh only
	[[nodiscard]]
	const std::vector<std::uint32_t>& GetPrimIndices() const noexcept override
	{
		return m_primIndices;
	}
	[[nodiscard]]
	const std::vector<MeshletDetails>& GetMeshletDetails() const noexcept override
	{
		return m_meshletDetails;
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

	ModelManagerVSIndividual vsIndividual{};

	MeshManagerVSIndividual vsIndividualMesh{
		logicalDevice, &memoryManager, queueManager.GetAllIndices()
	};

	TemporaryDataBufferGPU tempDataBuffer{};

	ModelBuffers modelBuffers{ logicalDevice, &memoryManager, Constants::frameCount, {} };

	{
		auto meshVS = std::make_unique<MeshBundleTemporaryDummy>();
		std::uint32_t index = vsIndividualMesh.AddMeshBundle(
			std::move(meshVS), stagingBufferManager, tempDataBuffer
		);
		EXPECT_EQ(index, 0u) << "Index isn't 0u";
	}
	{
		auto meshVS = std::make_unique<MeshBundleTemporaryDummy>();
		std::uint32_t index = vsIndividualMesh.AddMeshBundle(
			std::move(meshVS), stagingBufferManager, tempDataBuffer
		);
		EXPECT_EQ(index, 1u) << "Index isn't 1u";
	}
	vsIndividualMesh.RemoveMeshBundle(0u);
	{
		auto meshVS = std::make_unique<MeshBundleTemporaryDummy>();
		std::uint32_t index = vsIndividualMesh.AddMeshBundle(
			std::move(meshVS), stagingBufferManager, tempDataBuffer
		);
		EXPECT_EQ(index, 0u) << "Index isn't 0u";
	}

	{
		auto model       = std::make_shared<ModelDummy>();
		auto modelBundle = std::make_shared<ModelBundleDummy>();

		modelBundle->AddModel(std::move(model));

		std::vector<std::shared_ptr<Model>> models = modelBundle->GetModels();
		std::vector<std::uint32_t> modelIndices    = modelBuffers.AddMultipleRU32(std::move(models));

		std::uint32_t index = vsIndividual.AddModelBundle(std::move(modelBundle), std::move(modelIndices));
		EXPECT_EQ(index, 0u) << "Index isn't 0.";
	}
	{
		auto model       = std::make_shared<ModelDummy>();
		auto modelBundle = std::make_shared<ModelBundleDummy>();

		modelBundle->AddModel(std::move(model));

		std::vector<std::shared_ptr<Model>> models = modelBundle->GetModels();
		std::vector<std::uint32_t> modelIndices    = modelBuffers.AddMultipleRU32(std::move(models));

		std::uint32_t index = vsIndividual.AddModelBundle(std::move(modelBundle), std::move(modelIndices));
		EXPECT_EQ(index, 1u) << "Index isn't 1.";
	}
	{
		auto modelBundle = std::make_shared<ModelBundleDummy>();

		for (size_t index = 0u; index < 5u; ++index)
			modelBundle->AddModel(std::make_shared<ModelDummy>());

		std::vector<std::shared_ptr<Model>> models = modelBundle->GetModels();
		std::vector<std::uint32_t> modelIndices    = modelBuffers.AddMultipleRU32(std::move(models));

		std::uint32_t index = vsIndividual.AddModelBundle(std::move(modelBundle), std::move(modelIndices));
		EXPECT_EQ(index, 2u) << "Index isn't 2.";
	}
	modelBuffers.Remove(vsIndividual.RemoveModelBundle(1u));
	{
		auto modelBundle = std::make_shared<ModelBundleDummy>();

		for (size_t index = 0u; index < 7u; ++index)
			modelBundle->AddModel(std::make_shared<ModelDummy>());

		std::vector<std::shared_ptr<Model>> models = modelBundle->GetModels();
		std::vector<std::uint32_t> modelIndices    = modelBuffers.AddMultipleRU32(std::move(models));

		std::uint32_t index = vsIndividual.AddModelBundle(std::move(modelBundle), std::move(modelIndices));
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
		logicalDevice, &memoryManager, queueManager.GetAllIndices(), Constants::frameCount
	};

	MeshManagerVSIndirect vsIndirectMesh{ logicalDevice, &memoryManager, queueManager.GetAllIndices() };

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

	vsIndirect.SetDescriptorBufferLayoutVS(descBuffersVS, Constants::vsSetLayoutIndex);
	vsIndirect.SetDescriptorBufferLayoutCS(descBuffersCS, Constants::csSetLayoutIndex);

	for (auto& descBuffer : descBuffersVS)
		descBuffer.CreateBuffer();
	for (auto& descBuffer : descBuffersCS)
		descBuffer.CreateBuffer();

	PipelineLayout graphicsPipelineLayout{ logicalDevice };

	ModelManagerVSIndirect::SetGraphicsConstantRange(graphicsPipelineLayout);

	graphicsPipelineLayout.Create(descBuffersVS.front().GetValidLayouts());

	PipelineLayout computePipelineLayout{ logicalDevice };

	ModelManagerVSIndirect::SetComputeConstantRange(computePipelineLayout);

	computePipelineLayout.Create(descBuffersCS.front().GetValidLayouts());

	TemporaryDataBufferGPU tempDataBuffer{};

	ModelBuffers modelBuffers{
		logicalDevice, &memoryManager, Constants::frameCount,
		queueManager.GetComputeAndGraphicsIndices().ResolveQueueIndices()
	};

	{
		auto meshVS = std::make_unique<MeshBundleTemporaryDummy>();
		std::uint32_t index = vsIndirectMesh.AddMeshBundle(
			std::move(meshVS), stagingBufferManager, tempDataBuffer
		);
		EXPECT_EQ(index, 0u) << "Index isn't 0u";
	}
	{
		auto meshVS = std::make_unique<MeshBundleTemporaryDummy>();
		std::uint32_t index = vsIndirectMesh.AddMeshBundle(
			std::move(meshVS), stagingBufferManager, tempDataBuffer
		);
		EXPECT_EQ(index, 1u) << "Index isn't 1u";
	}
	vsIndirectMesh.RemoveMeshBundle(0u);
	{
		auto meshVS = std::make_unique<MeshBundleTemporaryDummy>();
		std::uint32_t index = vsIndirectMesh.AddMeshBundle(
			std::move(meshVS), stagingBufferManager, tempDataBuffer
		);
		EXPECT_EQ(index, 0u) << "Index isn't 0u";
	}

	{
		auto model       = std::make_shared<ModelDummy>();
		auto modelBundle = std::make_shared<ModelBundleDummy>();

		modelBundle->AddModel(std::move(model));

		std::vector<std::shared_ptr<Model>> models = modelBundle->GetModels();
		std::vector<std::uint32_t> modelIndices    = modelBuffers.AddMultipleRU32(std::move(models));

		std::uint32_t index = vsIndirect.AddModelBundle(std::move(modelBundle), std::move(modelIndices));
		EXPECT_EQ(index, 0u) << "Index isn't 0.";
	}
	{
		auto model       = std::make_shared<ModelDummy>();
		auto modelBundle = std::make_shared<ModelBundleDummy>();

		modelBundle->AddModel(std::move(model));

		std::vector<std::shared_ptr<Model>> models = modelBundle->GetModels();
		std::vector<std::uint32_t> modelIndices    = modelBuffers.AddMultipleRU32(std::move(models));

		std::uint32_t index = vsIndirect.AddModelBundle(std::move(modelBundle), std::move(modelIndices));
		EXPECT_EQ(index, 1u) << "Index isn't 1.";
	}
	{
		auto modelBundle = std::make_shared<ModelBundleDummy>();

		for (size_t index = 0u; index < 5u; ++index)
			modelBundle->AddModel(std::make_shared<ModelDummy>());

		std::vector<std::shared_ptr<Model>> models = modelBundle->GetModels();
		std::vector<std::uint32_t> modelIndices    = modelBuffers.AddMultipleRU32(std::move(models));

		std::uint32_t index = vsIndirect.AddModelBundle(std::move(modelBundle), std::move(modelIndices));
		EXPECT_EQ(index, 2u) << "Index isn't 2.";
	}
	modelBuffers.Remove(vsIndirect.RemoveModelBundle(1u));
	{
		auto modelBundle = std::make_shared<ModelBundleDummy>();

		for (size_t index = 0u; index < 7u; ++index)
			modelBundle->AddModel(std::make_shared<ModelDummy>());

		std::vector<std::shared_ptr<Model>> models = modelBundle->GetModels();
		std::vector<std::uint32_t> modelIndices    = modelBuffers.AddMultipleRU32(std::move(models));

		std::uint32_t index = vsIndirect.AddModelBundle(std::move(modelBundle), std::move(modelIndices));
		EXPECT_EQ(index, 1u) << "Index isn't 1.";
	}
	{
		auto model       = std::make_shared<ModelDummy>();
		auto modelBundle = std::make_shared<ModelBundleDummy>();

		modelBundle->AddModel(std::move(model));

		std::vector<std::shared_ptr<Model>> models = modelBundle->GetModels();
		std::vector<std::uint32_t> modelIndices    = modelBuffers.AddMultipleRU32(std::move(models));

		std::uint32_t index = vsIndirect.AddModelBundle(std::move(modelBundle), std::move(modelIndices));
		EXPECT_EQ(index, 3u) << "Index isn't 3.";
	}
	{
		auto model       = std::make_shared<ModelDummy>();
		auto modelBundle = std::make_shared<ModelBundleDummy>();

		modelBundle->AddModel(std::move(model));

		std::vector<std::shared_ptr<Model>> models = modelBundle->GetModels();
		std::vector<std::uint32_t> modelIndices    = modelBuffers.AddMultipleRU32(std::move(models));

		std::uint32_t index = vsIndirect.AddModelBundle(std::move(modelBundle), std::move(modelIndices));
		EXPECT_EQ(index, 4u) << "Index isn't 4.";
	}
	{
		auto modelBundle = std::make_shared<ModelBundleDummy>();

		for (size_t index = 0u; index < 7u; ++index)
		{
			auto model = std::make_shared<ModelDummy>();

			model->SetPipelineIndex(2u);

			modelBundle->AddModel(std::move(model));
		}
		for (size_t index = 0u; index < 5u; ++index)
		{
			auto model = std::make_shared<ModelDummy>();

			model->SetPipelineIndex(1u);

			modelBundle->AddModel(std::move(model));
		}
		for (size_t index = 0u; index < 3u; ++index)
		{
			auto model = std::make_shared<ModelDummy>();

			model->SetPipelineIndex(3u);

			modelBundle->AddModel(std::move(model));
		}

		std::vector<std::shared_ptr<Model>> models = modelBundle->GetModels();
		std::vector<std::uint32_t> modelIndices    = modelBuffers.AddMultipleRU32(std::move(models));

		std::uint32_t index = vsIndirect.AddModelBundle(std::move(modelBundle), std::move(modelIndices));
		EXPECT_EQ(index, 5u) << "Index isn't 5.";

		vsIndirect.ChangeModelPipeline(index, 7u, 1u, 3u);
		vsIndirect.ChangeModelPipeline(index, 4u, 2u, 1u);
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

	ModelManagerMS managerMS{};

	MeshManagerMS managerMSMesh{ logicalDevice, &memoryManager, queueManager.GetAllIndices() };

	std::vector<VkDescriptorBuffer> descBuffers{};

	for (size_t _ = 0u; _ < Constants::frameCount; ++_)
		descBuffers.emplace_back(
			VkDescriptorBuffer{ logicalDevice, &memoryManager, Constants::descSetLayoutCount }
		);

	managerMSMesh.SetDescriptorBufferLayout(descBuffers, Constants::vsSetLayoutIndex);

	for (auto& descBuffer : descBuffers)
		descBuffer.CreateBuffer();

	PipelineLayout graphicsPipelineLayout{ logicalDevice };

	ModelManagerMS::SetGraphicsConstantRange(graphicsPipelineLayout);

	graphicsPipelineLayout.Create(descBuffers.front().GetValidLayouts());

	TemporaryDataBufferGPU tempDataBuffer{};

	ModelBuffers modelBuffers{ logicalDevice, &memoryManager, Constants::frameCount, {} };

	{
		auto meshMS = std::make_unique<MeshBundleTemporaryDummy>();
		std::uint32_t index = managerMSMesh.AddMeshBundle(
			std::move(meshMS), stagingBufferManager, tempDataBuffer
		);
		EXPECT_EQ(index, 0u) << "Index isn't 0u";
	}
	{
		auto meshMS = std::make_unique<MeshBundleTemporaryDummy>();
		std::uint32_t index = managerMSMesh.AddMeshBundle(
			std::move(meshMS), stagingBufferManager, tempDataBuffer
		);
		EXPECT_EQ(index, 1u) << "Index isn't 1u";
	}
	managerMSMesh.RemoveMeshBundle(0u);
	{
		auto meshMS = std::make_unique<MeshBundleTemporaryDummy>();
		std::uint32_t index = managerMSMesh.AddMeshBundle(
			std::move(meshMS), stagingBufferManager, tempDataBuffer
		);
		EXPECT_EQ(index, 0u) << "Index isn't 0u";
	}
	{
		auto meshMS = std::make_unique<MeshBundleTemporaryDummy>();
		std::uint32_t index = managerMSMesh.AddMeshBundle(
			std::move(meshMS), stagingBufferManager, tempDataBuffer
		);
		EXPECT_EQ(index, 2u) << "Index isn't 2u";
	}

	{
		auto model       = std::make_shared<ModelDummy>();
		auto modelBundle = std::make_shared<ModelBundleDummy>();

		modelBundle->AddModel(std::move(model));

		std::vector<std::shared_ptr<Model>> models = modelBundle->GetModels();
		std::vector<std::uint32_t> modelIndices    = modelBuffers.AddMultipleRU32(std::move(models));

		std::uint32_t index = managerMS.AddModelBundle(std::move(modelBundle), std::move(modelIndices));
		EXPECT_EQ(index, 0u) << "Index isn't 0.";
	}
	{
		auto model       = std::make_shared<ModelDummy>();
		auto modelBundle = std::make_shared<ModelBundleDummy>();

		modelBundle->AddModel(std::move(model));

		std::vector<std::shared_ptr<Model>> models = modelBundle->GetModels();
		std::vector<std::uint32_t> modelIndices    = modelBuffers.AddMultipleRU32(std::move(models));

		std::uint32_t index = managerMS.AddModelBundle(std::move(modelBundle), std::move(modelIndices));
		EXPECT_EQ(index, 1u) << "Index isn't 1.";
	}
	{
		auto modelBundle = std::make_shared<ModelBundleDummy>();

		for (size_t index = 0u; index < 5u; ++index)
			modelBundle->AddModel(std::make_shared<ModelDummy>());

		std::vector<std::shared_ptr<Model>> models = modelBundle->GetModels();
		std::vector<std::uint32_t> modelIndices    = modelBuffers.AddMultipleRU32(std::move(models));

		std::uint32_t index = managerMS.AddModelBundle(std::move(modelBundle), std::move(modelIndices));
		EXPECT_EQ(index, 2u) << "Index isn't 2.";
	}
	modelBuffers.Remove(managerMS.RemoveModelBundle(1u));
	{
		auto modelBundle = std::make_shared<ModelBundleDummy>();

		for (size_t index = 0u; index < 7u; ++index)
			modelBundle->AddModel(std::make_shared<ModelDummy>());

		std::vector<std::shared_ptr<Model>> models = modelBundle->GetModels();
		std::vector<std::uint32_t> modelIndices    = modelBuffers.AddMultipleRU32(std::move(models));

		std::uint32_t index = managerMS.AddModelBundle(std::move(modelBundle), std::move(modelIndices));
		EXPECT_EQ(index, 1u) << "Index isn't 1.";
	}
	{
		auto modelBundle = std::make_shared<ModelBundleDummy>();

		for (size_t index = 0u; index < 7u; ++index)
		{
			auto model = std::make_shared<ModelDummy>();

			model->SetPipelineIndex(2u);

			modelBundle->AddModel(std::move(model));
		}
		for (size_t index = 0u; index < 5u; ++index)
		{
			auto model = std::make_shared<ModelDummy>();

			model->SetPipelineIndex(1u);

			modelBundle->AddModel(std::move(model));
		}
		for (size_t index = 0u; index < 3u; ++index)
		{
			auto model = std::make_shared<ModelDummy>();

			model->SetPipelineIndex(3u);

			modelBundle->AddModel(std::move(model));
		}

		std::vector<std::shared_ptr<Model>> models = modelBundle->GetModels();
		std::vector<std::uint32_t> modelIndices    = modelBuffers.AddMultipleRU32(std::move(models));

		std::uint32_t index = managerMS.AddModelBundle(std::move(modelBundle), std::move(modelIndices));
		EXPECT_EQ(index, 3u) << "Index isn't 3.";

		managerMS.ChangeModelPipeline(index, 7u, 1u, 3u);
	}
}

