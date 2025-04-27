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

	static const MeshBundleTemporaryData meshBundleData
	{
		.vertices       = { Vertex{} },
		.indices        = { 0u, 1u, 2u },
		.primIndices    = { 0u },
		.meshletDetails = { MeshletDetails{}, MeshletDetails{} },
		.bundleDetails = MeshBundleTemporaryDetails
		{
			.meshTemporaryDetailsVS = { MeshTemporaryDetailsVS{} },
			.meshTemporaryDetailsMS = { MeshTemporaryDetailsMS{} }
		}
	};
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

class ModelBundleDummy
{
	std::shared_ptr<ModelBundle> m_modelBundle;

	using ModelContainer_t    = ModelBundle::ModelContainer_t;
	using PipelineContainer_t = ModelBundle::PipelineContainer_t;

public:
	ModelBundleDummy() : m_modelBundle{ std::make_shared<ModelBundle>() }
	{
		m_modelBundle->SetMeshBundleIndex(Constants::meshBundleIndex);
	}

	void AddModel(std::uint32_t pipelineIndex, std::shared_ptr<Model> model) noexcept
	{
		ModelContainer_t& models       = m_modelBundle->GetModels();
		PipelineContainer_t& pipelines = m_modelBundle->GetPipelineBundles();

		const auto modelIndex = static_cast<std::uint32_t>(std::size(models));

		models.emplace_back(std::move(model));

		pipelines[pipelineIndex]->AddModelIndex(modelIndex);
	}

	std::uint32_t AddPipeline(std::shared_ptr<PipelineModelBundle> pipeline) noexcept
	{
		PipelineContainer_t& pipelines = m_modelBundle->GetPipelineBundles();

		const auto pipelineIndex = static_cast<std::uint32_t>(std::size(pipelines));

		pipelines.emplace_back(std::move(pipeline));

		return pipelineIndex;
	}

	void ChangeModelPipeline(
		std::uint32_t modelIndexInBundle, std::uint32_t oldPipelineIndex,
		std::uint32_t newPipelineIndex
	) noexcept {
		size_t oldPipelineIndexInBundle = std::numeric_limits<size_t>::max();
		size_t newPipelineIndexInBundle = std::numeric_limits<size_t>::max();

		PipelineContainer_t& pipelines = m_modelBundle->GetPipelineBundles();

		const size_t pipelineCount = std::size(pipelines);

		for (size_t index = 0u; index < pipelineCount; ++index)
		{
			const size_t currentPipelineIndex = pipelines[index]->GetPipelineIndex();

			if (currentPipelineIndex == oldPipelineIndex)
				oldPipelineIndexInBundle = index;

			if (currentPipelineIndex == newPipelineIndex)
				newPipelineIndexInBundle = index;

			const bool bothFound = oldPipelineIndexInBundle != std::numeric_limits<size_t>::max()
				&& newPipelineIndexInBundle != std::numeric_limits<size_t>::max();

			if (bothFound)
				break;
		}

		pipelines[oldPipelineIndexInBundle]->RemoveModelIndex(modelIndexInBundle);
		pipelines[newPipelineIndexInBundle]->AddModelIndex(modelIndexInBundle);
	}

	[[nodiscard]]
	std::uint32_t GetMeshBundleIndex() const noexcept
	{
		return m_modelBundle->GetMeshBundleIndex();
	}
	[[nodiscard]]
	auto&& GetModels(this auto&& self) noexcept
	{
		return std::forward_like<decltype(self)>(self.m_modelBundle->GetModels());
	}
	[[nodiscard]]
	auto&& GetPipelineBundles(this auto&& self) noexcept
	{
		return std::forward_like<decltype(self)>(self.m_modelBundle->GetPipelineBundles());
	}

	[[nodiscard]]
	std::shared_ptr<ModelBundle> GetModelBundle() const noexcept
	{
		return m_modelBundle;
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
		MeshBundleTemporaryData meshVS = Constants::meshBundleData;

		std::uint32_t index = vsIndividualMesh.AddMeshBundle(
			std::move(meshVS), stagingBufferManager, tempDataBuffer
		);
		EXPECT_EQ(index, 0u) << "Index isn't 0u";
	}
	{
		MeshBundleTemporaryData meshVS = Constants::meshBundleData;

		std::uint32_t index = vsIndividualMesh.AddMeshBundle(
			std::move(meshVS), stagingBufferManager, tempDataBuffer
		);
		EXPECT_EQ(index, 1u) << "Index isn't 1u";
	}
	vsIndividualMesh.RemoveMeshBundle(0u);
	{
		MeshBundleTemporaryData meshVS = Constants::meshBundleData;

		std::uint32_t index = vsIndividualMesh.AddMeshBundle(
			std::move(meshVS), stagingBufferManager, tempDataBuffer
		);
		EXPECT_EQ(index, 0u) << "Index isn't 0u";
	}

	{
		auto model       = std::make_shared<Model>();
		auto pipeline    = std::make_shared<PipelineModelBundle>();
		auto modelBundle = std::make_unique<ModelBundleDummy>();

		modelBundle->AddPipeline(std::move(pipeline));
		modelBundle->AddModel(0u, std::move(model));

		std::vector<std::shared_ptr<Model>> models = modelBundle->GetModels();
		std::vector<std::uint32_t> modelIndices    = modelBuffers.AddMultipleRU32(
			std::move(models)
		);

		std::uint32_t index = vsIndividual.AddModelBundle(
			modelBundle->GetModelBundle(), modelIndices
		);
		EXPECT_EQ(index, 0u) << "Index isn't 0.";
	}
	{
		auto model       = std::make_shared<Model>();
		auto pipeline    = std::make_shared<PipelineModelBundle>();
		auto modelBundle = std::make_unique<ModelBundleDummy>();

		modelBundle->AddPipeline(std::move(pipeline));
		modelBundle->AddModel(0u, std::move(model));

		std::vector<std::shared_ptr<Model>> models = modelBundle->GetModels();
		std::vector<std::uint32_t> modelIndices    = modelBuffers.AddMultipleRU32(
			std::move(models)
		);

		std::uint32_t index = vsIndividual.AddModelBundle(
			modelBundle->GetModelBundle(), modelIndices
		);
		EXPECT_EQ(index, 1u) << "Index isn't 1.";
	}
	{
		auto modelBundle = std::make_unique<ModelBundleDummy>();
		auto pipeline    = std::make_shared<PipelineModelBundle>();
		auto pipeline1   = std::make_shared<PipelineModelBundle>();

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
			modelBundle->GetModelBundle(), modelIndices
		);
		EXPECT_EQ(index, 2u) << "Index isn't 2.";
	}
	::RemoveModelBundle(modelBuffers, *vsIndividual.RemoveModelBundle(1u));
	{
		auto modelBundle = std::make_unique<ModelBundleDummy>();
		auto pipeline    = std::make_shared<PipelineModelBundle>();
		auto pipeline1   = std::make_shared<PipelineModelBundle>();

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

		std::uint32_t index = vsIndividual.AddModelBundle(
			modelBundle->GetModelBundle(), modelIndices
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
		MeshBundleTemporaryData meshVS = Constants::meshBundleData;

		std::uint32_t index = vsIndirectMesh.AddMeshBundle(
			std::move(meshVS), stagingBufferManager, tempDataBuffer
		);
		EXPECT_EQ(index, 0u) << "Index isn't 0u";
	}
	{
		MeshBundleTemporaryData meshVS = Constants::meshBundleData;

		std::uint32_t index = vsIndirectMesh.AddMeshBundle(
			std::move(meshVS), stagingBufferManager, tempDataBuffer
		);
		EXPECT_EQ(index, 1u) << "Index isn't 1u";
	}
	vsIndirectMesh.RemoveMeshBundle(0u);
	{
		MeshBundleTemporaryData meshVS = Constants::meshBundleData;

		std::uint32_t index = vsIndirectMesh.AddMeshBundle(
			std::move(meshVS), stagingBufferManager, tempDataBuffer
		);
		EXPECT_EQ(index, 0u) << "Index isn't 0u";
	}

	{
		auto model       = std::make_shared<Model>();
		auto pipeline    = std::make_shared<PipelineModelBundle>();
		auto modelBundle = std::make_unique<ModelBundleDummy>();

		modelBundle->AddPipeline(std::move(pipeline));
		modelBundle->AddModel(0u, std::move(model));

		std::vector<std::shared_ptr<Model>> models = modelBundle->GetModels();
		std::vector<std::uint32_t> modelIndices    = modelBuffers.AddMultipleRU32(
			std::move(models)
		);

		std::uint32_t index = vsIndirect.AddModelBundle(
			modelBundle->GetModelBundle(), modelIndices
		);
		EXPECT_EQ(index, 0u) << "Index isn't 0.";
	}
	{
		auto model       = std::make_shared<Model>();
		auto pipeline    = std::make_shared<PipelineModelBundle>();
		auto modelBundle = std::make_unique<ModelBundleDummy>();

		modelBundle->AddPipeline(std::move(pipeline));
		modelBundle->AddModel(0u, std::move(model));

		std::vector<std::shared_ptr<Model>> models = modelBundle->GetModels();
		std::vector<std::uint32_t> modelIndices    = modelBuffers.AddMultipleRU32(
			std::move(models)
		);

		std::uint32_t index = vsIndirect.AddModelBundle(
			modelBundle->GetModelBundle(), modelIndices
		);
		EXPECT_EQ(index, 1u) << "Index isn't 1.";
	}
	{
		auto modelBundle = std::make_unique<ModelBundleDummy>();
		auto pipeline    = std::make_shared<PipelineModelBundle>();
		auto pipeline1   = std::make_shared<PipelineModelBundle>();

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

		std::uint32_t index = vsIndirect.AddModelBundle(
			modelBundle->GetModelBundle(), modelIndices
		);
		EXPECT_EQ(index, 2u) << "Index isn't 2.";
	}
	::RemoveModelBundle(modelBuffers, *vsIndirect.RemoveModelBundle(1u));
	{
		auto modelBundle = std::make_unique<ModelBundleDummy>();
		auto pipeline    = std::make_shared<PipelineModelBundle>();
		auto pipeline1   = std::make_shared<PipelineModelBundle>();

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

		std::uint32_t index = vsIndirect.AddModelBundle(
			modelBundle->GetModelBundle(), modelIndices
		);
		EXPECT_EQ(index, 1u) << "Index isn't 1.";
	}
	{
		auto model       = std::make_shared<Model>();
		auto pipeline    = std::make_shared<PipelineModelBundle>();
		auto modelBundle = std::make_unique<ModelBundleDummy>();

		modelBundle->AddPipeline(std::move(pipeline));
		modelBundle->AddModel(0u, std::move(model));

		std::vector<std::shared_ptr<Model>> models = modelBundle->GetModels();
		std::vector<std::uint32_t> modelIndices    = modelBuffers.AddMultipleRU32(
			std::move(models)
		);

		std::uint32_t index = vsIndirect.AddModelBundle(
			modelBundle->GetModelBundle(), modelIndices
		);
		EXPECT_EQ(index, 3u) << "Index isn't 3.";
	}
	{
		auto model       = std::make_shared<Model>();
		auto pipeline    = std::make_shared<PipelineModelBundle>();
		auto modelBundle = std::make_unique<ModelBundleDummy>();

		modelBundle->AddPipeline(std::move(pipeline));
		modelBundle->AddModel(0u, std::move(model));

		std::vector<std::shared_ptr<Model>> models = modelBundle->GetModels();
		std::vector<std::uint32_t> modelIndices    = modelBuffers.AddMultipleRU32(
			std::move(models)
		);

		std::uint32_t index = vsIndirect.AddModelBundle(
			modelBundle->GetModelBundle(), modelIndices
		);
		EXPECT_EQ(index, 4u) << "Index isn't 4.";
	}
	{
		auto modelBundle = std::make_unique<ModelBundleDummy>();
		auto pipeline    = std::make_shared<PipelineModelBundle>();
		auto pipeline1   = std::make_shared<PipelineModelBundle>();
		auto pipeline2   = std::make_shared<PipelineModelBundle>();

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

		std::uint32_t index = vsIndirect.AddModelBundle(
			modelBundle->GetModelBundle(), modelIndices
		);
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
		MeshBundleTemporaryData meshMS = Constants::meshBundleData;

		std::uint32_t index = managerMSMesh.AddMeshBundle(
			std::move(meshMS), stagingBufferManager, tempDataBuffer
		);
		EXPECT_EQ(index, 0u) << "Index isn't 0u";
	}
	{
		MeshBundleTemporaryData meshMS = Constants::meshBundleData;

		std::uint32_t index = managerMSMesh.AddMeshBundle(
			std::move(meshMS), stagingBufferManager, tempDataBuffer
		);
		EXPECT_EQ(index, 1u) << "Index isn't 1u";
	}
	managerMSMesh.RemoveMeshBundle(0u);
	{
		MeshBundleTemporaryData meshMS = Constants::meshBundleData;

		std::uint32_t index = managerMSMesh.AddMeshBundle(
			std::move(meshMS), stagingBufferManager, tempDataBuffer
		);
		EXPECT_EQ(index, 0u) << "Index isn't 0u";
	}
	{
		MeshBundleTemporaryData meshMS = Constants::meshBundleData;

		std::uint32_t index = managerMSMesh.AddMeshBundle(
			std::move(meshMS), stagingBufferManager, tempDataBuffer
		);
		EXPECT_EQ(index, 2u) << "Index isn't 2u";
	}

	{
		auto model       = std::make_shared<Model>();
		auto pipeline    = std::make_shared<PipelineModelBundle>();
		auto modelBundle = std::make_unique<ModelBundleDummy>();

		modelBundle->AddPipeline(std::move(pipeline));
		modelBundle->AddModel(0u, std::move(model));

		std::vector<std::shared_ptr<Model>> models = modelBundle->GetModels();
		std::vector<std::uint32_t> modelIndices    = modelBuffers.AddMultipleRU32(
			std::move(models)
		);

		std::uint32_t index = managerMS.AddModelBundle(
			modelBundle->GetModelBundle(), modelIndices
		);
		EXPECT_EQ(index, 0u) << "Index isn't 0.";
	}
	{
		auto model       = std::make_shared<Model>();
		auto pipeline    = std::make_shared<PipelineModelBundle>();
		auto modelBundle = std::make_unique<ModelBundleDummy>();

		modelBundle->AddPipeline(std::move(pipeline));
		modelBundle->AddModel(0u, std::move(model));

		std::vector<std::shared_ptr<Model>> models = modelBundle->GetModels();
		std::vector<std::uint32_t> modelIndices    = modelBuffers.AddMultipleRU32(
			std::move(models)
		);

		std::uint32_t index = managerMS.AddModelBundle(
			modelBundle->GetModelBundle(), modelIndices
		);
		EXPECT_EQ(index, 1u) << "Index isn't 1.";
	}
	{
		auto modelBundle = std::make_unique<ModelBundleDummy>();
		auto pipeline    = std::make_shared<PipelineModelBundle>();
		auto pipeline1   = std::make_shared<PipelineModelBundle>();

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

		std::uint32_t index = managerMS.AddModelBundle(
			modelBundle->GetModelBundle(), modelIndices
		);
		EXPECT_EQ(index, 2u) << "Index isn't 2.";
	}
	::RemoveModelBundle(modelBuffers, *managerMS.RemoveModelBundle(1u));
	{
		auto modelBundle = std::make_unique<ModelBundleDummy>();
		auto pipeline    = std::make_shared<PipelineModelBundle>();
		auto pipeline1   = std::make_shared<PipelineModelBundle>();

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

		std::uint32_t index = managerMS.AddModelBundle(
			modelBundle->GetModelBundle(), modelIndices
		);
		EXPECT_EQ(index, 1u) << "Index isn't 1.";
	}
	{
		auto modelBundle = std::make_unique<ModelBundleDummy>();
		auto pipeline    = std::make_shared<PipelineModelBundle>();
		auto pipeline1   = std::make_shared<PipelineModelBundle>();
		auto pipeline2   = std::make_shared<PipelineModelBundle>();

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

		std::uint32_t index = managerMS.AddModelBundle(
			modelBundle->GetModelBundle(), modelIndices
		);
		EXPECT_EQ(index, 3u) << "Index isn't 3.";

		modelBundle->ChangeModelPipeline(7u, 1u, 3u);
		managerMS.ReconfigureModels(index, 1u, 3u);
	}
}

