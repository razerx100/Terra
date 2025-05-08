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

static void RemoveModelBundle(
	ModelContainer& modelContainer, const std::shared_ptr<ModelBundle>& modelBundle
) noexcept {
	modelContainer.RemoveModels(modelBundle->GetIndicesInContainer());
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

	auto modelContainer = std::make_shared<ModelContainer>();

	ModelBuffers modelBuffers{ logicalDevice, &memoryManager, Constants::frameCount, {} };

	modelBuffers.SetModelContainer(modelContainer);

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
		auto modelBundle = std::make_shared<ModelBundle>();

		modelBundle->SetModelContainer(modelContainer);

		modelBundle->AddModel(Model{}, 0u);

		std::uint32_t index = vsIndividual.AddModelBundle(std::move(modelBundle));

		modelBuffers.ExtendModelBuffers();

		EXPECT_EQ(index, 0u) << "Index isn't 0.";
	}
	{
		auto modelBundle = std::make_shared<ModelBundle>();

		modelBundle->SetModelContainer(modelContainer);

		modelBundle->AddModel(Model{}, 0u);

		std::uint32_t index = vsIndividual.AddModelBundle(std::move(modelBundle));

		modelBuffers.ExtendModelBuffers();

		EXPECT_EQ(index, 1u) << "Index isn't 1.";
	}
	{
		auto modelBundle = std::make_shared<ModelBundle>();

		modelBundle->SetModelContainer(modelContainer);

		for (size_t index = 0u; index < 5u; ++index)
			modelBundle->AddModel(Model{}, 1u);
		for (size_t index = 0u; index < 5u; ++index)
			modelBundle->AddModel(Model{}, 0u);

		std::uint32_t index = vsIndividual.AddModelBundle(std::move(modelBundle));

		modelBuffers.ExtendModelBuffers();

		EXPECT_EQ(index, 2u) << "Index isn't 2.";
	}

	::RemoveModelBundle(*modelContainer, vsIndividual.RemoveModelBundle(1u));

	{
		auto modelBundle = std::make_shared<ModelBundle>();

		modelBundle->SetModelContainer(modelContainer);

		for (size_t index = 0u; index < 7u; ++index)
			modelBundle->AddModel(Model{}, 0u);
		for (size_t index = 0u; index < 5u; ++index)
			modelBundle->AddModel(Model{}, 1u);

		std::uint32_t index = vsIndividual.AddModelBundle(std::move(modelBundle));

		modelBuffers.ExtendModelBuffers();

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

	auto modelContainer = std::make_shared<ModelContainer>();

	ModelBuffers modelBuffers{
		logicalDevice, &memoryManager, Constants::frameCount,
		queueManager.GetComputeAndGraphicsIndices().ResolveQueueIndices()
	};

	modelBuffers.SetModelContainer(modelContainer);

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
		auto modelBundle = std::make_shared<ModelBundle>();

		modelBundle->SetModelContainer(modelContainer);

		modelBundle->AddModel(Model{}, 0u);

		std::uint32_t index = vsIndirect.AddModelBundle(std::move(modelBundle));

		modelBuffers.ExtendModelBuffers();

		EXPECT_EQ(index, 0u) << "Index isn't 0.";
	}
	{
		auto modelBundle = std::make_shared<ModelBundle>();

		modelBundle->SetModelContainer(modelContainer);

		modelBundle->AddModel(Model{}, 0u);

		std::uint32_t index = vsIndirect.AddModelBundle(std::move(modelBundle));

		modelBuffers.ExtendModelBuffers();

		EXPECT_EQ(index, 1u) << "Index isn't 1.";
	}
	{
		auto modelBundle = std::make_shared<ModelBundle>();

		modelBundle->SetModelContainer(modelContainer);

		for (size_t index = 0u; index < 5u; ++index)
			modelBundle->AddModel(Model{}, 1u);
		for (size_t index = 0u; index < 5u; ++index)
			modelBundle->AddModel(Model{}, 0u);

		std::uint32_t index = vsIndirect.AddModelBundle(std::move(modelBundle));

		modelBuffers.ExtendModelBuffers();

		EXPECT_EQ(index, 2u) << "Index isn't 2.";
	}

	::RemoveModelBundle(*modelContainer, vsIndirect.RemoveModelBundle(1u));

	{
		auto modelBundle = std::make_shared<ModelBundle>();

		modelBundle->SetModelContainer(modelContainer);

		for (size_t index = 0u; index < 7u; ++index)
			modelBundle->AddModel(Model{}, 0u);
		for (size_t index = 0u; index < 5u; ++index)
			modelBundle->AddModel(Model{}, 1u);

		std::uint32_t index = vsIndirect.AddModelBundle(std::move(modelBundle));

		modelBuffers.ExtendModelBuffers();

		EXPECT_EQ(index, 1u) << "Index isn't 1.";
	}
	{
		auto modelBundle = std::make_shared<ModelBundle>();

		modelBundle->SetModelContainer(modelContainer);

		modelBundle->AddModel(Model{}, 0u);

		std::uint32_t index = vsIndirect.AddModelBundle(std::move(modelBundle));

		modelBuffers.ExtendModelBuffers();

		EXPECT_EQ(index, 3u) << "Index isn't 3.";
	}
	{
		auto modelBundle = std::make_shared<ModelBundle>();

		modelBundle->SetModelContainer(modelContainer);

		modelBundle->AddModel(Model{}, 0u);

		std::uint32_t index = vsIndirect.AddModelBundle(std::move(modelBundle));

		modelBuffers.ExtendModelBuffers();

		EXPECT_EQ(index, 4u) << "Index isn't 4.";
	}
	{
		auto modelBundle = std::make_shared<ModelBundle>();
		auto pipeline    = PipelineModelBundle{};
		auto pipeline1   = PipelineModelBundle{};
		auto pipeline2   = PipelineModelBundle{};

		modelBundle->SetModelContainer(modelContainer);

		pipeline.SetPipelineIndex(1u);
		pipeline1.SetPipelineIndex(2u);
		pipeline2.SetPipelineIndex(3u);

		modelBundle->AddPipeline(std::move(pipeline));
		modelBundle->AddPipeline(std::move(pipeline1));
		modelBundle->AddPipeline(std::move(pipeline2));

		for (size_t index = 0u; index < 7u; ++index)
			modelBundle->AddModel(Model{}, 1u);

		for (size_t index = 0u; index < 5u; ++index)
			modelBundle->AddModel(Model{}, 3u);

		for (size_t index = 0u; index < 3u; ++index)
			modelBundle->AddModel(Model{}, 2u);

		std::shared_ptr<ModelBundle> modelBundle1 = modelBundle;

		std::uint32_t index = vsIndirect.AddModelBundle(std::move(modelBundle1));

		modelBuffers.ExtendModelBuffers();

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

	auto modelContainer = std::make_shared<ModelContainer>();

	ModelBuffers modelBuffers{ logicalDevice, &memoryManager, Constants::frameCount, {} };

	modelBuffers.SetModelContainer(modelContainer);

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
		auto modelBundle = std::make_shared<ModelBundle>();

		modelBundle->SetModelContainer(modelContainer);

		modelBundle->AddModel(Model{}, 0u);

		std::uint32_t index = managerMS.AddModelBundle(std::move(modelBundle));

		modelBuffers.ExtendModelBuffers();

		EXPECT_EQ(index, 0u) << "Index isn't 0.";
	}
	{
		auto modelBundle = std::make_shared<ModelBundle>();

		modelBundle->SetModelContainer(modelContainer);

		modelBundle->AddModel(Model{}, 0u);

		std::uint32_t index = managerMS.AddModelBundle(std::move(modelBundle));

		modelBuffers.ExtendModelBuffers();

		EXPECT_EQ(index, 1u) << "Index isn't 1.";
	}
	{
		auto modelBundle = std::make_shared<ModelBundle>();

		modelBundle->SetModelContainer(modelContainer);

		for (size_t index = 0u; index < 5u; ++index)
			modelBundle->AddModel(Model{}, 1u);
		for (size_t index = 0u; index < 5u; ++index)
			modelBundle->AddModel(Model{}, 0u);

		std::uint32_t index = managerMS.AddModelBundle(std::move(modelBundle));

		modelBuffers.ExtendModelBuffers();

		EXPECT_EQ(index, 2u) << "Index isn't 2.";
	}

	::RemoveModelBundle(*modelContainer, managerMS.RemoveModelBundle(1u));

	{
		auto modelBundle = std::make_shared<ModelBundle>();

		modelBundle->SetModelContainer(modelContainer);

		for (size_t index = 0u; index < 7u; ++index)
			modelBundle->AddModel(Model{}, 0u);
		for (size_t index = 0u; index < 5u; ++index)
			modelBundle->AddModel(Model{}, 1u);

		std::uint32_t index = managerMS.AddModelBundle(std::move(modelBundle));

		modelBuffers.ExtendModelBuffers();

		EXPECT_EQ(index, 1u) << "Index isn't 1.";
	}
	{
		auto modelBundle = std::make_shared<ModelBundle>();
		auto pipeline    = PipelineModelBundle{};
		auto pipeline1   = PipelineModelBundle{};
		auto pipeline2   = PipelineModelBundle{};

		modelBundle->SetModelContainer(modelContainer);

		pipeline.SetPipelineIndex(1u);
		pipeline1.SetPipelineIndex(2u);
		pipeline2.SetPipelineIndex(3u);

		modelBundle->AddPipeline(std::move(pipeline));
		modelBundle->AddPipeline(std::move(pipeline1));
		modelBundle->AddPipeline(std::move(pipeline2));

		for (size_t index = 0u; index < 7u; ++index)
			modelBundle->AddModel(Model{}, 1u);

		for (size_t index = 0u; index < 5u; ++index)
			modelBundle->AddModel(Model{}, 3u);

		for (size_t index = 0u; index < 3u; ++index)
			modelBundle->AddModel(Model{}, 2u);

		std::shared_ptr<ModelBundle> modelBundle1 = modelBundle;

		std::uint32_t index = managerMS.AddModelBundle(std::move(modelBundle1));

		modelBuffers.ExtendModelBuffers();

		EXPECT_EQ(index, 3u) << "Index isn't 3.";

		modelBundle->ChangeModelPipeline(7u, 1u, 3u);
		managerMS.ReconfigureModels(index, 1u, 3u);
	}
}

