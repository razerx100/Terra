#include <gtest/gtest.h>
#include <memory>
#include <limits>
#include <ranges>
#include <algorithm>

#include <VKInstanceManager.hpp>
#include <VkDeviceManager.hpp>
#include <VkModelManager.hpp>
#include <VkModelBuffer.hpp>
#include <VkStagingBufferManager.hpp>
#include <VkDescriptorBuffer.hpp>
#include <VKRenderPass.hpp>

using namespace Terra;

namespace Constants
{
	constexpr const char* appName              = "TerraTest";
	constexpr std::uint32_t frameCount         = 2u;
	constexpr std::uint32_t descSetLayoutCount = 2u;
	constexpr std::uint32_t vsSetLayoutIndex   = 0u;
	constexpr std::uint32_t csSetLayoutIndex   = 0u;
	constexpr std::uint32_t fsSetLayoutIndex   = 1u;
	constexpr std::uint32_t meshBundleIndex    = 0u;
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

class PipelineModelBundleDummy : public PipelineModelBundle
{
	std::uint32_t              m_psoIndex = 0u;
	std::vector<std::uint32_t> m_modelIndices;
public:
	void SetPipelineIndex(std::uint32_t index) noexcept { m_psoIndex = index; }

	void AddModelIndex(std::uint32_t index) noexcept
	{
		m_modelIndices.emplace_back(index);
	}

	void RemoveModelIndex(std::uint32_t indexInBundle) noexcept
	{
		std::erase(m_modelIndices, indexInBundle);
	}

	[[nodiscard]]
	std::uint32_t GetPipelineIndex() const noexcept override { return m_psoIndex; }
	[[nodiscard]]
	const std::vector<std::uint32_t>& GetModelIndicesInBundle() const noexcept override
	{
		return m_modelIndices;
	}
};

class ModelBundleDummy : public ModelBundle
{
	using DummyPipelineContainer_t = std::vector<std::shared_ptr<PipelineModelBundleDummy>>;

	std::uint32_t            m_meshBundleIndex = Constants::meshBundleIndex;
	ModelContainer_t         m_models;
	PipelineContainer_t      m_pipelines;
	DummyPipelineContainer_t m_dummyPipelines;

public:
	void AddModel(std::uint32_t pipelineIndex, std::shared_ptr<Model> model) noexcept
	{
		const auto modelIndex = static_cast<std::uint32_t>(std::size(m_models));

		m_models.emplace_back(std::move(model));

		m_dummyPipelines[pipelineIndex]->AddModelIndex(modelIndex);
	}

	std::uint32_t AddPipeline(std::shared_ptr<PipelineModelBundleDummy> pipeline) noexcept
	{
		const auto pipelineIndex = static_cast<std::uint32_t>(std::size(m_pipelines));

		m_dummyPipelines.emplace_back(pipeline);
		m_pipelines.emplace_back(std::move(pipeline));

		return pipelineIndex;
	}

	void ChangeModelPipeline(
		std::uint32_t modelIndexInBundle, std::uint32_t oldPipelineIndex,
		std::uint32_t newPipelineIndex
	) noexcept {
		size_t oldPipelineIndexInBundle = std::numeric_limits<size_t>::max();
		size_t newPipelineIndexInBundle = std::numeric_limits<size_t>::max();

		const size_t pipelineCount = std::size(m_dummyPipelines);

		for (size_t index = 0u; index < pipelineCount; ++index)
		{
			const size_t currentPipelineIndex = m_dummyPipelines[index]->GetPipelineIndex();

			if (currentPipelineIndex == oldPipelineIndex)
				oldPipelineIndexInBundle = index;

			if (currentPipelineIndex == newPipelineIndex)
				newPipelineIndexInBundle = index;

			const bool bothFound = oldPipelineIndexInBundle != std::numeric_limits<size_t>::max()
				&& newPipelineIndexInBundle != std::numeric_limits<size_t>::max();

			if (bothFound)
				break;
		}

		m_dummyPipelines[oldPipelineIndexInBundle]->RemoveModelIndex(modelIndexInBundle);
		m_dummyPipelines[newPipelineIndexInBundle]->AddModelIndex(modelIndexInBundle);
	}

	[[nodiscard]]
	std::uint32_t GetMeshBundleIndex() const noexcept override { return m_meshBundleIndex; }
	[[nodiscard]]
	const ModelContainer_t& GetModels() const noexcept override
	{
		return m_models;
	}
	[[nodiscard]]
	ModelContainer_t& GetModels() noexcept override
	{
		return m_models;
	}
	[[nodiscard]]
	const PipelineContainer_t& GetPipelineBundles() const noexcept override
	{
		return m_pipelines;
	}
};

class MeshBundleTemporaryDummy : public MeshBundleTemporary
{
	std::vector<MeshletDetails> m_meshletDetails = { MeshletDetails{}, MeshletDetails{} };
	std::vector<Vertex>         m_vertices       = { Vertex{} };
	std::vector<std::uint32_t>  m_vertexIndices  = { 0u, 1u, 2u };
	std::vector<std::uint32_t>  m_primIndices    = { 0u };
	MeshBundleTemporaryDetails  m_bundleDetails
	{
		.meshTemporaryDetailsVS = { MeshTemporaryDetailsVS{} }
	};

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
			models.emplace_back(std::make_shared<Model>());

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
			models.emplace_back(std::make_shared<Model>());

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
				models.emplace_back(std::make_shared<Model>());

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
				models.emplace_back(std::make_shared<Model>());

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
				models.emplace_back(std::make_shared<Model>());

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
				models.emplace_back(std::make_shared<Model>());

			std::vector<size_t> modelIndices = modelBuffers.AddMultiple(std::move(models));

			for (size_t index = 5u; index < std::size(modelIndices); ++index)
			{
				const size_t modelIndex = modelIndices.at(index);

				EXPECT_EQ(modelIndex, index) << "Model index doesn't match.";
			}
		}
	}
}

static void RemoveModelBundle(
	ModelBuffers& modelBuffer, const ModelBundle& modelBundle
) noexcept {
	const auto& models = modelBundle.GetModels();

	for (const auto& model : models)
		modelBuffer.Remove(model->GetModelIndexInBuffer());
}

TEST_F(ModelManagerTest, ModelManagerVSIndividualTest)
{
	VkDevice logicalDevice          = s_deviceManager->GetLogicalDevice();
	VkPhysicalDevice physicalDevice = s_deviceManager->GetPhysicalDevice();

	MemoryManager memoryManager{ physicalDevice, logicalDevice, 20_MB, 200_KB };

	const auto& queueManager = s_deviceManager->GetQueueFamilyManager();

	ThreadPool threadPool{ 2u };

	StagingBufferManager stagingBufferManager{
		logicalDevice, &memoryManager, &threadPool, &queueManager
	};

	VKRenderPass renderPass{ logicalDevice };
	renderPass.Create(RenderPassBuilder{});

	ModelManagerVSIndividual vsIndividual{};

	MeshManagerVSIndividual vsIndividualMesh{
		logicalDevice, &memoryManager, queueManager.GetAllIndices()
	};

	Callisto::TemporaryDataBufferGPU tempDataBuffer{};

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
		auto model       = std::make_shared<Model>();
		auto pipeline    = std::make_shared<PipelineModelBundleDummy>();
		auto modelBundle = std::make_shared<ModelBundleDummy>();

		modelBundle->AddPipeline(std::move(pipeline));
		modelBundle->AddModel(0u, std::move(model));

		std::vector<std::shared_ptr<Model>> models = modelBundle->GetModels();
		std::vector<std::uint32_t> modelIndices    = modelBuffers.AddMultipleRU32(
			std::move(models)
		);

		std::uint32_t index = vsIndividual.AddModelBundle(std::move(modelBundle), modelIndices);
		EXPECT_EQ(index, 0u) << "Index isn't 0.";
	}
	{
		auto model       = std::make_shared<Model>();
		auto pipeline    = std::make_shared<PipelineModelBundleDummy>();
		auto modelBundle = std::make_shared<ModelBundleDummy>();

		modelBundle->AddPipeline(std::move(pipeline));
		modelBundle->AddModel(0u, std::move(model));

		std::vector<std::shared_ptr<Model>> models = modelBundle->GetModels();
		std::vector<std::uint32_t> modelIndices    = modelBuffers.AddMultipleRU32(
			std::move(models)
		);

		std::uint32_t index = vsIndividual.AddModelBundle(std::move(modelBundle), modelIndices);
		EXPECT_EQ(index, 1u) << "Index isn't 1.";
	}
	{
		auto modelBundle = std::make_shared<ModelBundleDummy>();
		auto pipeline    = std::make_shared<PipelineModelBundleDummy>();
		auto pipeline1   = std::make_shared<PipelineModelBundleDummy>();

		modelBundle->AddPipeline(std::move(pipeline));
		modelBundle->AddPipeline(std::move(pipeline1));

		for (size_t index = 0u; index < 5u; ++index)
			modelBundle->AddModel(1u, std::make_shared<Model>());
		for (size_t index = 0u; index < 5u; ++index)
			modelBundle->AddModel(0u, std::make_shared<Model>());

		std::vector<std::shared_ptr<Model>> models = modelBundle->GetModels();
		std::vector<std::uint32_t> modelIndices    = modelBuffers.AddMultipleRU32(
			std::move(models)
		);

		std::uint32_t index = vsIndividual.AddModelBundle(
			std::move(modelBundle), modelIndices
		);
		EXPECT_EQ(index, 2u) << "Index isn't 2.";
	}
	::RemoveModelBundle(modelBuffers, *vsIndividual.RemoveModelBundle(1u));
	{
		auto modelBundle = std::make_shared<ModelBundleDummy>();
		auto pipeline    = std::make_shared<PipelineModelBundleDummy>();
		auto pipeline1   = std::make_shared<PipelineModelBundleDummy>();

		modelBundle->AddPipeline(std::move(pipeline));
		modelBundle->AddPipeline(std::move(pipeline1));

		for (size_t index = 0u; index < 7u; ++index)
			modelBundle->AddModel(0u, std::make_shared<Model>());
		for (size_t index = 0u; index < 5u; ++index)
			modelBundle->AddModel(1u, std::make_shared<Model>());

		std::vector<std::shared_ptr<Model>> models = modelBundle->GetModels();
		std::vector<std::uint32_t> modelIndices    = modelBuffers.AddMultipleRU32(
			std::move(models)
		);

		std::uint32_t index = vsIndividual.AddModelBundle(std::move(modelBundle), modelIndices);
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

	StagingBufferManager stagingBufferManager{
		logicalDevice, &memoryManager,  &threadPool, &queueManager
	};

	VKRenderPass renderPass{ logicalDevice };
	renderPass.Create(RenderPassBuilder{});

	ModelManagerVSIndirect vsIndirect{
		logicalDevice, &memoryManager, queueManager.GetAllIndices(), Constants::frameCount
	};

	MeshManagerVSIndirect vsIndirectMesh{
		logicalDevice, &memoryManager, queueManager.GetAllIndices()
	};

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

	Callisto::TemporaryDataBufferGPU tempDataBuffer{};

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
		auto model       = std::make_shared<Model>();
		auto pipeline    = std::make_shared<PipelineModelBundleDummy>();
		auto modelBundle = std::make_shared<ModelBundleDummy>();

		modelBundle->AddPipeline(std::move(pipeline));
		modelBundle->AddModel(0u, std::move(model));

		std::vector<std::shared_ptr<Model>> models = modelBundle->GetModels();
		std::vector<std::uint32_t> modelIndices    = modelBuffers.AddMultipleRU32(
			std::move(models)
		);

		std::uint32_t index = vsIndirect.AddModelBundle(std::move(modelBundle), modelIndices);
		EXPECT_EQ(index, 0u) << "Index isn't 0.";
	}
	{
		auto model       = std::make_shared<Model>();
		auto pipeline    = std::make_shared<PipelineModelBundleDummy>();
		auto modelBundle = std::make_shared<ModelBundleDummy>();

		modelBundle->AddPipeline(std::move(pipeline));
		modelBundle->AddModel(0u, std::move(model));

		std::vector<std::shared_ptr<Model>> models = modelBundle->GetModels();
		std::vector<std::uint32_t> modelIndices    = modelBuffers.AddMultipleRU32(
			std::move(models)
		);

		std::uint32_t index = vsIndirect.AddModelBundle(std::move(modelBundle), modelIndices);
		EXPECT_EQ(index, 1u) << "Index isn't 1.";
	}
	{
		auto modelBundle = std::make_shared<ModelBundleDummy>();
		auto pipeline    = std::make_shared<PipelineModelBundleDummy>();
		auto pipeline1   = std::make_shared<PipelineModelBundleDummy>();

		modelBundle->AddPipeline(std::move(pipeline));
		modelBundle->AddPipeline(std::move(pipeline1));

		for (size_t index = 0u; index < 5u; ++index)
			modelBundle->AddModel(1u, std::make_shared<Model>());
		for (size_t index = 0u; index < 5u; ++index)
			modelBundle->AddModel(0u, std::make_shared<Model>());

		std::vector<std::shared_ptr<Model>> models = modelBundle->GetModels();
		std::vector<std::uint32_t> modelIndices    = modelBuffers.AddMultipleRU32(
			std::move(models)
		);

		std::uint32_t index = vsIndirect.AddModelBundle(std::move(modelBundle), modelIndices);
		EXPECT_EQ(index, 2u) << "Index isn't 2.";
	}
	::RemoveModelBundle(modelBuffers, *vsIndirect.RemoveModelBundle(1u));
	{
		auto modelBundle = std::make_shared<ModelBundleDummy>();
		auto pipeline    = std::make_shared<PipelineModelBundleDummy>();
		auto pipeline1   = std::make_shared<PipelineModelBundleDummy>();

		modelBundle->AddPipeline(std::move(pipeline));
		modelBundle->AddPipeline(std::move(pipeline1));

		for (size_t index = 0u; index < 7u; ++index)
			modelBundle->AddModel(0u, std::make_shared<Model>());
		for (size_t index = 0u; index < 5u; ++index)
			modelBundle->AddModel(1u, std::make_shared<Model>());

		std::vector<std::shared_ptr<Model>> models = modelBundle->GetModels();
		std::vector<std::uint32_t> modelIndices    = modelBuffers.AddMultipleRU32(
			std::move(models)
		);

		std::uint32_t index = vsIndirect.AddModelBundle(std::move(modelBundle), modelIndices);
		EXPECT_EQ(index, 1u) << "Index isn't 1.";
	}
	{
		auto model       = std::make_shared<Model>();
		auto pipeline    = std::make_shared<PipelineModelBundleDummy>();
		auto modelBundle = std::make_shared<ModelBundleDummy>();

		modelBundle->AddPipeline(std::move(pipeline));
		modelBundle->AddModel(0u, std::move(model));

		std::vector<std::shared_ptr<Model>> models = modelBundle->GetModels();
		std::vector<std::uint32_t> modelIndices    = modelBuffers.AddMultipleRU32(
			std::move(models)
		);

		std::uint32_t index = vsIndirect.AddModelBundle(std::move(modelBundle), modelIndices);
		EXPECT_EQ(index, 3u) << "Index isn't 3.";
	}
	{
		auto model       = std::make_shared<Model>();
		auto pipeline    = std::make_shared<PipelineModelBundleDummy>();
		auto modelBundle = std::make_shared<ModelBundleDummy>();

		modelBundle->AddPipeline(std::move(pipeline));
		modelBundle->AddModel(0u, std::move(model));

		std::vector<std::shared_ptr<Model>> models = modelBundle->GetModels();
		std::vector<std::uint32_t> modelIndices    = modelBuffers.AddMultipleRU32(
			std::move(models)
		);

		std::uint32_t index = vsIndirect.AddModelBundle(std::move(modelBundle), modelIndices);
		EXPECT_EQ(index, 4u) << "Index isn't 4.";
	}
	{
		auto modelBundle = std::make_shared<ModelBundleDummy>();
		auto pipeline    = std::make_shared<PipelineModelBundleDummy>();
		auto pipeline1   = std::make_shared<PipelineModelBundleDummy>();
		auto pipeline2   = std::make_shared<PipelineModelBundleDummy>();

		pipeline->SetPipelineIndex(1u);
		pipeline1->SetPipelineIndex(2u);
		pipeline2->SetPipelineIndex(3u);

		modelBundle->AddPipeline(std::move(pipeline));
		modelBundle->AddPipeline(std::move(pipeline1));
		modelBundle->AddPipeline(std::move(pipeline2));

		for (size_t index = 0u; index < 7u; ++index)
			modelBundle->AddModel(1u, std::move(std::make_shared<Model>()));

		for (size_t index = 0u; index < 5u; ++index)
			modelBundle->AddModel(0u, std::move(std::make_shared<Model>()));

		for (size_t index = 0u; index < 3u; ++index)
			modelBundle->AddModel(2u, std::move(std::make_shared<Model>()));

		std::vector<std::shared_ptr<Model>> models = modelBundle->GetModels();
		std::vector<std::uint32_t> modelIndices    = modelBuffers.AddMultipleRU32(
			std::move(models)
		);

		std::uint32_t index = vsIndirect.AddModelBundle(modelBundle, modelIndices);
		EXPECT_EQ(index, 5u) << "Index isn't 5.";

		modelBundle->ChangeModelPipeline(7u, 1u, 3u);
		vsIndirect.ReconfigureModels(index, 1u, 3u);

		modelBundle->ChangeModelPipeline(5u, 2u, 1u);
		modelBundle->ChangeModelPipeline(4u, 2u, 1u);
		vsIndirect.ReconfigureModels(index, 2u, 1u);
	}
}

TEST_F(ModelManagerTest, ModelManagerMS)
{
	VkDevice logicalDevice          = s_deviceManager->GetLogicalDevice();
	VkPhysicalDevice physicalDevice = s_deviceManager->GetPhysicalDevice();

	MemoryManager memoryManager{ physicalDevice, logicalDevice, 20_MB, 200_KB };

	const auto& queueManager = s_deviceManager->GetQueueFamilyManager();

	ThreadPool threadPool{ 2u };

	StagingBufferManager stagingBufferManager{
		logicalDevice, &memoryManager, &threadPool, &queueManager
	};

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

	Callisto::TemporaryDataBufferGPU tempDataBuffer{};

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
		auto model       = std::make_shared<Model>();
		auto pipeline    = std::make_shared<PipelineModelBundleDummy>();
		auto modelBundle = std::make_shared<ModelBundleDummy>();

		modelBundle->AddPipeline(std::move(pipeline));
		modelBundle->AddModel(0u, std::move(model));

		std::vector<std::shared_ptr<Model>> models = modelBundle->GetModels();
		std::vector<std::uint32_t> modelIndices    = modelBuffers.AddMultipleRU32(
			std::move(models)
		);

		std::uint32_t index = managerMS.AddModelBundle(std::move(modelBundle), modelIndices);
		EXPECT_EQ(index, 0u) << "Index isn't 0.";
	}
	{
		auto model       = std::make_shared<Model>();
		auto pipeline    = std::make_shared<PipelineModelBundleDummy>();
		auto modelBundle = std::make_shared<ModelBundleDummy>();

		modelBundle->AddPipeline(std::move(pipeline));
		modelBundle->AddModel(0u, std::move(model));

		std::vector<std::shared_ptr<Model>> models = modelBundle->GetModels();
		std::vector<std::uint32_t> modelIndices    = modelBuffers.AddMultipleRU32(
			std::move(models)
		);

		std::uint32_t index = managerMS.AddModelBundle(std::move(modelBundle), modelIndices);
		EXPECT_EQ(index, 1u) << "Index isn't 1.";
	}
	{
		auto modelBundle = std::make_shared<ModelBundleDummy>();
		auto pipeline    = std::make_shared<PipelineModelBundleDummy>();
		auto pipeline1   = std::make_shared<PipelineModelBundleDummy>();

		modelBundle->AddPipeline(std::move(pipeline));
		modelBundle->AddPipeline(std::move(pipeline1));

		for (size_t index = 0u; index < 5u; ++index)
			modelBundle->AddModel(1u, std::make_shared<Model>());
		for (size_t index = 0u; index < 5u; ++index)
			modelBundle->AddModel(0u, std::make_shared<Model>());

		std::vector<std::shared_ptr<Model>> models = modelBundle->GetModels();
		std::vector<std::uint32_t> modelIndices    = modelBuffers.AddMultipleRU32(
			std::move(models)
		);

		std::uint32_t index = managerMS.AddModelBundle(std::move(modelBundle), modelIndices);
		EXPECT_EQ(index, 2u) << "Index isn't 2.";
	}
	::RemoveModelBundle(modelBuffers, *managerMS.RemoveModelBundle(1u));
	{
		auto modelBundle = std::make_shared<ModelBundleDummy>();
		auto pipeline    = std::make_shared<PipelineModelBundleDummy>();
		auto pipeline1   = std::make_shared<PipelineModelBundleDummy>();

		modelBundle->AddPipeline(std::move(pipeline));
		modelBundle->AddPipeline(std::move(pipeline1));

		for (size_t index = 0u; index < 7u; ++index)
			modelBundle->AddModel(0u, std::make_shared<Model>());
		for (size_t index = 0u; index < 5u; ++index)
			modelBundle->AddModel(1u, std::make_shared<Model>());

		std::vector<std::shared_ptr<Model>> models = modelBundle->GetModels();
		std::vector<std::uint32_t> modelIndices    = modelBuffers.AddMultipleRU32(
			std::move(models)
		);

		std::uint32_t index = managerMS.AddModelBundle(std::move(modelBundle), modelIndices);
		EXPECT_EQ(index, 1u) << "Index isn't 1.";
	}
	{
		auto modelBundle = std::make_shared<ModelBundleDummy>();
		auto pipeline    = std::make_shared<PipelineModelBundleDummy>();
		auto pipeline1   = std::make_shared<PipelineModelBundleDummy>();
		auto pipeline2   = std::make_shared<PipelineModelBundleDummy>();

		pipeline->SetPipelineIndex(1u);
		pipeline1->SetPipelineIndex(2u);
		pipeline2->SetPipelineIndex(3u);

		modelBundle->AddPipeline(std::move(pipeline));
		modelBundle->AddPipeline(std::move(pipeline1));
		modelBundle->AddPipeline(std::move(pipeline2));

		for (size_t index = 0u; index < 7u; ++index)
			modelBundle->AddModel(1u, std::move(std::make_shared<Model>()));

		for (size_t index = 0u; index < 5u; ++index)
			modelBundle->AddModel(0u, std::move(std::make_shared<Model>()));

		for (size_t index = 0u; index < 3u; ++index)
			modelBundle->AddModel(2u, std::move(std::make_shared<Model>()));

		std::vector<std::shared_ptr<Model>> models = modelBundle->GetModels();
		std::vector<std::uint32_t> modelIndices    = modelBuffers.AddMultipleRU32(
			std::move(models)
		);

		std::uint32_t index = managerMS.AddModelBundle(modelBundle, modelIndices);
		EXPECT_EQ(index, 3u) << "Index isn't 3.";

		modelBundle->ChangeModelPipeline(7u, 1u, 3u);
		managerMS.ReconfigureModels(index, 1u, 3u);
	}
}

