#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <Eigen/Dense>

#include <cstddef>
#include <cstdint>
#include <cstdlib> // TODO check for possible removal
#include <cstdio> // TODO check its usage for possible removal in non debug builds
#include <cstring>
#include <cassert>

#include <filesystem> // path
#include <fstream> // ifstream
#include <string_view> // unused yet
#include <vector>
#include <span> // unused yet
#include <unordered_set>
#include <algorithm> // copy, unstable_sort, transform, unique
#include <type_traits> // is_standard_layout

// here just for the copied allocator
#include <new>
#include <limits>
#include <iostream>

// TODO IMPORTANT BUG: when opened from executable the game doesn't lauch. Thats because the build doesn't contain the compiled shaders. 
//		Either change current directory or at build copy shader folders (only .spv files)
// TODO change naming convention from snake_case to camelCase for vars and PascalCase for types
// TODO register_extensions and setupInstance manage a lot of memory, and they should have allocated
// 		void* in the beginning as working buffers, instead of allocating on demand, therefore reducing
// 		number of allocations and casts needed
// TODO remove all std:: usage (except for platform abstraction and type support facilities). example: remove std::vector
// TODO setup macro for compiler specific restrict keyword. Eg. __restrict__ for g++/clang, __restrict for MSVC, and forceinline, attribute(force_inline) for g++/clang, and __force_inline__ for MSVC
// TODO add constexpr where fit
// TODO setup documentation before source grows too much
// TODO modify app class so that it can support having multiple layers
// TODO custom allocation
// TODO once you write a user-defined constructor, syntesized move constructor and move assignment operators are disabled by default. Write them
// TODO -fno-exceptions
// TODO write allocator, example from cppreference
template<typename T>
struct Mallocator
{
    using value_type = T;
	constexpr Mallocator() = default;
    template<class U>
    constexpr Mallocator (const Mallocator <U>&) noexcept {}
    [[nodiscard]] T* allocate(std::size_t n)
    {
        if (n > std::numeric_limits<std::size_t>::max() / sizeof(T))
            throw std::bad_array_new_length();
        if (auto p = static_cast<T*>(std::malloc(n * sizeof(T))))
        {
            report(p, n);
            return p;
        }
        throw std::bad_alloc();
    }
    void deallocate(T* p, std::size_t n) noexcept
    {
        report(p, n, 0);
        std::free(p);
    }
private:
    void report(T* p, std::size_t n, bool alloc = true) const
    {
        std::cout << (alloc ? "Alloc: " : "Dealloc: ") << sizeof(T) * n
                  << " bytes at " << std::hex << std::showbase
                  << reinterpret_cast<void*>(p) << std::dec << '\n';
    }
};

template<class T, class U>
bool operator==(const Mallocator <T>&, const Mallocator <U>&) { return true; }

template<class T, class U>
bool operator!=(const Mallocator <T>&, const Mallocator <U>&) { return false; }
// NOTE: glfwGetGammaRamp to get the monitor's gamma
// NOTE: to get started more "smoothly", I WILL NOT DO CUSTOM ALLOCATION IN THIS MOCK APPLICATION
// 		 subsequent Vulkan trainings will include more as I learn from scratch graphics development

#define WINDOW_WIDTH 600
#define WINDOW_HEIGHT 480

namespace mxc
{
	using status_t = uint32_t;
	#define APP_SUCCESS 0
	#define APP_GENERIC_ERR 1
	#define APP_MEMORY_ERR 2
	#define APP_LACK_OF_VULKAN_1_2_SUPPORT_ERR 3
	#define APP_INIT_FAILURE 4
	#define APP_QUEUE_ERR 5
	#define APP_DEVICE_CREATION_ERR 6
	#define APP_SWAPCHAIN_CREATION_ERR 7
	#define APP_VK_ALLOCATION_ERR 8
	#define APP_REQUIRES_RESIZE_ERR 9
	//... more errors
	
	struct Vertex
	{
		Eigen::Vector3f pos;
		Eigen::Vector3f col;
	};
	static_assert(std::is_standard_layout_v<Vertex>);
	// TODO change return conventions into more meaningful and specific error carrying type to convey a more 
	// 		specific status report
	/*** WARNING: most of the functions will return APP_TRUE if successful, and APP_FALSE if unsuccessful ***/
	struct VulkanPipelineConfig 
	{
		VkPipelineVertexInputStateCreateInfo vertexInputStateCI {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.vertexBindingDescriptionCount = 0,
			.pVertexBindingDescriptions = nullptr,
			.vertexAttributeDescriptionCount = 0,
			.pVertexAttributeDescriptions = nullptr
		};

		VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCI {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
			.primitiveRestartEnable = VK_FALSE
		};

		VkPipelineTessellationStateCreateInfo tessellationStateCI {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.patchControlPoints = 0
		};

		VkPipelineViewportStateCreateInfo viewportStateCI {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
			.pNext = nullptr, 
			.flags = 0,
			.viewportCount = 0,
			.pViewports = nullptr,
			.scissorCount = 0,
			.pScissors = nullptr 
		};

		VkPipelineRasterizationStateCreateInfo rasterizationStateCI {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.depthClampEnable = VK_FALSE,
			.rasterizerDiscardEnable = VK_FALSE,
			.polygonMode = VK_POLYGON_MODE_FILL,
			.cullMode = VK_CULL_MODE_BACK_BIT,
			.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
			.depthBiasEnable = VK_FALSE,
			.depthBiasConstantFactor = 0.f,
			.depthBiasClamp = 0.f,
			.depthBiasSlopeFactor = 0.f,
			.lineWidth = 1.f,
		};

		VkPipelineMultisampleStateCreateInfo multisampleStateCI {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
			.sampleShadingEnable = VK_FALSE,
			.minSampleShading = 0.f,
			.pSampleMask = nullptr,
			.alphaToCoverageEnable = VK_FALSE,
			.alphaToOneEnable = VK_FALSE
		};

		//VkStencilOpState constexpr stencilOpState {
		//		.failOp = VK_STENCIL_OP_KEEP, // what to do if stencil test fails
		//		.passOp = VK_STENCIL_OP_KEEP, // what to do if stencil test is successful
		//		.depthFailOp = VK_STENCIL_OP_KEEP, // passes stencil but fails depth test
		//		.compareOp = VK_COMPARE_OP_ALWAYS, // specifies stencil test operation. same type as depth comparison, but with another meaning
		//		.compareMask = 0, // selects the bits of the unsigned integer values participating in the stencil test
		//		.writeMask = 0, // selects the bits of the unsigned integer values updated by the stencil test in the stencil framebuffer attachment
		//		.reference = 0 // integer stencil reference value used in some of the compare ops
		//}
		VkPipelineDepthStencilStateCreateInfo depthStencilStateCI {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.depthTestEnable = VK_FALSE,
			.depthWriteEnable = VK_FALSE,
			.depthCompareOp = VK_COMPARE_OP_LESS,
			.depthBoundsTestEnable = VK_FALSE,
			.stencilTestEnable = VK_FALSE,
			.front = {VK_STENCIL_OP_KEEP,VK_STENCIL_OP_KEEP,VK_STENCIL_OP_KEEP,VK_COMPARE_OP_ALWAYS,0,0,0},
			.back = {VK_STENCIL_OP_KEEP,VK_STENCIL_OP_KEEP,VK_STENCIL_OP_KEEP,VK_COMPARE_OP_ALWAYS,0,0,0},
			.minDepthBounds = 0.f,
			.maxDepthBounds = 0.f
		};

		VkPipelineColorBlendStateCreateInfo colorBlendStateCI {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.logicOpEnable = VK_FALSE,
			.logicOp = VkLogicOp{},
			.attachmentCount = 1u,
			.pAttachments = nullptr,
			.blendConstants = {}
		};

		VkPipelineDynamicStateCreateInfo dynamicStateCI {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.dynamicStateCount = 0,
			.pDynamicStates = nullptr
		};
	};

	template <template<class> class AllocTemplate = std::allocator>
	class Renderer
	{
	public: // type shortcuts
		template <typename T>
		using VectorCustom = std::vector<T, AllocTemplate<T>>;

	public: // constructors
		Renderer();
		Renderer(Renderer const&) = delete;
		auto operator=(Renderer const&) -> Renderer = delete;
		// TODO define move semantics
		~Renderer();
		
	public: // public functions, initialization procedures
		// TODO refactor so that you take a raw array. it is too restrictive to constraint api caller to use a specific class data structure
		auto init(std::span<char const*> desiredInstanceExtensions, 
				  std::span<char const*> desiredDeviceExtensions, 
				  GLFWwindow* window, uint32_t width, uint32_t height,
				  std::span<Vertex> vertexInput, std::span<uint32_t> indexInput) & -> status_t;  // TODO vert and indices are Temp, need to refactor in their own class
		// TODO init arguments refactored in a customizeable struct, create swapchain only if requested, and create a window class
		auto draw() & -> status_t;
		auto resize(uint32_t width, uint32_t height) & -> status_t; // TODO recreate swapchain only if swapchain has been requested

	public: // public function, utilities
		auto progress_incomplete() const & -> status_t; // TODO: const correct and ref correct members

	private: // setup vulkan objects functions, called by init
		auto setupInstance(std::span<char const*> const& desiredExtensions) & -> status_t;
		auto setupSurfaceKHR(GLFWwindow* window) & -> status_t; // TODO refactor
		auto setupPhyDevice(std::span<char const*> const& desiredDeviceExtensions) -> status_t;
		auto setupDeviceAndQueues(std::span<char const*> const& deviceExtensions) & -> status_t;
		auto setupSwapchain(uint32_t width, uint32_t height) & -> status_t;
		auto setupCommandBuffers() & -> status_t;
		// auto setupStagingBuffer() & -> status_t;
		auto setupDepthImage() & -> status_t;
		auto setupDepthDeviceMemory() & -> status_t;
		auto setupRenderPass() & -> status_t;
		auto setupFramebuffers() & -> status_t;
		auto setupGraphicsPipeline() & -> status_t;
		auto recordCommands(uint32_t framebufferIdx) & -> status_t;
		auto setupSynchronizationObjects() & -> status_t;
		auto setupVertexInput(std::span<Vertex> vertexInput, std::span<uint32_t> indexInput) & -> status_t;

		// frequently called
		// auto createBuffer(/**/) & -> status_t;
		// auto createImage(/**/) & -> status_t;
		auto checkMemoryRequirements(VkMemoryRequirements const& memoryRequirements, VkMemoryPropertyFlagBits const& requestedMemoryProperties, uint32_t* outMemoryTypeIndex) & -> status_t;

	private: // data members, dispatchable and non dispatchable vulkan objects handles
		// vulkan initialization members
		VkInstance m_instance;
		VkPhysicalDevice m_phyDevice;
		VkDevice m_device;

		// children of VkDevice
		struct queues_t
		{
			// NOTE: graphics queue and presentation queue could be the same
			int32_t graphics;
			int32_t presentation;
		};
		#define MXC_RENDERER_QUEUES_COUNT sizeof(queues_t)/sizeof(int32_t)
		#define MXC_RENDERER_GRAPHICS_QUEUES_COUNT 1
		union // anonymous unions cannot possess anonymous structs. anonymous unions do NOT define a type, but a variable
		{
			int32_t m_queueIdxArr[MXC_RENDERER_QUEUES_COUNT]; 
			queues_t m_queueIdx;
		};
		VkQueue m_queues[MXC_RENDERER_QUEUES_COUNT];

		VkCommandPool m_graphicsCmdPool;	//cmdPools can also be trimmed or reset
		VectorCustom<VkCommandBuffer> m_graphicsCmdBufs; // these two are created/allocated for the m_queueIdx.graphics queue

		VkRenderPass m_renderPass;
		// color output attachment and depth output attachment
		#define MXC_RENDERER_ATTACHMENT_COUNT 2
		VkImage m_depthImage;
		VkImageView m_depthImageView;
		VkDeviceMemory m_depthImageMemory;

		// cmdbuf count = framebuffer count = swapchain images count = semaphores count = fences count
		VectorCustom<VkFramebuffer> m_framebuffers; // triple buffering
			
		#define MXC_RENDERER_SHADERS_COUNT 2
		VkPipeline m_graphicsPipeline;
		VkPipelineLayout m_graphicsPipelineLayout;

		VectorCustom<VkFence> m_fenceInFlightFrame;
		// we are using triple buffering, so at least 2 pairs of semaphores should be created, to be safe we will associate a pair of semaphores to each framebuffer
		VectorCustom<VkSemaphore> m_semaphoreImageAvailable;
		VectorCustom<VkSemaphore> m_semaphoreRenderFinished;

		// presentation WSI extension
		VkSurfaceKHR m_surface;
		VkSurfaceFormatKHR m_surfaceFormatUsed;
		VkPresentModeKHR m_presentModeUsed;
		VkSurfaceCapabilitiesKHR m_surfaceCapabilities; // TODO remember to check these in the creation of swapchain, pipeline, ...
		VkSwapchainKHR m_swapchain;
		VectorCustom<VkImage> m_swapchainImages;
		VectorCustom<VkImageView> m_swapchainImageViews;
		VkExtent2D m_surfaceExtent;

		// other vulkan related
		VkFormat m_depthImageFormat;

		VkBuffer m_stagingBuffer;
		VkBuffer m_vertexBuffer;
		VkBuffer m_indexBuffer;
		VkDeviceMemory m_inputBuffersMemory; // TODO refactor everything so that there is only ONE VkDevice Memory, or at least fewer of them
		VkDeviceMemory m_stagingBufferMemory;

#ifndef NDEBUG // CMAKE_BUILD_TYPE=Debug
		VkDebugUtilsMessengerEXT m_dbgMessenger;
		#define MXC_RENDERER_INSERT_MESSENGER MESSENGER_CREATED
#elif
		#define MXC_RENDERER_INSERT_MESSENGER 0
#endif // ifndef NDEBUG

		// renderer state machine to TODO refactor. type is below
		uint32_t m_progressStatus;

	private: // data members, utilities
		enum m_Progress_t : uint32_t
		{
			INITIALIZED = 0x80000000,
			INSTANCE_CREATED = 0x40000000,
#ifndef NDEBUG // CMAKE_BUILD_TYPE=Debug
			MESSENGER_CREATED = 0x20000000,
#endif // ifndef NDEBUG
			PHY_DEVICE_GOT = 0x10000000,
			DEVICE_CREATED = 0x08000000,
			SURFACE_CREATED = 0x0400000,
			SWAPCHAIN_CREATED = 0x02000000,
			SWAPCHAIN_IMAGE_VIEWS_CREATED = 0x01000000,
			COMMAND_BUFFER_ALLOCATED = 0x00800000,
			RENDERPASS_CREATED = 0x00200000,
			DEPTH_IMAGE_CREATED = 0x00100000,
			DEPTH_MEMORY_ALLOCATED = 0x00008000,
			FRAMEBUFFERS_CREATED = 0x00080000,
			GRAPHICS_PIPELINE_LAYOUT_CREATED = 0x00040000,
			GRAPHICS_PIPELINE_CREATED = 0x00020000,
			SYNCHRONIZATION_OBJECTS_CREATED = 0x00010000,
			VERTEX_INPUT_BOUND = 0x00008000
		};
	
	private: // functions, utilities

		#ifndef NDEBUG
		auto dbgPrintInstanceExtensionsAndLayers(std::span<const char*> const& instanceExtensions, std::span<char const*> const& layers) -> void;
		#endif
	};

	template <template<class> class AllocTemplate> Renderer<AllocTemplate>::Renderer() 
			: m_instance(VK_NULL_HANDLE), m_phyDevice(VK_NULL_HANDLE), m_device(VK_NULL_HANDLE)
			, m_queueIdxArr{-1}, m_queues{VK_NULL_HANDLE} // TODO Don't forget to update m_queueIdxArr when adding queue types
			, m_graphicsCmdPool(VK_NULL_HANDLE), m_graphicsCmdBufs(VectorCustom<VkCommandBuffer>(0)), m_renderPass(VK_NULL_HANDLE)
			, m_framebuffers(VectorCustom<VkFramebuffer>()), m_graphicsPipeline(VK_NULL_HANDLE), m_graphicsPipelineLayout(VK_NULL_HANDLE)
			, m_fenceInFlightFrame(VectorCustom<VkFence>()), m_semaphoreImageAvailable(VectorCustom<VkSemaphore>()), m_semaphoreRenderFinished(VectorCustom<VkSemaphore>()), m_depthImage(VK_NULL_HANDLE), m_depthImageView(VK_NULL_HANDLE)
			, m_depthImageMemory(VK_NULL_HANDLE), m_surface(VK_NULL_HANDLE), m_surfaceFormatUsed(VK_FORMAT_UNDEFINED), m_presentModeUsed(VK_PRESENT_MODE_FIFO_KHR), m_surfaceCapabilities({0})
			, m_swapchain(VK_NULL_HANDLE), m_swapchainImages(VectorCustom<VkImage>()), m_swapchainImageViews(VectorCustom<VkImageView>())
			, m_surfaceExtent(VkExtent2D{0,0}), m_depthImageFormat(VK_FORMAT_D32_SFLOAT), m_stagingBuffer(VK_NULL_HANDLE), m_vertexBuffer(VK_NULL_HANDLE), m_indexBuffer(VK_NULL_HANDLE), m_inputBuffersMemory(VK_NULL_HANDLE), m_stagingBufferMemory(VK_NULL_HANDLE)
#ifndef NDEBUG // CMAKE_BUILD_TYPE=Debug
			, m_dbgMessenger(VK_NULL_HANDLE)
#endif
			, m_progressStatus(0u)
	{
	}				
template <template<class> class AllocTemplate> 
	auto Renderer<AllocTemplate>::init(std::span<char const*> desiredInstanceExtensions, 
									   std::span<char const*> desiredDeviceExtensions,
									   GLFWwindow* window, uint32_t width, uint32_t height,
									   std::span<Vertex> vertexInput, std::span<uint32_t> indexInput) & -> status_t
	{
		if (setupInstance(desiredInstanceExtensions)
			|| setupSurfaceKHR(window) // TODO work in progress, refactor to another class, like "rendererWindowAdaptor" to make renderer and window loosely coupled
			|| setupPhyDevice(desiredDeviceExtensions)
			|| setupDeviceAndQueues(desiredDeviceExtensions)
			|| setupSwapchain(width, height)
			|| setupCommandBuffers()
			|| setupDepthImage()
			|| setupDepthDeviceMemory()
			|| setupVertexInput(vertexInput, indexInput)
			|| setupRenderPass()
			|| setupFramebuffers()
			|| setupGraphicsPipeline()
			|| setupSynchronizationObjects())
		{
			return APP_GENERIC_ERR;
		}

		m_progressStatus |= INITIALIZED;
		return APP_SUCCESS;
	}

	template <template<class> class AllocTemplate> 
	Renderer<AllocTemplate>::~Renderer()
	{
		printf("time to destroy renderer!\n");
		// wait until the device is idle to clean resources so we don't break anything with VkFences
		if (m_progressStatus & DEVICE_CREATED)
		{	
			vkDeviceWaitIdle(m_device);
		}

		// destroy vertex and index buffers
		vkDestroyBuffer(m_device, m_stagingBuffer, /*VkAllocationCallbacks**/nullptr);
		vkDestroyBuffer(m_device, m_vertexBuffer, /*VkAllocationCallbacks**/nullptr);
		vkDestroyBuffer(m_device, m_indexBuffer, /*VkAllocationCallbacks**/nullptr);
		vkFreeMemory(m_device, m_inputBuffersMemory, /*VkALlocationCallbacks**/nullptr);
		vkFreeMemory(m_device, m_stagingBufferMemory, /*VkAllocationCallbacks**/nullptr);

		// Then we can clean everything up. Note that we do not check for successful initialization. That's because
		// the vkDestroy and vkDeallocate functions can be called when the handle to be destroyed/freed is VK_NULL_HANDLE 
		for (uint32_t i = 0u; i < m_swapchainImages.size(); ++i)
		{
			vkDestroyFence(m_device, m_fenceInFlightFrame[i], /*VkAllocationCallbacks**/nullptr);
			vkDestroySemaphore(m_device, m_semaphoreImageAvailable[i], /*VkAllocationCallbacks**/nullptr);
			vkDestroySemaphore(m_device, m_semaphoreRenderFinished[i], /*VkAllocationCallbacks**/nullptr);
		}

		vkDestroyPipeline(m_device, m_graphicsPipeline, /*VkAllocationCallbacks**/nullptr);
		vkDestroyPipelineLayout(m_device, m_graphicsPipelineLayout, /*VkAllocationCallbacks**/nullptr);

		// TODO vkDestroyFramebuffer times 3, then free memory 
		for (uint32_t i = 0; i < m_swapchainImages.size(); ++i)
		{
			vkDestroyFramebuffer(m_device, m_framebuffers[i], /*VkAllocationCallbacks**/nullptr);
		}
		
		// destroy depth buffer
		printf("remember to clean up device memory!\n");
		vkFreeMemory(m_device, m_depthImageMemory, /*VkAllocationCallbacks**/nullptr);
		vkDestroyImageView(m_device, m_depthImageView, /*VkAllocationCallbacks**/nullptr);
		vkDestroyImage(m_device, m_depthImage, /*VkAllocationCallbacks**/nullptr);
	
		vkDestroyRenderPass(m_device, m_renderPass, /*VkAllocationCallbacks**/nullptr);
	
		vkFreeCommandBuffers(m_device, m_graphicsCmdPool, static_cast<uint32_t>(m_swapchainImages.size()), m_graphicsCmdBufs.data());
		vkDestroyCommandPool(m_device, m_graphicsCmdPool, /*VkAllocationCallbacks**/nullptr);

		for (uint32_t i = 0u; i < m_swapchainImages.size(); ++i)
		{
			vkDestroyImageView(m_device, m_swapchainImageViews[i], /*VkAllocationCallback**/nullptr);
		}

		vkDestroySwapchainKHR(m_device, m_swapchain, /*VkAllocationCallbacks**/nullptr);
		vkDestroySurfaceKHR(m_instance, m_surface, /*VkAllocationCallbacks**/nullptr);

		// all its child objects need to be destroyed before doing this
		vkDestroyDevice(m_device, /*VkAllocationCallbacks**/nullptr);

#ifndef NDEBUG // CMAKE_BUILD_TYPE=Debug
		auto const vkDestroyDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
			vkGetInstanceProcAddr(m_instance, "vkDestroyDebugUtilsMessengerEXT")
		);
		if (vkDestroyDebugUtilsMessengerEXT)
		{
			vkDestroyDebugUtilsMessengerEXT(m_instance, m_dbgMessenger, /*pAllocationCallbacks**/nullptr);
		}
#endif

		vkDestroyInstance(m_instance, /*VkAllocationCallbacks*/nullptr);
		printf("destroyed renderer!\n");
	}
	
	//
#ifndef NDEBUG // CMAKE_BUILD_TYPE=Debug TODO
	// by default, Vulkan's validation layers print all their output to the STDOUT. we can decide to handle the report of validation layers
	// ourselves by defining a function callback with format predefined by the specification, ONLY AFTER enabling the extension "VK_EXT_debug_utils"
	// such string constant can be also written by typing the macro VK_EXT_DEBUG_UTILS_EXTENSION_NAME
	/** types with the following signature: VkDebugUtils*EXT
	 * VkDebugUtilsMessageSeverityFlagBitsEXT = number which is one of the following ( VK_DEBUG_UTILS_MESSAGE_SEVERITY_<content>_BIT_EXT )
	 * 		VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT -> diagnostic message
	 * 		<= VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT -> report message, eg creation of a VkInstance
	 * 		<= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT -> warning message
	 * 		<= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT -> the one you don't want to see
	 * 		their ordering can be used to filter messages "not important enough"
	 * 	VkDebugUtilsMessageTypeFlagsEXT = again a bit field with names VK_DEBUG_UTILS_MESSAGE_TYPE_*_BIT_EXT
	 * 		VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT -> the message reports the happening of an event unrelated with the specification(ie errors) or performance(suboptimal usage of vulkan)
	 * 		VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT -> message reports violation of specification
	 * 		VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT -> message reports suboptimal usage of vulkan
	 * 	VkDebugUtilsMessengerCallbackDataEXT is a struct containing the message itself + some info about it
	 * 		pMessage -> pointer to a nul-terminated string
	 * 		pObjects -> array of vulkan objects handles related to the message
	 * 		objectCount
	 * 	pUserData -> pointer specified during the setup of the callback containing custom data
	 *
	 * 	the return bool of the callback reports whether the Vulkan call which triggered it should be aborted or not. This is normally used to 
	 * 	test the validation itself, and should always return VK_FALSE otherwise
	 */
	VKAPI_ATTR
	static auto VKAPI_CALL debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
		VkDebugUtilsMessageTypeFlagsEXT message_type,
		VkDebugUtilsMessengerCallbackDataEXT const* pcallback_data,
		void* puser_data
	) -> VkBool32
	{
		// TODO maybe it's better to put it in a file?
		fprintf(stderr, "%s\n", pcallback_data->pMessage);
		return VK_FALSE;

	}
	template <template<class> class AllocTemplate> 
	auto Renderer<AllocTemplate>::dbgPrintInstanceExtensionsAndLayers(std::span<char const*> const& instanceExtensions, std::span<char const*> const& layers) -> void
	{
		FILE* const file = fopen("./ext_and_layers_names.txt", "w");
		if (file)
		{
			fprintf(file, "extensions:\n");
			for (uint32_t i = 0u; i < instanceExtensions.size(); ++i)
			{
				fprintf(file, "%s\n", instanceExtensions[i]);
			}
			fprintf(file, "validation layers:\n");
			for (uint32_t i = 0u; i < layers.size(); ++i)
			{
				fprintf(file, "%s\n", layers[i]);
			}
			fclose(file);
			printf("instance extensions and layers debug file created!\n");
		}
		else
		{
			printf("no debug file could be created!");
		}
	}
#endif // ifndef NDEBUG

	template <template<class> class AllocTemplate> 
	auto Renderer<AllocTemplate>::setupInstance(std::span<char const*> const& desiredInstanceExtensions) & -> status_t
	{
     	// -- Check for vulkan 1.2 support ------------------------------------------------------------------
		uint32_t supported_vk_api_version;
		vkEnumerateInstanceVersion(&supported_vk_api_version);	
       	if (supported_vk_api_version < VK_MAKE_API_VERSION(/*variant*/0,/*major*/1,/*minor*/2,/*patch*/0))
		{
			// TODO substitute with a primitive logging with pretty printing, enabled only in Debug
       		fprintf(stderr, "system does not support vulkan 1.2 or higher!");
			return APP_LACK_OF_VULKAN_1_2_SUPPORT_ERR;
    	}

		// -- Check for desired instance extensions support ---------------------------------------------------
		uint32_t extensionPropertyCnt;
		vkEnumerateInstanceExtensionProperties(/*layer*/nullptr, &extensionPropertyCnt, nullptr);
		VectorCustom<VkExtensionProperties> extensionProperties(extensionPropertyCnt);
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionPropertyCnt, extensionProperties.data());

		for (uint32_t i = 0; i < desiredInstanceExtensions.size(); ++i)
		{
			uint32_t j = 0;
			for (; j < extensionPropertyCnt; ++j)
			{
				if (strcmp(desiredInstanceExtensions[i], extensionProperties[j].extensionName) == 0)
				{
					break;
				}
			}
			if (j == desiredInstanceExtensions.size())
			{
				fprintf(stderr, "couldn't find extension %s\n", extensionProperties[i].extensionName);
				return APP_GENERIC_ERR;
			}
		}

		// -- setup validation layers and messenger ---------------------------------------------------------------------------------------------------------
		VectorCustom<char const*> desiredValidationLayers {
			"VK_LAYER_KHRONOS_validation"
		};
		uint32_t enabledLayerCnt = 0u;
		VkDebugUtilsMessengerCreateInfoEXT const* dbgMsgerPtr = nullptr;

		#ifndef NDEBUG // CMAKE_BUILD_TYPE=Debug

		// create info for the debug messenger, passed first as extension to the instance create info and then used to create msger
		VkDebugUtilsMessengerCreateInfoEXT const dbgMsgerCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
			.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
							 | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT
							 | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
							 | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,	// select which msg severities you want the messenger to report
			.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
						 | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
						 | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,		// select which msg types you want the messenger to report
			.pfnUserCallback = debugCallback,									// select which function to use as callback
			.pUserData = nullptr												// selects user data to pass as 4th param to the callback. OPTIONAL
		};
		dbgMsgerPtr = &dbgMsgerCreateInfo;
	
		// check desired validation layers for support
		uint32_t layerPropertyCnt;
		vkEnumerateInstanceLayerProperties(&layerPropertyCnt, nullptr);
		VectorCustom<VkLayerProperties> layerProperties(layerPropertyCnt);
		vkEnumerateInstanceLayerProperties(&layerPropertyCnt, layerProperties.data());

		for (uint32_t i = 0u; i < desiredValidationLayers.size(); ++i)
		{
			for (uint32_t j = 0u; j < layerPropertyCnt; ++j)
			{
				if (0 == strncmp(layerProperties[j].layerName, desiredValidationLayers[i], VK_MAX_EXTENSION_NAME_SIZE))
				{
					++enabledLayerCnt;
					break;
				}
			}
		}
		if (enabledLayerCnt == 0u)
		{
			fprintf(stderr, "no layers were available!\n");
		}

		#endif // ifndef NDEBUG
	
		// -- create the vulkan instance --------------------------------------------------------------------------------------------------
		VkApplicationInfo const applicationInfo = {
			.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
			.pNext = nullptr,
			.pApplicationName = "my first vulkan app",
			.applicationVersion = 1u,
			.pEngineName = "my first vulkan app",
			.engineVersion = 1u,
			.apiVersion = VK_MAKE_API_VERSION(/*variant*/0,/*major*/1,/*minor*/2,/*patch*/0) // the only useful thing here
		};

		VkInstanceCreateInfo const instanceCreateInfo{
			.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
			.pNext = dbgMsgerPtr,
			.flags = 0, // Bitmask on some options that define the BEHAVIOUR OF THE INSTANCE
			.pApplicationInfo = &applicationInfo,
			.enabledLayerCount = enabledLayerCnt, // enabling ALL layers for now TODO
			.ppEnabledLayerNames = desiredValidationLayers.data(),
			.enabledExtensionCount = static_cast<uint32_t>(desiredInstanceExtensions.size()),
			.ppEnabledExtensionNames = desiredInstanceExtensions.data()
		};

		VkResult result = vkCreateInstance(&instanceCreateInfo, /*VkAllocationCallbacks**/nullptr, &m_instance);
		if (result != VK_SUCCESS)
		{
			fprintf(stderr, "failed to create an instance!\n");
			return APP_GENERIC_ERR;
		}

		m_progressStatus |= INSTANCE_CREATED;
		printf("instance created!\n");

		// -- create debug messenger --------------------------------------------------------------------------------------
#ifndef NDEBUG // CMAKE_BUILD_TYPE=Debug
		// since this is an extension function, it is not automatically loaded and we need to call vkGetInstanceProcAddr(insance, funcname) ourselves
		auto const vkCreateDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(m_instance, "vkCreateDebugUtilsMessengerEXT"));
		if (!vkCreateDebugUtilsMessengerEXT)
		{
			return VK_ERROR_EXTENSION_NOT_PRESENT;
		}

		result = vkCreateDebugUtilsMessengerEXT(m_instance, &dbgMsgerCreateInfo, /*VkAllocationCallbacks**/nullptr, &m_dbgMessenger);
		if (result != VK_SUCCESS)
		{
			return APP_GENERIC_ERR;
		}

		m_progressStatus |= MESSENGER_CREATED;
		dbgPrintInstanceExtensionsAndLayers(desiredInstanceExtensions, desiredValidationLayers);
#endif // ifndef NDEBUG	
		return APP_SUCCESS;
	}

	template <template<class> class AllocTemplate> 
	auto Renderer<AllocTemplate>::setupPhyDevice(std::span<char const*> const& desiredDeviceExtensions) -> status_t
	{
		assert(m_progressStatus & INSTANCE_CREATED);
		assert(m_progressStatus & SURFACE_CREATED);

		// variable used in all enumerate functions present here
		uint32_t enumerateCounter;

		// -- retrieve list of vulkan capable physical devices ----------------------------------------------------------------
		vkEnumeratePhysicalDevices(m_instance, &enumerateCounter, nullptr);
		VectorCustom<VkPhysicalDevice> phyDevices(enumerateCounter);
		vkEnumeratePhysicalDevices(m_instance, &enumerateCounter, phyDevices.data());

		// then you can query the properties of the physical device with vkGetPhysicalDeviceProperties and choose the one
		// that suits your needs. I understand none of that, therefore I'm going to choose the first available GPU
		// you have no idea how many more get properties functions there are.
		// we will also query the available queue families. for now we need a graphics queue and a presentation enabled queue (WSI)
		for (uint32_t i = 0u; i < phyDevices.size(); ++i)
		{
			// -- query physical device properties. Most relevant: limits and sparse properties TODO -------------------------------------------------------------------
			VkPhysicalDeviceProperties phyDeviceProperties;
			vkGetPhysicalDeviceProperties(phyDevices[i], &phyDeviceProperties);
			if (phyDeviceProperties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
			{
				printf("this is not a dedicated GPU!\n");
			}

			// -- query features TODO ---------------------------------------------------------------------------------------------

			// -- query available queue families with their associated indices. First query how many there are to allocate a buffer ---------------
			{
				vkGetPhysicalDeviceQueueFamilyProperties(phyDevices[i], &enumerateCounter, nullptr);
				if (enumerateCounter == 0u)
				{
					continue;
				}

				VectorCustom<VkQueueFamilyProperties> queueFamilyProperties(enumerateCounter);
				vkGetPhysicalDeviceQueueFamilyProperties(phyDevices[i], &enumerateCounter, queueFamilyProperties.data());
				for (uint32_t j = 0u; j < queueFamilyProperties.size(); ++j) // TODO refactor this to be more flexible
				{
					if (m_queueIdx.graphics == -1 
						&& 0 != queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
					{
						m_queueIdx.graphics = j;
					}
					
					// query for the given index presentation support for the given queue family
					if (m_queueIdx.presentation == -1)
					{
						VkBool32 presentationSupported;
						vkGetPhysicalDeviceSurfaceSupportKHR(phyDevices[i], /*queue family idx*/j, m_surface, &presentationSupported);
						if (presentationSupported)
						{
							m_queueIdx.presentation = j;
							break; // TODO move this break when adding more queue types
						}
					}
				}

				// now check if all necessary queues are present, if not continue with next device 
				if (bool allQueuesPresent = m_queueIdx.graphics != -1 && m_queueIdx.presentation != -1; 
					!allQueuesPresent)
				{
					continue;
				}
			} // vector killed

			// -- get and store the surface capabilities, an intersection between the display capabilities
			//    and the physical device capabilities regarding presentation. Needed to create a swapchain ----------------------------------------------
			// vkGetPhysicalDeviceSurfaceCapabilitiesKHR(phyDevices[i], m_surface, &m_surfaceCapabilities); moved to swapchain creation
			
			// -- now check for surface format support, we want R8G8B8A8 format, with sRGB nonlinear colorspace. ------------------------------------------
			// PS=there are more advanced query function to check for more specific features, 
			// such as specifics about compression support
			{
				vkGetPhysicalDeviceSurfaceFormatsKHR(phyDevices[i], m_surface, &enumerateCounter, nullptr);
				if (enumerateCounter == 0)
				{
					continue;
				}
	
				VectorCustom<VkSurfaceFormatKHR> surfaceFormats(enumerateCounter);
				vkGetPhysicalDeviceSurfaceFormatsKHR(phyDevices[i], m_surface, &enumerateCounter, surfaceFormats.data());
	
				uint32_t i = 0u;
				for (; i < surfaceFormats.size(); ++i)
				{
					// NOTE: format undefined, returned by vkGetPhysicalDeviceSurfaceFormatsKHR means all formats are supported under the associated color space
					if (surfaceFormats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
						&& (surfaceFormats[i].format == VK_FORMAT_UNDEFINED || surfaceFormats[i].format == VK_FORMAT_R8G8B8A8_UNORM || VK_FORMAT_B8G8R8A8_UNORM))
					{
						m_surfaceFormatUsed = surfaceFormats[i];
						break;
					}
				}
				if (i == surfaceFormats.size())
				{
					// forceful approach
					// continue;
				
					// sane approach
					m_surfaceFormatUsed = surfaceFormats[0];
				}
			} // vector killed

			// -- choose appropriate presentation mode for the swapchain ----------------------------------------------------------------------------
			// We want as present mode, ie how images in the swapchain are managed during image
			// swap, mailbox presentation mode, which means that 1) swapchain will keep the most 
			// recent image and throw away the older ones 2) images are swapped during the vertical 
			// blank interval only
			{
				vkGetPhysicalDeviceSurfacePresentModesKHR(phyDevices[i], m_surface, &enumerateCounter, nullptr);
				if (enumerateCounter == 0)
				{
					continue;
				}
	
				VectorCustom<VkPresentModeKHR> surfacePresentModes(enumerateCounter);
				vkGetPhysicalDeviceSurfacePresentModesKHR(phyDevices[i], m_surface, &enumerateCounter, surfacePresentModes.data());
				for (uint32_t i = 0u; i < surfacePresentModes.size(); ++i)
				{
					if (surfacePresentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR);
					{
						m_presentModeUsed = VK_PRESENT_MODE_MAILBOX_KHR;
						break;
					}
				}
				// if loop finishes and m_presentModeUsed is not set, its default value 
				// (set by the renderer class constructor) is VK_PRESENT_MODE_FIFO_KHR
			}
			// -- check requested device extension support -------------------------------------------------------------------------------------------------------
			{

				printf("about to check device extension support on the physical device\n");
				// then check against actual support of needed extensions for the device
				vkEnumerateDeviceExtensionProperties(phyDevices[i], nullptr, &enumerateCounter, nullptr);
				VectorCustom<VkExtensionProperties> supportedDeviceExtensions(enumerateCounter);
				vkEnumerateDeviceExtensionProperties(phyDevices[i], nullptr, &enumerateCounter, supportedDeviceExtensions.data());

				for (uint32_t i = 0u; i < desiredDeviceExtensions.size(); ++i)
				{
					uint32_t j = 0u;
					for (; j < supportedDeviceExtensions.size(); ++j)
					{
						printf("\tchecking for desired device extension %u, on device extension %u\n", i, j);
						if (0 == strncmp(supportedDeviceExtensions[j].extensionName, desiredDeviceExtensions[i], VK_MAX_EXTENSION_NAME_SIZE))
						{
							break;
						}
					}
					if (j == supportedDeviceExtensions.size())
					{
						fprintf(stderr, "couldn't find device extension %s\n", desiredDeviceExtensions[i]);
						return APP_GENERIC_ERR;
					}
				}
			}

			m_phyDevice = phyDevices[i];
			m_progressStatus |= PHY_DEVICE_GOT;
			printf("got physical device!\n");
			break;
		}

		return APP_SUCCESS;
	}
	
	template <template<class> class AllocTemplate> 
	auto Renderer<AllocTemplate>::setupDeviceAndQueues(std::span<char const*> const& deviceExtensions) & -> status_t
	{
		assert(m_progressStatus & PHY_DEVICE_GOT);
		// we can also map a device to multiple physical devices, a "device group", 
		// physical devices that can access each other's memory. I'don't care
		
		// TODO it can be refactored in a struct of arrays to be more readable
		// allocate necessary memory
		// -- for each queue family (of which we'll save 1 unique queue each), store index, priority and device queue create info -----------------------------------

		// "QUEUES" need to be setup BEFORE setting up the device, as queues are "CREATED ALONGSIDE THE DEVICE"
		// for each DISTICT queue we need to create its associated VkDeviceQueueCreateInfo, 
		// "BUT", a queue having presentation feature can be the same queue
		// we will be using for graphics operation. Therefore we need to create a set of queue indices
		// "IMPORTANT NOTE" TODO we might need again this set of queue indices. If that is the case, move this code
		VectorCustom<uint32_t> queueUniqueIndices; // TODO it can be optimized, i.e. unique indices can be built up during the creation of queueIdxArr back in setupPhysicalDevice
		{
			// TODO compare construction of unordered_set with unstable_sort+unique technique
			std::unordered_set<uint32_t> const queueUniqueIndicesSet(std::begin(m_queueIdxArr), std::end(m_queueIdxArr));
			queueUniqueIndices.resize(queueUniqueIndicesSet.size());
			std::copy(queueUniqueIndicesSet.cbegin(), queueUniqueIndicesSet.cend(), queueUniqueIndices.begin());
		}
		// each queue must have a "priority", between 0.0 and 1.0. the higher the priority the more
		// important the queue is, and such information MIGHT be used by the implementation to eg.
		// schedule the queues, give processing time to them. Vulkan makes NO GUARANTEES
		VectorCustom<float> queuePriorities(queueUniqueIndices.size(), 1.f);
		
		// -- create a VkDeviceQueueCreateInfo for each unique queue --------------------------------------------------------------------------------
		VectorCustom<VkDeviceQueueCreateInfo> deviceQueueCreateInfos(queueUniqueIndices.size(), {
			.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
			.pNext = nullptr, // TODO ?
			.flags = 0, // it has just the value VK_DEVICE_QUEUE_CREATE_PROTECTED_BIT, idk what is it, type is an enum called VkDeviceQueueCreateFlags
			.queueFamilyIndex = 0,
			.queueCount = 1, // every queue family supports at least 1 queue. For more than one, you check the Physical device queue family properties TODO
			.pQueuePriorities = nullptr
		});
	
		for (uint32_t i = 0u; i < deviceQueueCreateInfos.size(); ++i)
		{
			deviceQueueCreateInfos[i].queueFamilyIndex = queueUniqueIndices[i];
			deviceQueueCreateInfos[i].pQueuePriorities = queuePriorities.data(); 
		}

		// TODO this is to refactor and move to physical device selection code. Do it by passing supported extensions to the init function and then pass them here
		// -- Create device ------------------------------------------------------------------------------------------------------------------
		VkDeviceCreateInfo const deviceCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
			.pNext = nullptr, // TODO explore extensions in future
			.queueCreateInfoCount = static_cast<uint32_t>(deviceQueueCreateInfos.size()),
			.pQueueCreateInfos = deviceQueueCreateInfos.data(), 
			.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size()),
			.ppEnabledExtensionNames = deviceExtensions.data(),
			.pEnabledFeatures = nullptr // feature specification is OPTIONAL, VkPhysicalDeviceFeatures* TODO add---------------------------------------------------------- init params
		}; 

		if (vkCreateDevice(m_phyDevice, &deviceCreateInfo, /*VkAllocationCallbacks*/nullptr, &m_device) != VK_SUCCESS)
		{
			return APP_DEVICE_CREATION_ERR;
		}

		// TODO VK_EXT_device_memory_report, in 4.2 in vkspec
		
		// if anything goes wrong with the device when executing a command, it can become "lost", to query with "VK_ERROR_DEVICE_LOST", which 
		// is a return value of some operations using the device. When a device is lost, "its child objects (anything created/allocated whose
		// creation function has as first param the device), are not destroyed"
		
		m_progressStatus |= DEVICE_CREATED;
		printf("created device!\n");

		// -- store queue handles --------------------------------------------------------------------------------------------------------------
		for (uint32_t i = 0u; i < MXC_RENDERER_QUEUES_COUNT; ++i)
		{
			vkGetDeviceQueue(m_device, m_queueIdx.graphics, /*queue index within the family*/0u, &m_queues[i]);// TODO change when supporting more than 1 queue within a queue family
		}

		return APP_SUCCESS;
	}

	// TODO refactor
	template <template<class> class AllocTemplate> auto Renderer<AllocTemplate>::setupSurfaceKHR(GLFWwindow* window) & -> status_t
	{
		// setup surface
		if (VK_SUCCESS != glfwCreateWindowSurface(m_instance, window, /*VkAllocationCallbacks**/nullptr, &m_surface))
		{
			return APP_GENERIC_ERR;
		}

		m_progressStatus |= SURFACE_CREATED;

		return APP_SUCCESS;
	}

	// TODO for now will use composite alpha opaque
	template <template<class> class AllocTemplate> auto Renderer<AllocTemplate>::setupSwapchain(uint32_t width, uint32_t height) & -> status_t
	{
		// -- get and store the surface capabilities, an intersection between the display capabilities
		//    and the physical device capabilities regarding presentation. Needed to create a swapchain ----------------------------------------------
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_phyDevice, m_surface, &m_surfaceCapabilities);
			
		// -- create swapchain ----------------------------------------------------------------------------------------------------------------
		// TODO remove hardcoded window width and height when implementing dynamic viewport
		// m_surfaceCapabilities.currentExtent, needed for swapchain creation, can contain either 
		// the current width and height of the surface or the special value {0xfffffffff,0xffffffff), indicating
		// that I can choose image size, which in such case will be the window size
		m_surfaceExtent = {
			.width = m_surfaceCapabilities.currentExtent.width != 0xffffffff ? m_surfaceCapabilities.currentExtent.width : width,
			.height= m_surfaceCapabilities.currentExtent.height != 0xffffffff ? m_surfaceCapabilities.currentExtent.height : height
		};

		// if the graphics and presentation queue (TODO might change as these change) belong to the 
		// same queue family, or not. the swapchain needs to know about how many and which queue families it will
		// deal with. TODO to refactor out when structure of family queue indices will change. in particular, if and when we decide to use more than one queue per family this has to change
		uint32_t const queueFamilyCnt = 1u + (m_queueIdx.graphics == m_queueIdx.presentation);
		uint32_t const queueFamilyIndices[2] = {static_cast<uint32_t>(m_queueIdx.graphics), static_cast<uint32_t>(m_queueIdx.presentation)};
		
		
		// swapchain requires we specify if the image is going to be accessed by one queue at a time (sharing mode = exclusive) or by more than one (sharing mode = concurrent).
		// this depends on whether or not the presentation queue and the graphics queue are one and the same.
		VkSharingMode imageSharingMode = queueFamilyCnt == 1 ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT;

		VkSwapchainCreateInfoKHR const swapchainCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
			.pNext = nullptr,
			.flags = 0u, 
			.surface = m_surface,
			.minImageCount = m_surfaceCapabilities.minImageCount + 1, // increment by one to allow for TRIBLE BUFFERING
			.imageFormat = m_surfaceFormatUsed.format,
			.imageColorSpace = m_surfaceFormatUsed.colorSpace,
			.imageExtent = m_surfaceExtent,
			.imageArrayLayers = 1, // number of views in a multiview surface. >1 only for stereoscopic apps, like VR
			.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, // bitfield expressing in how many ways an image will be used. TODO for now only colour
			.imageSharingMode = imageSharingMode, // either EXCLUSIVE or CONCURRENT, first means that an image will be exclusive to family queues, other concurrent, we have only one graphics family queue for now TODO
			.queueFamilyIndexCount = queueFamilyCnt,
			.pQueueFamilyIndices = queueFamilyIndices,
			.preTransform = m_surfaceCapabilities.currentTransform, // transform applied before presentation to image. will use current transform given by surface, which means do nothing
			.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR, // type VkCompositeAlphaFlagBitsKHR, indicates which alpha compositing mode to use after fragment shader
			.presentMode = m_presentModeUsed,
			.clipped = VK_FALSE, // specifies whether to not execute fragment shader on not visible pixels due to other windows on top of the app or window partially out of display bounds (if set to VK_TRUE). NOTE TODO DO NOT SET IT TO TRUE IF YOU WILL FURTHER PROCESS THE CONTENT OF THE FRAMEBUFFER
			.oldSwapchain = m_swapchain // special Vulkan object handle value specifying an invalid object
		};

		VkSwapchainKHR swapchain;
		VkResult res = vkCreateSwapchainKHR(m_device, &swapchainCreateInfo, /*vkAllocationCallbacks**/nullptr, &swapchain);
		if (res != VK_SUCCESS)
		{
			return APP_SWAPCHAIN_CREATION_ERR;
		}

		vkDestroySwapchainKHR(m_device, m_swapchain, /*VkAllocationCallbacks**/nullptr);
		m_swapchain = swapchain;
		m_progressStatus |= SWAPCHAIN_CREATED;
		printf("swapchain created!\n");

		// -- get images from swapchain and create associated image views --------------------------------------------------------------------
		{
			uint32_t swapchainImagesCnt;
			vkGetSwapchainImagesKHR(m_device, m_swapchain, &swapchainImagesCnt, nullptr);
			assert(swapchainImagesCnt);
			m_swapchainImages.resize(swapchainImagesCnt);
			vkGetSwapchainImagesKHR(m_device, m_swapchain, &swapchainImagesCnt, m_swapchainImages.data());
		}

		// TODO refactor image view creation
		m_swapchainImageViews.resize(m_swapchainImages.size());
		for (uint32_t i = 0u; i < m_swapchainImageViews.size(); ++i)
		{
			VkImageViewCreateInfo const image_view_create_info = {
				.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
				.pNext = nullptr, // TODO what
				.flags = 0,
				.image = m_swapchainImages[i],
				.viewType = VK_IMAGE_VIEW_TYPE_2D,  // dimensionality and number of image
				.format = m_surfaceFormatUsed.format,
				.components = VkComponentMapping{
					.r = VK_COMPONENT_SWIZZLE_IDENTITY, // type VkComponentSwizzle, you can remap a color channel into another or force one value
					.g = VK_COMPONENT_SWIZZLE_IDENTITY,
					.b = VK_COMPONENT_SWIZZLE_IDENTITY,
					.a = VK_COMPONENT_SWIZZLE_IDENTITY
				},
				.subresourceRange = { // VkImageSubresourceRange selects mip levels and array elements to use
					// which aspects of the image (color, depth, stencil, ...) are included in view
					.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT | VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT,
					.baseMipLevel = 0u,
					.levelCount = 1, // in the image created by swapchain there is 1 mipmap level
					.baseArrayLayer = 0u, // think of photoshop layers?
					.layerCount = 1u // in the swapchain image there are as many array layers as specified in the swapchain create info
				}
			};

			res = vkCreateImageView(m_device, &image_view_create_info, /*VkAllocationCallbacks**/nullptr, &m_swapchainImageViews[i]);
			if (res != VK_SUCCESS)
			{
				return APP_GENERIC_ERR;
			}
		}
		
		m_progressStatus |= SWAPCHAIN_IMAGE_VIEWS_CREATED;
		return APP_SUCCESS;
	}

	template <template<class> class AllocTemplate> auto Renderer<AllocTemplate>::setupCommandBuffers() & -> status_t
	{
		assert(m_progressStatus & DEVICE_CREATED && "command pools are child objects of devices, hence we need a device\n");

		// -- create command buffer pool -------------------------------------------------------------------------------------------
		VkCommandPoolCreateInfo const cmdPoolCreateInfo{
			.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
			.pNext = nullptr,
			.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT
				   | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT, // VkCommandPoolCreateFlags, there are 3: 1)transient(for driver optimizations) i.e. short lived(for driver optimizations)  2)resettable 3)protected(uses protected memory, https://registry.khronos.org/vulkan/specs/1.2-extensions/html/chap11.html#memory-protected-memory)
			.queueFamilyIndex = static_cast<uint32_t>(m_queueIdx.graphics), // if DEVICE_CREATED was set, then this is safe
		};

		VkResult res = vkCreateCommandPool(m_device, &cmdPoolCreateInfo, /*VkAllocationCallbacks**/nullptr, &m_graphicsCmdPool);
		if (res != VK_SUCCESS)
		{
			fprintf(stderr, "failed to create a command pool for the graphics queue!\n");
			return APP_MEMORY_ERR;
		}
	
		// -- allocate command buffers (if fails destroy command pool). There will be 1 command buffer for each framebuffer ------------------------
		VkCommandBufferAllocateInfo const graphicsCmdBufAllocInfo{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.pNext = nullptr,
			.commandPool = m_graphicsCmdPool,
			.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY, // VkCommandBufferLevel, can either be PRIMARY=surface level commands, executed by queues or SECONDARY = can be called by primary/secondary command buffers, a bit like functions
			.commandBufferCount = static_cast<uint32_t>(m_swapchainImages.size()) // cmdbuf count = framebuffer count = swapchain images count = semaphores count = fences count
		};

		m_graphicsCmdBufs.resize(m_swapchainImages.size());

		res = vkAllocateCommandBuffers(m_device, &graphicsCmdBufAllocInfo, m_graphicsCmdBufs.data());
		if (res != VK_SUCCESS)
		{
			vkDestroyCommandPool(m_device, m_graphicsCmdPool, /*VkAllocationCallbacks**/nullptr);
			fprintf(stderr, "failed to allocate a command buffer to the graphics command pool!\n");
			return APP_VK_ALLOCATION_ERR;
		}

		m_progressStatus |= COMMAND_BUFFER_ALLOCATED;
		printf("created command pool and allocated a resettable and transient command pool\n");
		return APP_SUCCESS;
	}

	template <template<class> class AllocTemplate> auto Renderer<AllocTemplate>::setupDepthImage() & -> status_t
	{
		assert(m_progressStatus & DEVICE_CREATED);

		uint32_t graphicsIdxsUnsigned[MXC_RENDERER_GRAPHICS_QUEUES_COUNT] = {static_cast<uint32_t>(m_queueIdx.graphics)}; // TODO change, not futureproof

		// create depth image
		VkImageCreateInfo const depthImgCreateInfo {
			.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,// specifies additional properties of the image. BE AWARE THAT MANY THINGS, such as 2darray, cubemap, sparce memory, multisampling, ... require a flag
			.imageType = VK_IMAGE_TYPE_2D,// number of dimensions
			.format = m_depthImageFormat,
			.extent = VkExtent3D{
					.width = m_surfaceCapabilities.currentExtent.width != 0xffffffff ? m_surfaceCapabilities.currentExtent.width : WINDOW_WIDTH,
					.height = m_surfaceCapabilities.currentExtent.height != 0xffffffff ? m_surfaceCapabilities.currentExtent.height : WINDOW_HEIGHT,
					.depth = 0
			},
			.mipLevels = 1, // numbers of levels of detail 
			.arrayLayers = 1, // numbers of layers in the image (Photoshop sense)
			.samples = VK_SAMPLE_COUNT_1_BIT,//VkSampleCountFlagBits
			.tiling = VK_IMAGE_TILING_OPTIMAL, // how image is laid out in memory, between optimal, linear, drm(requires extension, linux only)
			.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
			.sharingMode = VK_SHARING_MODE_EXCLUSIVE, // either exclusive or concurrent
			.queueFamilyIndexCount = MXC_RENDERER_GRAPHICS_QUEUES_COUNT,
			.pQueueFamilyIndices = graphicsIdxsUnsigned, // assuming device and queues have been setup
			.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
		};

		VkResult res = vkCreateImage(m_device, &depthImgCreateInfo, /*VkAllocationCallbacks**/nullptr, &m_depthImage);
		if (res != VK_SUCCESS)
		{
			fprintf(stderr, "failed to create depth image!\n");
			return APP_GENERIC_ERR;
		}

		// create depth image view
		// they are non dispatchable handles NOT accessed by shaders but represent a range in an image with associated metadata
		// special values useful for creation = VK_REMAINING_ARRAY_LAYERS, VK_REMAINING_MIP_LEVELS
		VkImageViewCreateInfo const depthImgViewCreateInfo {
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,// there is one to specify that view will be read during fragment density stage? and one for reading the view in CPU at the end of the command buffer execution
			.image = m_depthImage,
			.viewType = VK_IMAGE_VIEW_TYPE_2D,// dimentionality and type of view, must be "less then or equal" to image
			.format = m_depthImageFormat,
			.components = VkComponentMapping{// VkComponentMapping, all swizzle identities
				.r = VK_COMPONENT_SWIZZLE_IDENTITY,
				.g = VK_COMPONENT_SWIZZLE_IDENTITY,
				.b = VK_COMPONENT_SWIZZLE_IDENTITY,
				.a = VK_COMPONENT_SWIZZLE_IDENTITY
			},
			.subresourceRange = VkImageSubresourceRange{// specifies subrange accessible from view
				.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
				.baseMipLevel = 0,
				.levelCount = VK_REMAINING_MIP_LEVELS,
				.baseArrayLayer = 0,
				.layerCount = VK_REMAINING_ARRAY_LAYERS
			}
		};

		res = vkCreateImageView(m_device, &depthImgViewCreateInfo, /*VkAllocationCallbacks**/nullptr, &m_depthImageView);
		if (res != VK_SUCCESS)
		{
			vkDestroyImage(m_device, m_depthImage, /*VkAllocationCallbacks**/nullptr);
			fprintf(stderr, "failed to create depth image view!\n");
			return APP_GENERIC_ERR;
		}

		m_progressStatus |= DEPTH_IMAGE_CREATED;
		return APP_SUCCESS;
	}

	template <template<class> class AllocTemplate> auto Renderer<AllocTemplate>::checkMemoryRequirements(VkMemoryRequirements const& memoryRequirements, VkMemoryPropertyFlagBits const& requestedMemoryProperties, uint32_t* outMemoryTypeIndex) & -> status_t
	{		
		// TODO refactor so that this is done once
		VkPhysicalDeviceMemoryProperties deviceMemoryProperties; // typecount, types, heapcount, heaps
		vkGetPhysicalDeviceMemoryProperties(m_phyDevice, &deviceMemoryProperties);

		for (uint32_t i = 0; i < deviceMemoryProperties.memoryTypeCount; ++i)
		{
			// VkMemoryRequirements::memoryTypeBits is a bitfield (uint32) whose bits are set to one if the corresponding memory type index(==power of 2) can be used by the resource
			// so we will: 1) check if current memory type index can be used by resource
			// 			   2) check if the memory type's properties are suitable to OUR needs
			if ( (memoryRequirements.memoryTypeBits & (1 << i)) // check if i-th bit is a type usable by resource
			 	 && (deviceMemoryProperties.memoryTypes[i].propertyFlags & requestedMemoryProperties) == requestedMemoryProperties) // check if type's properties match our needs
			{
				*outMemoryTypeIndex = i;
				printf("----------------------->>>>returning memory type index %u\n", *outMemoryTypeIndex);
				return APP_SUCCESS;
			}
		}

		return APP_GENERIC_ERR;
	}

	template <template<class> class AllocTemplate> auto Renderer<AllocTemplate>::setupDepthDeviceMemory() & -> status_t
	{
		assert((m_progressStatus & (DEVICE_CREATED | DEPTH_IMAGE_CREATED)) && "VkDevice required to allocate VkDeviceMemory!\n");

		// get image memory requirements for depth image
		VkMemoryRequirements depthImageMemoryRequirements; // size, alignment, memory type bits
		vkGetImageMemoryRequirements(m_device, m_depthImage, &depthImageMemoryRequirements);

		// allocate memory
		// -- for each memory type present in the device, find one that matches our memory requirement
		uint32_t memoryTypeIndex;
		if (checkMemoryRequirements(depthImageMemoryRequirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &memoryTypeIndex) != APP_SUCCESS)
		{
			fprintf(stderr, "could not find any suitable memory types to allocate memory for depth image!\n");
			return APP_GENERIC_ERR;
		}

		VkMemoryAllocateInfo const allocateInfo {
			.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
			.pNext = nullptr, // There are many extentions, most of them even platform-specific
			.allocationSize = depthImageMemoryRequirements.size,
			.memoryTypeIndex = memoryTypeIndex // type of memory required, must be supported by device
		};
		VkResult const res = vkAllocateMemory(m_device, &allocateInfo, /*VkAllocationCallbacks**/nullptr, &m_depthImageMemory);

		// bind allocated memory to depth image
		vkBindImageMemory(m_device, m_depthImage, m_depthImageMemory, 0); // last parameter is offset TODO this means that in one memory heap we can allocate more than one image!

		m_progressStatus |= DEPTH_MEMORY_ALLOCATED;
		printf("allocated device memory for depth buffer!\n");
		return APP_SUCCESS;
	}

	template <template<class> class AllocTemplate> auto Renderer<AllocTemplate>::setupRenderPass() & -> status_t
	{
		// -- create output attachment and references ------------------------------------------------------------------------------------
		// render pass output attachment descriptions array (for the render pass, we will also need attachment references for the subpasses, which are handles decorated with some data to this array) VkAttachmentDescription const outAttachmentDescriptions[MXC_RENDERER_ATTACHMENT_COUNT] {
		VkAttachmentDescription const outAttachmentDescriptions[] {
			// color attachment
			{
				.flags = 0,// only flag is VK_ATTACHMENT_DESCRIPTION_MAY_ALIAS_BIT, which specifies that all attachments use the same device memory. I am not dealing with memory now
				.format = m_surfaceFormatUsed.format,
				.samples = VK_SAMPLE_COUNT_1_BIT, // VkSampleCountFlagBits, we are not using multisampling
				.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR, // how color and depth of the attachment will be modified at the beginning of the first subpass. Clear color specified when recording commands
				.storeOp = VK_ATTACHMENT_STORE_OP_STORE, // how color and depth components of the attachments will be treated at the end of the last subpass. We want to store them to present them
				.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE, // how stencil components of the attachments will be treated at the beginning of the first subpass. I am not using a stencil buffer now TODO
				.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
				// CANNOT BE UNDEFINED IF LOADOP IS LOAD
				.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,//(VkImageLayout) layout of the attachment image subresource EXPECTED when the render pass instance begins(the first subpass). RENDER PASS IS A BLUEPRINT FOR A RENDERING PROCESS, CALLED RENDER PASS INSTANCE
				.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR// layout of the attachment image subresource will be TRANSITIONED TO when the render pass instance ends(the last subpass)
			},
			// depth attachment
			{
				.flags = 0,
			    // VK_FORMAT_D32_SFLOAT: 32-bit float for depth
			    // VK_FORMAT_D32_SFLOAT_S8_UINT: 32-bit signed float for depth and 8 bit stencil component
			    // VK_FORMAT_D24_UNORM_S8_UINT: 24-bit float for depth and 8 bit stencil component
				.format = m_depthImageFormat, // no need to check for support of such a format, as the 3 of them are MANDATORY BY THE VULKAN SPECIFICATION. see setupPhysical device and TODO refactor
				.samples = VK_SAMPLE_COUNT_1_BIT,
				.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR, //(VkAttachmentLoadOp)
				.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
				.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
				.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
				.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
			}
		};

		// output attachment references
		VkAttachmentReference const outAttachmentRefs[MXC_RENDERER_ATTACHMENT_COUNT] {
			// color attachment ref
			{
				.attachment = 0, // index of the attachment used as stored in the array of attachment descriptions
				.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL// VkImageLayout, specifies the layout of the attachment used during the subpass
			},
			// depth attachment ref
			{
				.attachment = 1,
				.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
			}
		}; 

		// subpasses creation
		VkSubpassDescription const subpassDescriptions[] {
			{
				.flags = 0,// specifies usage of subpass, if it matches one of the flags. They all require extensions
				.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,// pipeline type supported by the subpass (you could use same subpass descriptions to create multiple pipelines)
				.inputAttachmentCount = 0, // TODO need descriptor sets. They match the input of the vertex shader written with layout(binding = n, location = 0) in ...
				.pInputAttachments = nullptr, // type VkAttachmentReference*
				.colorAttachmentCount = 1, // same count as resolve attachment count
				.pColorAttachments = outAttachmentRefs, // these are outputs of the fragment shader, the layout(location = 0) out ... 
				.pResolveAttachments = nullptr, // resolve attachments are attachments specifying in a multisampling scenario multiple samples per pixels are converted to one sample per pixel, requires VK_SUBPASS_DESCRIPTION_SHADER_RESOLVE_BIT_QCOM
				.pDepthStencilAttachment = nullptr, //&outAttachmentRefs[1], // ALWAYS ONE IN EACH SUBPASS
				.preserveAttachmentCount = 0, // preserve attachments are all the attachments of the render pass not used in this subpass
				.pPreserveAttachments = nullptr
			}
		};

		// -- subpass dependencies creation ---------------------------------------------------------------------------------------
		// last subpass dependency, since it's from initial to final layout, it's implicit but we specify it anyway. the first is needed
		// subpasses may execute out of order among each other, and this is a problem since the image during rendering will be manipulated in different intermediate gpu-specific formats depending on which stage of the
		// render pass an image currently is. Therefore we require that between an image forma transition to the other the n-th subpass has finished executing in all invocations before starting with the other.
		VkSubpassDependency subpassDependencies[] {
			// color attachment, from VK_IMAGE_LAYOUT_UNDEFINED (coming from the swapchain) to VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL (going to the first subpass)
			{
				.srcSubpass = VK_SUBPASS_EXTERNAL, // it means outside of our subpasses
				.dstSubpass = 0, // this is the index of the subpass
				.srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, // last stage that has to happen STRICTLY before the layout transition happens. bottom of pipe means the last stage of the pipeline. it means the end of the subpass external stuff
				.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, // stage that can start only AFTER the transition has been completed
				.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT, // memory operation within the source stage that has to be completed before the layout transition can begin. we first need to read the color data coming from the swapchain, then we can convert it. this is equivalent of bitwise or'ing every read access flag
				.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
							   //| VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, // conversion has to happen before we attempt to read/write to the color output attachment
				.dependencyFlags = 0
				// each access mask can occur in sone particular stages (table on vkspec). So access mask can be thought of as "substages", consisting of types of memory accesses
			},
			// color attachment, from VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL (1st subpass) to VK_IMAGE_LAYOUT_PRESENT_SRC_KHR (SUBPASS_EXTERNAL)
			{
				.srcSubpass = 0,
				.dstSubpass = VK_SUBPASS_EXTERNAL,
				.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, // first we need to finish to output to the color attachment in the framebuffer
				.dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
				.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
							   //| VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, 
				.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT,
				.dependencyFlags = 0
			}
		};

		// -- render pass creation --------------------------------------------------------------------------------------------------------------
		VkRenderPassCreateInfo const renderPassCreateInfo{
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,// there is only one flag(type VkRenderPassCreateFlagBits), which enables the use of render pass transforms, ie transforms applied to all primitives in each subpass. VK_RENDER_PASS_CREATE_TRANSFORM_BIT_QCOM
			.attachmentCount = MXC_RENDERER_ATTACHMENT_COUNT,
			.pAttachments = outAttachmentDescriptions,
			.subpassCount = 1, // TODO for now we do not use subpasses
			.pSubpasses = subpassDescriptions,
			.dependencyCount = sizeof(subpassDependencies)/sizeof(VkSubpassDependency), // TODO 
			.pDependencies = subpassDependencies//TODO
		};
		
		printf("we created %u subpass dependencies!\n", sizeof(subpassDependencies)/sizeof(VkSubpassDependency));

		VkResult const res = vkCreateRenderPass(m_device, &renderPassCreateInfo, /*VkAllocationCallbacks**/nullptr, &m_renderPass);
		if (res != VK_SUCCESS)
		{
			fprintf(stderr, "failed to create renderpass!\n");
			return APP_GENERIC_ERR;
		}

		printf("created renderpass!\n");
		m_progressStatus |= RENDERPASS_CREATED;
		return APP_SUCCESS;
	}

	template <template<class> class AllocTemplate> auto Renderer<AllocTemplate>::setupFramebuffers() & -> status_t
	{
		assert(m_progressStatus & RENDERPASS_CREATED);

		// -- create Framebuffers -----------------------------------------------------------------------------------------------
		m_framebuffers.resize(m_swapchainImages.size());

		// The reason you need multiple presentable images is because one (or more) of them is in the process of being presented. 
		// During that time (which could take a while), you still want the GPU to be doing stuff. Since it can’t “do stuff” to the image that’s 
		// being presented, it would have to “do stuff” to some other image. But you’re not presenting your depth buffer, are you? 
		// None of the above applies.the contents of the depth buffer are its own. Its contents are generated by on-GPU processes: the renderpass 
		// load/clear operation, subpasses that use it as a depth attachment, etc. Once you’re finished generating an image with the depth buffer, 
		// you don’t need it anymore. So that buffer can be immediately reused by another rendering process. the only potential synchronization issue 
		// is that you have to prevent the next frame’s rendering commands from starting until the current frame’s commands have finished with the 
		// depth buffer. And since you usually have plenty of other reasons for imposing synchronization between frames, 
		// that synchronization should be sufficient.
		VkImageView usedAttachments[MXC_RENDERER_ATTACHMENT_COUNT] { m_swapchainImageViews[0], m_depthImageView}; // we have 2 attachments per swapchain image, 1st is color, 2nd depth. Depth doesn't change

		VkFramebufferCreateInfo framebufferCreateInfo {
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.pNext = nullptr, // TODO look into that
			.flags = 0, // there is, as of now, 1 flag only, to create an "imageless" framebuffer
			.renderPass = m_renderPass,
			.attachmentCount = MXC_RENDERER_ATTACHMENT_COUNT,
			.pAttachments = usedAttachments, // as it's a pointer, all I have to do to create 3 different framebuffers is to change the color output attachment in the loop
			.width = m_surfaceExtent.width,
			.height = m_surfaceExtent.height,
			.layers = 1 // TODO remove hardcoded number
		};

		VkResult res;
		for (uint32_t i = 0; i < m_framebuffers.size(); ++i)
		{
			res = vkCreateFramebuffer(m_device, &framebufferCreateInfo, /*VkAllocationCallbacks**/nullptr, &m_framebuffers[i]);
			if (res != VK_SUCCESS)
			{
				for (uint32_t j = 0; j < i; ++j)
				{
					vkDestroyFramebuffer(m_device, m_framebuffers[j], /*VkAllocationCallbacks**/nullptr);
				}

				fprintf(stderr, "failed to create framebuffers!\n");
				return APP_GENERIC_ERR;
			}

			usedAttachments[0] = m_swapchainImageViews[i+1]; // will go out of bounds in last iteration, but won't be read
		}

		printf("%u framebuffers created!\n", m_framebuffers.size());
		m_progressStatus |= FRAMEBUFFERS_CREATED;
		return APP_SUCCESS;
	}

	template <template<class> class AllocTemplate> auto Renderer<AllocTemplate>::setupVertexInput(std::span<Vertex> vertexInput, std::span<uint32_t> indexInput) & -> status_t
	{
		assert(m_progressStatus & DEVICE_CREATED);
		
		// -- create a staging, index, vertex buffers -----------------------------------------------------------------------------
		VkBufferCreateInfo const bufferCreateInfo[] {
			{ // staging buffer
				.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
				.pNext = nullptr, 
				.flags = 0,
				.size = static_cast<VkDeviceSize>(vertexInput.size() * sizeof(Vertex) + indexInput.size() * sizeof(uint32_t)),
				.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
				.queueFamilyIndexCount = 1,
				.pQueueFamilyIndices = nullptr
			},
			{ // vertex buffer
				.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
				.pNext = nullptr,
				.flags = 0,
				.size = static_cast<VkDeviceSize>(vertexInput.size() * sizeof(Vertex)),
				.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
				.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
				.queueFamilyIndexCount = 1,
				.pQueueFamilyIndices = nullptr// queueFamilyIndices WRONG! even though we have more than one queue, presentation is not involved
			},
			{ // index buffer
				.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
				.pNext = nullptr,
				.flags = 0,
				.size = static_cast<VkDeviceSize>(indexInput.size() * sizeof(uint32_t)),
				.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
				.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
				.queueFamilyIndexCount = 1,
				.pQueueFamilyIndices = nullptr
			}
		};
		if (vkCreateBuffer(m_device, &bufferCreateInfo[0], /*VkAllocationCallbacks**/nullptr, &m_stagingBuffer) != VK_SUCCESS
			|| vkCreateBuffer(m_device, &bufferCreateInfo[1], /*VkAllocationCallbacks**/nullptr, &m_vertexBuffer) != VK_SUCCESS
			|| vkCreateBuffer(m_device, &bufferCreateInfo[2], /*VKAllocationCallbacks**/nullptr, &m_indexBuffer) != VK_SUCCESS)
		{
			fprintf(stderr, "failed to create staging, vertex or index buffer!\n");
			m_progressStatus &= ~VERTEX_INPUT_BOUND;
			return APP_GENERIC_ERR;
		}
		
		// TODO refactor to use DEVICE_LOCAL memory, and to use vkFlushMappedMemoryRanges
		// -- allocate memory for the buffers --------------------------------------------------------------
		// now we are getting an intersection between memory requirements of the vertex buffer and of the index buffer,
		// because we are going to use the same device memory
		VkMemoryRequirements bufferMemoryRequirements[2]; // TODO refactor

		// device local memory requirements are an intersection of those of the vertex buffer and of the index buffer
		vkGetBufferMemoryRequirements(m_device, m_vertexBuffer, &bufferMemoryRequirements[0]);
		vkGetBufferMemoryRequirements(m_device, m_indexBuffer, &bufferMemoryRequirements[1]);
		size_t const vertexBufferSize = bufferMemoryRequirements[0].size;
		size_t const indexBufferSize = bufferMemoryRequirements[1].size;

		bufferMemoryRequirements[0].size += bufferMemoryRequirements[1].size;
		bufferMemoryRequirements[0].alignment = std::max(bufferMemoryRequirements[0].alignment, bufferMemoryRequirements[1].alignment); 
		bufferMemoryRequirements[0].memoryTypeBits &= bufferMemoryRequirements[1].memoryTypeBits;

		// host visible memory requirements for staging buffer
		vkGetBufferMemoryRequirements(m_device, m_stagingBuffer, &bufferMemoryRequirements[1]);

		uint32_t memoryTypeIndices[2];
		if (checkMemoryRequirements(bufferMemoryRequirements[0], static_cast<VkMemoryPropertyFlagBits>(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT), &memoryTypeIndices[0]) != APP_SUCCESS
			|| checkMemoryRequirements(bufferMemoryRequirements[1], static_cast<VkMemoryPropertyFlagBits>(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT), &memoryTypeIndices[1]) != APP_SUCCESS)
		{
			fprintf(stderr, "couldn't find any suitable memory type for staging, index or vertex buffer!\n");
			m_progressStatus &= ~VERTEX_INPUT_BOUND;
			return APP_GENERIC_ERR;
		}

		VkMemoryAllocateInfo const memoryAllocateInfos[2] {
			{ // device local vertex and index buffers
				.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
				.pNext = nullptr,
				.allocationSize = bufferMemoryRequirements[0].size,
				.memoryTypeIndex = memoryTypeIndices[0]
			},
			{ // host visible staging buffer
				.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
				.pNext = nullptr,
				.allocationSize = bufferMemoryRequirements[1].size,
				.memoryTypeIndex = memoryTypeIndices[1]
			}
		};

		if (vkAllocateMemory(m_device, &memoryAllocateInfos[0], /*VkAllocationCallbacks**/nullptr, &m_inputBuffersMemory) != VK_SUCCESS
			|| vkAllocateMemory(m_device, &memoryAllocateInfos[1], /*VkAllocationCallbacks*/nullptr, &m_stagingBufferMemory) != VK_SUCCESS)
		{
			fprintf(stderr, "failed to allocate device memory for vertex buffer!\n");
			m_progressStatus &= ~VERTEX_INPUT_BOUND;
			return APP_GENERIC_ERR;
		}

		// -- map memory and copy to staging buffer the data ------------------------------------------------------
		void* mmappedPtr;
		VkResult res = vkMapMemory(
			m_device,
			m_stagingBufferMemory,
			0, // offset
			VK_WHOLE_SIZE, // size
			0, // flags
			&mmappedPtr);
		if (res != VK_SUCCESS)
		{
			fprintf(stderr, "failed to map memory!\n");
			if (res == VK_ERROR_MEMORY_MAP_FAILED)
			{
				printf("VK_ERROR_MEMORY_MAP_FAILED ----------------------\n");
				return APP_GENERIC_ERR;
			}
		}

		memcpy(mmappedPtr, vertexInput.data(), vertexInput.size() * sizeof(Vertex));
		memcpy(reinterpret_cast<unsigned char*>(mmappedPtr)+vertexBufferSize, indexInput.data(), indexInput.size()*sizeof(uint32_t));

		VkMappedMemoryRange const memoryRange {
			.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
			.pNext = nullptr,
			.memory = m_stagingBufferMemory,
			.offset = 0,
			.size = VK_WHOLE_SIZE
		};
		// vkFlushMappedMemoryRanges(m_device, /*memory range count*/1, &memoryRange);

		vkUnmapMemory(m_device, m_stagingBufferMemory);
		
		// -- bind buffer to memory (you can do it before or after memory mapping) -----------------------------------------------
		if (vkBindBufferMemory(m_device, m_stagingBuffer, m_stagingBufferMemory, /*offset*/0) != VK_SUCCESS 
			|| vkBindBufferMemory(m_device, m_vertexBuffer, m_inputBuffersMemory, /*offset*/0) != VK_SUCCESS
			|| vkBindBufferMemory(m_device, m_indexBuffer, m_inputBuffersMemory, /*offset*/vertexBufferSize) != VK_SUCCESS)
		{
			fprintf(stderr, "failed to bind memory for vertex or index buffer!\n");
			m_progressStatus &= ~VERTEX_INPUT_BOUND;
			return APP_GENERIC_ERR;
		}

		// -- record and submit copy operation in a local command buffer ---------------------------------------------------------
		VkCommandBuffer copyCommandBuffer;
		VkCommandBufferAllocateInfo const copyCommandBufferCreateInfo {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.pNext = nullptr,
			.commandPool = m_graphicsCmdPool,
			.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = 1 // ?
		};
		res = vkAllocateCommandBuffers(m_device, &copyCommandBufferCreateInfo, &copyCommandBuffer);
		
		VkCommandBufferBeginInfo const beginInfo {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.pNext = nullptr,
			.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
			.pInheritanceInfo = nullptr // only for secondary cmd bufs
		};
		res = vkBeginCommandBuffer(copyCommandBuffer, &beginInfo);
		if (res != VK_SUCCESS)
		{
			fprintf(stderr, "failed to begin recording of copy command!\n");
			m_progressStatus &= ~VERTEX_INPUT_BOUND;
			return APP_GENERIC_ERR;
		}
		
		VkBufferCopy regions[2] {
			{ // vertex buffer
				.srcOffset = 0,
				.dstOffset = 0,
				.size = vertexBufferSize
			},
			{ // index buffer
				.srcOffset = vertexBufferSize,
				.dstOffset = 0, // the offset given here is added to the offset given to the memory
				.size = indexBufferSize
			}
		};
		vkCmdCopyBuffer(copyCommandBuffer, m_stagingBuffer, m_vertexBuffer, /*regionCount*/1, &regions[0]);
		vkCmdCopyBuffer(copyCommandBuffer, m_stagingBuffer, m_indexBuffer, /*regionCount*/1, &regions[1]);

		res = vkEndCommandBuffer(copyCommandBuffer);
		if (res != VK_SUCCESS)
		{
			fprintf(stderr, "failed to end copy command buffer!\n");
			m_progressStatus &= ~VERTEX_INPUT_BOUND;
			return APP_GENERIC_ERR;
		}

		// TODO setup proper synchronization. Now we will vkQueueIdle
		VkSubmitInfo const submitInfo {
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.pNext = nullptr,
			.waitSemaphoreCount = 0,
			.pWaitSemaphores = nullptr,
			.pWaitDstStageMask = nullptr,
			.commandBufferCount = 1,
			.pCommandBuffers = &copyCommandBuffer,
			.signalSemaphoreCount = 0,
			.pSignalSemaphores = nullptr
		};

		uint32_t queue = static_cast<uint32_t>(m_queueIdx.graphics);
		vkQueueSubmit(m_queues[0], /*submit count*/1, &submitInfo, /*fence to signal when finished*/VK_NULL_HANDLE);
		vkQueueWaitIdle(m_queues[0]); // TODO remove

		vkFreeCommandBuffers(m_device, m_graphicsCmdPool, copyCommandBufferCreateInfo.commandBufferCount, &copyCommandBuffer);
		
		printf("vertex input set up!\n");
		m_progressStatus |= VERTEX_INPUT_BOUND;
		return APP_SUCCESS;
	}

	template <template<class> class AllocTemplate> auto Renderer<AllocTemplate>::setupGraphicsPipeline() & -> status_t
	{
		assert(m_progressStatus & (FRAMEBUFFERS_CREATED | RENDERPASS_CREATED) && "graphics pipeline creation requires a renderpass and framebuffers!\n");

		// creation of all information about pipeline steps: layout, and then in order of execution
		// -- pipeline layout TODO update when adding descriptor sets --------------------------------------------------------------------------------------------------------------
		VkPipelineLayoutCreateInfo const graphicsPipelineLayoutCI {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0, // there is only 1 and requires an extension
			.setLayoutCount = 0, // TODO change, number of descriptor sets to include in the pipeline layout
			.pSetLayouts = nullptr, // TODO change, pointer to array of VkDescriptorSetLayout objects
			.pushConstantRangeCount = 0, // TODO change, number to push constant ranges (range = a part of push constant)
			.pPushConstantRanges = nullptr // TODO change
		};

		VkResult res = vkCreatePipelineLayout(m_device, &graphicsPipelineLayoutCI, /*VkAllocationCallbacks**/nullptr, &m_graphicsPipelineLayout);
		if (res != VK_SUCCESS)
		{
			fprintf(stderr, "failed to create graphics pipeline layout!\n");
			return APP_GENERIC_ERR;
		}
		printf("created graphics pipeline layout\n");
		m_progressStatus |= GRAPHICS_PIPELINE_LAYOUT_CREATED;

		// -- VkPipelineShaderStageCreateInfo describes the shaders to use in the graphics pipeline TODO number of stages hardcoded ----------------------------------------------------------------------------------------------
		// ---- creation of the shader modules
		// ------ evaluate file sizes
		namespace fs = std::filesystem;
		fs::path const relativeShaderPaths[MXC_RENDERER_SHADERS_COUNT] {"shaders/triangle.vert.spv","shaders/triangle.frag.spv"};// TODO hardcode of shaders numbers
		fs::path const shaderPaths[MXC_RENDERER_SHADERS_COUNT] = {fs::current_path() / relativeShaderPaths[0], fs::current_path() / relativeShaderPaths[1]}; 
		std::error_code ecs[MXC_RENDERER_SHADERS_COUNT];
		size_t const shaderSizes[MXC_RENDERER_SHADERS_COUNT] {fs::file_size(shaderPaths[0], ecs[0]), fs::file_size(shaderPaths[1], ecs[1])}; // TODO check the error code, whose codes themselves are platform specific
		
		// ------ allocate char buffers to store shader binary data
		VectorCustom<char> shadersBuf[2];
		shadersBuf[0].resize(shaderSizes[0]);
		shadersBuf[1].resize(shaderSizes[1]);
		printf("current path is %s\nshader path of vertex shader: %s\nshader path of fragment shader: %s\n", fs::current_path().c_str(), shaderPaths[0].c_str(), shaderPaths[1].c_str());
		std::ifstream shaderStreams[MXC_RENDERER_SHADERS_COUNT] {std::ifstream(shaderPaths[0]), std::ifstream(shaderPaths[1])};
		if (!shaderStreams[0].is_open() || !shaderStreams[0].is_open())
		{
			fprintf(stderr, "failed to open shader files whyyyyyyyyyyyyyyyyyyyyyyyy\n");
			return APP_GENERIC_ERR;
		}
		shaderStreams[0].read(shadersBuf[0].data(), shaderSizes[0]);
		shaderStreams[1].read(shadersBuf[1].data(), shaderSizes[1]);

		printf("vector sizes are %u and %u\n", shadersBuf[0].size(), shadersBuf[1].size());
		assert(shadersBuf[0].size() != 0 && shadersBuf[1].size() != 0);

		VkShaderModuleCreateInfo const shaderModuleCreateInfos[MXC_RENDERER_SHADERS_COUNT] {
			{ // Vertex Shader Module Create Info
				.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
				.pNext = nullptr,
				.flags = 0,
				.codeSize = shaderSizes[0],
				.pCode = reinterpret_cast<uint32_t*>(shadersBuf[0].data())
			},
			{ // Fragment Shader Module Create Info
				.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
				.pNext = nullptr,
				.flags = 0,
				.codeSize = shaderSizes[1],
				.pCode = reinterpret_cast<uint32_t*>(shadersBuf[1].data())
			}
		};

		VkShaderModule shaders[MXC_RENDERER_SHADERS_COUNT] = {VK_NULL_HANDLE};
		if ( vkCreateShaderModule(m_device, &shaderModuleCreateInfos[0],/*VkAllocationCallbacks**/nullptr, &shaders[0]) != VK_SUCCESS 
			|| vkCreateShaderModule(m_device, &shaderModuleCreateInfos[1], /*VkAllocationCallbacks**/nullptr, &shaders[1]) != VK_SUCCESS)
		{
			vkDestroyPipelineLayout(m_device, m_graphicsPipelineLayout, /*VkAllocationCallbacks**/nullptr);
			return APP_GENERIC_ERR;
		}

		// ---- creation of the information structure
		VkPipelineShaderStageCreateInfo const graphicsPipelineShaderStageCIs[MXC_RENDERER_SHADERS_COUNT] {
			VkPipelineShaderStageCreateInfo{ // VERTEX SHADER
				.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
				.pNext = nullptr, // TODO
				.flags = 0, // all of the flags regard SUBGROUPS, a group of tasks running in parallel in a compute unit (warp or wavefront). Too advanced TODO future
				.stage = VK_SHADER_STAGE_VERTEX_BIT,
				.module = shaders[0],
				.pName = "main",
				.pSpecializationInfo = nullptr// sepcialization constants are a mechanism to specify in a SPIR-V module constant values at pipeline creation time, which can be modified while executing the application TODO later
			},
			VkPipelineShaderStageCreateInfo{ // FRAGMENT SHADER
				.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
				.pNext = nullptr,
				.flags = 0,
				.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
				.module = shaders[1],
				.pName = "main",
				.pSpecializationInfo = nullptr
			}
		};

		// -- rest of the pipeline create info -------------------------------------------------------------------------------------------------------------------------------------------------------------
		VkVertexInputBindingDescription const vertInputBindingDescriptions[] {
			{
				.binding = 0, // Binding number which this structure is describing. You need a description for each binding in use. TODO we have no binding now
				.stride = sizeof(Vertex), // distance between successive elements in bytes (if attributes are stored interleaved, per vert, then stride = sizeof(Vertex))
				.inputRate = VK_VERTEX_INPUT_RATE_VERTEX // VkVertexInputRate specifies whether attributes stored in the buffer are PER VERTEX or PER INSTANCE
			}
		};

		VkVertexInputAttributeDescription const vertInputAttributeDescriptions[] {
			{ // position
				.location = 0,
				.binding = 0,
				.format = VK_FORMAT_R32G32B32_SFLOAT, // VkFormat
				.offset = offsetof(Vertex, pos) // in bytes, from the start of the current element
			},
			{ // color
				.location = 1,
				.binding = 0,
				.format = VK_FORMAT_R32G32B32_SFLOAT,
				.offset = offsetof(Vertex, col)
			}
		};

		VkPipelineColorBlendAttachmentState const colorBlendAttachmentStates[] {
			{
				.blendEnable = VK_TRUE,
				.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
				.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
				.colorBlendOp = VK_BLEND_OP_ADD,
				.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
				.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
				.alphaBlendOp = VK_BLEND_OP_ADD,
				.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT // specifies which components of the color has to be written alfter blending
			}
		};

		VkDynamicState const dynamicStates[2] {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR
		}; // TODO refactor to make it configurable

		VulkanPipelineConfig graphicsPipelineConfig;
		graphicsPipelineConfig.rasterizationStateCI.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		graphicsPipelineConfig.colorBlendStateCI.pAttachments = colorBlendAttachmentStates;
		graphicsPipelineConfig.dynamicStateCI.dynamicStateCount = 2; // TODO make it configurable
		graphicsPipelineConfig.dynamicStateCI.pDynamicStates = dynamicStates;

		graphicsPipelineConfig.vertexInputStateCI.vertexBindingDescriptionCount = 1; // TODO make it configurable, ie DYNAMIC
		graphicsPipelineConfig.vertexInputStateCI.pVertexBindingDescriptions = vertInputBindingDescriptions;
		graphicsPipelineConfig.vertexInputStateCI.vertexAttributeDescriptionCount = 2;
		graphicsPipelineConfig.vertexInputStateCI.pVertexAttributeDescriptions = vertInputAttributeDescriptions;

		// TODO setup pipeline cache
		// -- finally assemble the graphics pipeline ------------------------------------------------------------------------------------------------
		VkGraphicsPipelineCreateInfo const graphicsPipelineCreateInfo {
			.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
			.pNext = nullptr, // TODO you have no idea how many extension structures and flags there are
			.flags = 0,
			.stageCount = 2, // TODO change that
			.pStages = graphicsPipelineShaderStageCIs,
			.pVertexInputState = &graphicsPipelineConfig.vertexInputStateCI,
			.pInputAssemblyState = &graphicsPipelineConfig.inputAssemblyStateCI,
			.pTessellationState = &graphicsPipelineConfig.tessellationStateCI,
			.pViewportState = &graphicsPipelineConfig.viewportStateCI,
			.pRasterizationState = &graphicsPipelineConfig.rasterizationStateCI,
			.pMultisampleState = &graphicsPipelineConfig.multisampleStateCI,
			.pDepthStencilState = &graphicsPipelineConfig.depthStencilStateCI,
			.pColorBlendState = &graphicsPipelineConfig.colorBlendStateCI,
			.layout = m_graphicsPipelineLayout,
			.renderPass = m_renderPass,
			.subpass = 0, // subpass index in the renderpass. A pipeline will execute 1 subpass only.
			.basePipelineHandle = VK_NULL_HANDLE, // recycle old pipeline, requires PIPELINE_DERIVATIVES flag. TODO set this up
			.basePipelineIndex = 0 // doesn't count if basePipelineHandle is VK_NULL_HANDLE
		};

		res = vkCreateGraphicsPipelines(
			m_device,
			VK_NULL_HANDLE, // VkPipelineCache
			1, // createInfoCount
			&graphicsPipelineCreateInfo,
			nullptr, // VkAllocationCallbacks*
			&m_graphicsPipeline
		); // TODO enable pipeline caching, TODO do not hardcode pipeline number

		// -- cleanup ----------------------------------------------------------------------------------------------
		shaderStreams[0].close();
		shaderStreams[1].close();
		for (uint32_t i = 0u; i < MXC_RENDERER_SHADERS_COUNT; ++i)
			vkDestroyShaderModule(m_device, shaders[i], /*VkAllocationCallbacks**/nullptr);
		
		m_progressStatus |= GRAPHICS_PIPELINE_CREATED;
		printf("pipeline created!\n");
		return APP_SUCCESS;
	}

	template <template<class> class AllocTemplate> auto Renderer<AllocTemplate>::setupSynchronizationObjects() & -> status_t
	{
		assert((m_progressStatus & DEVICE_CREATED) && "device is required to create synchronization primitives!\n");
		
		m_fenceInFlightFrame.resize(m_swapchainImages.size());
		m_semaphoreImageAvailable.resize(m_swapchainImages.size());
		m_semaphoreRenderFinished.resize(m_swapchainImages.size());

		VkFenceCreateInfo const fenceCreateInfo {
			.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
			.pNext = nullptr, // export fence?
			.flags = VK_FENCE_CREATE_SIGNALED_BIT // we need it to start signaled otherwise the CPU will wait indefinitely on the first draw. this is today the only flag
		};

		VkSemaphoreTypeCreateInfo const semaphoreTypeCreateInfo {
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
			.pNext = nullptr, // export semaphore?
			.semaphoreType = VK_SEMAPHORE_TYPE_BINARY, // binary=signaled or unsignaled or timeline=counting semaphore
			.initialValue = 0U // counts only if semaphoreType is VK_SEMAPHORE_TYPE_TIMELINE. By the way, timeline semaphores REQUIRE A FEATURE
		};

		VkSemaphoreCreateInfo const semaphoreCreateInfo {
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
			.pNext = static_cast<const void*>(&semaphoreTypeCreateInfo),
			.flags = 0 // none for now
		};

		uint16_t created[m_swapchainImages.size()] = {};
		VkResult res;
		for (uint32_t i = 0u; i < m_swapchainImages.size(); ++i)
		{
			res = vkCreateFence(m_device, &fenceCreateInfo, /*VkAllocationCallbacks**/nullptr, &m_fenceInFlightFrame[i]);
			if (res != VK_SUCCESS)
			++created[0];
			res = vkCreateSemaphore(m_device, &semaphoreCreateInfo, /*VkAllocationCallbacks**/nullptr, &m_semaphoreImageAvailable[i]);
			if (res != VK_SUCCESS) break;
			++created[1];
			res = vkCreateSemaphore(m_device, &semaphoreCreateInfo, /*VkAllocationCallbacks**/nullptr, &m_semaphoreRenderFinished[i]);
			if (res != VK_SUCCESS) break;
			++created[2];
		}

		if (res != VK_SUCCESS)
		{
			for (uint32_t i = 0u; i < created[0]; ++i)
			{
				vkDestroyFence(m_device, m_fenceInFlightFrame[i], /*VkAllocationCallbacks**/nullptr);
			}
			for (uint32_t i = 0u; i < created[1]; ++i)
			{
				vkDestroySemaphore(m_device, m_semaphoreImageAvailable[i], /*VkAllocationCallbacks**/nullptr);
			}
			for (uint32_t i = 0u; i < created[2]; ++i)
			{
				vkDestroySemaphore(m_device, m_semaphoreRenderFinished[i], /*VkAllocationCallbacks**/nullptr);
			}

			fprintf(stderr, "failed to create synchronization primitives!\n");
			return APP_GENERIC_ERR;
		}

		printf("created fences and semaphores(can acquire image from swapchain and render finished) created\n");
		m_progressStatus |= SYNCHRONIZATION_OBJECTS_CREATED;
		return APP_SUCCESS;
	}

	template <template<class> class AllocTemplate> auto Renderer<AllocTemplate>::recordCommands(uint32_t framebufferIdx) & -> status_t
	{
		assert(framebufferIdx < m_swapchainImages.size() && "framebuffer index out of bounds");
		assert((m_progressStatus & (GRAPHICS_PIPELINE_CREATED | COMMAND_BUFFER_ALLOCATED)) && "command buffer recording requires a pipeline and a command buffer!\n");

		// should be called begin RECORDING, we are not executing any command here
		VkCommandBufferBeginInfo const cmdBufBeginInfo {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.pNext = nullptr, // can be pointer to vkgroupcommandbufferbegininfo if you setup a device group
			.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,// we have 3: one time submit = buffer will be reset after first use, render pass continue = for secondary cmd bufs only, specifies that this 2ndary cmd buf is entirely in renderpass, simultaneous use = allows to resubmit buffer to a queue while it is in the pending state, aka not finished
			.pInheritanceInfo = nullptr // used if this is a secondary buffer, defines the state that the secondary buffer will inherit from primary command buffer (render pass, subpass, framebuffer, and pipeline statistics)
		};
		VkClearValue const clearValues[] {
			// color attachment
			{.color = VkClearColorValue{0.3f, 0.3f, 0.3f, 1.f}},
			// depth attachment
			{.depthStencil = VkClearDepthStencilValue{.depth = 0.f, .stencil = 0u}} // stencil value ignored
		};
		VkRenderPassBeginInfo const renderPassBeginInfo {
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
			.pNext = nullptr, // can be stuff for device groups
			.renderPass = m_renderPass,
			.framebuffer = m_framebuffers[framebufferIdx],
			.renderArea = VkRect2D{VkOffset2D{0,0}, m_surfaceExtent},
			.clearValueCount = 2u, // array of clear colors, used if in the subpass the loadOp and/or stencilLoadOp was specified as VK_ATTACHMENT_LOAD_OP_CLEAR, there is one clear color for each attachment, AND THE ARRAY IS INDEXED BY THE ATTACHMENT NUMBER, so there can be holes
			.pClearValues = clearValues // using clear values for color attachment and depth attachment, to clear buffer values in the command buffer use vkCmdFillBuffer
		};

		vkBeginCommandBuffer(m_graphicsCmdBufs[framebufferIdx], &cmdBufBeginInfo); // TODO rework when command buffers become > 1
		{

			vkCmdBeginRenderPass(m_graphicsCmdBufs[framebufferIdx], &renderPassBeginInfo, /*VkSubpassContents*/VK_SUBPASS_CONTENTS_INLINE); // inline = no secondary buffers are executed in each subpass, while secondary means that subpass is recorded in a secondary command buffer
			{
				// bind graphics pipeline to render pass
				vkCmdBindPipeline(m_graphicsCmdBufs[framebufferIdx], /*VkPipelineBindPoint*/VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline); // bind point = type of pipeline to bind

				// bind vertex and index buffers
				VkBuffer const vertexBuffers[] {m_vertexBuffer};
				VkDeviceSize const offsets[] {0}; // offset from beginning to buffer, from which vulkan will bind
				vkCmdBindVertexBuffers(m_graphicsCmdBufs[framebufferIdx], 0/*first binding*/, 1/*binding count*/, vertexBuffers, offsets);
				vkCmdBindIndexBuffer(m_graphicsCmdBufs[framebufferIdx], m_indexBuffer, /*offset*/0, VK_INDEX_TYPE_UINT32);

				// TODO we didn't specify viewport and scissor to be dynamic for now, so no need to vkCmdSet them, but I'll come back
				VkViewport const viewport {
					.x = 0.f,// x,y define the upper-left corner of the viewport in screen coordinates. x,y must be >= viewportBoundsRange[0], x+width,y+height <= viewportBoundsRange[1]. they are VkPhysicalDeviceLimits
					.y = 0.f,
					.width = static_cast<float>(m_surfaceExtent.width), // width, height are the viewport's width and height. MUST BE LESS THAN the maximum specified in the device limits(in the device properties) // TODO setup checks TODO TODO important
					.height = static_cast<float>(m_surfaceExtent.height),
					.minDepth = 0.f, // viewport's width and height. min can be bigger than max (what even is the result then?), for values out of the range [0.f,1.f], you need extension depth_range_unrestricted
					.maxDepth = 1.f
				};

				VkRect2D const scissor {
					.offset = {0, 0}, // VkOffset type, has 2 SIGNED integers
					.extent = m_surfaceExtent // VkExtent type, has 2 UNSIGNED integers
				};
				vkCmdSetViewport(m_graphicsCmdBufs[framebufferIdx], 0/*1st viewport*/, 1/*viewport count*/, &viewport);
				vkCmdSetScissor(m_graphicsCmdBufs[framebufferIdx], 0/*1st scissor*/, 1/*scissor count*/, &scissor);

				// draw command. TODO refactor hardcoded numbers
				// vkCmdDraw(m_graphicsCmdBufs[framebufferIdx], /*vertexCount*/3, /*instance count*/1, /*firstVertexID*/0, /*firstInstanceID*/0); // vertex count == how many times to call the vertex shader != how many vertices we have stored in a buffer, instance count == number of times to draw the same primitives
				vkCmdDrawIndexed(m_graphicsCmdBufs[framebufferIdx], /*indexCount*/3, /*instanceCount*/1, /*firstIndexID*/0, /*vertexOffset*/0, /*firstInstance*/0);
			}
			vkCmdEndRenderPass(m_graphicsCmdBufs[framebufferIdx]);
		}
		VkResult const res = vkEndCommandBuffer(m_graphicsCmdBufs[framebufferIdx]);
		if (res != VK_SUCCESS)
		{
			return APP_GENERIC_ERR;
		}

		return APP_SUCCESS;
	}

	template <template<class> class AllocTemplate> auto Renderer<AllocTemplate>::draw() & -> status_t
	{
		VkResult res;


		uint32_t static currentFramebuffer = 0u;

		// wait for the previous frame to finish, as otherwise we would continue to submit command buffers indefinitely without knowing if the GPU has finished the previous one or not
		vkWaitForFences(m_device, /*fenceCount*/1, &m_fenceInFlightFrame[currentFramebuffer], /*wait all or any*/VK_TRUE, /*timeout*/0xffffffffffffffff); // TODO swap that hex for std::numeric_limits
		//printf("TIME TO DRAW\n");
		// ok next frame incoming. close the fence so that no other draw submission can get through until this one has finished. We do this because fences are not automatically closed
		vkResetFences(m_device, /*fenceCount*/1, &m_fenceInFlightFrame[currentFramebuffer]);

		// then acquire next available image from the swapchain. To be safe that it is not being presented, we will wait on semaphoreImageAvailable
		uint32_t imageIdx;
		res = vkAcquireNextImageKHR(m_device, m_swapchain, /*timeout in ns*/0xffffffffffffffff, m_semaphoreImageAvailable[currentFramebuffer], /*fenceToSignal*/VK_NULL_HANDLE, &imageIdx);
		// if the window is resized while the device is not idle, we need to catch here that the surface is not anymore compatible with our swapchain, and recreate it, together with framebuffers, images and pipeline
		if (res == VK_ERROR_OUT_OF_DATE_KHR)
		{
			fprintf(stderr, "Houston, we have a problem\n");
			return APP_REQUIRES_RESIZE_ERR;
		}
		
		// now reset, record and submit the command buffer
		if (vkResetCommandBuffer(m_graphicsCmdBufs[currentFramebuffer], /*reset flags*/0) != VK_SUCCESS) // only reset flag for now is "release all resources"
		{
			fprintf(stderr, "couldn't reset command buffer!\n");
			return APP_GENERIC_ERR;
		}
		
		recordCommands(currentFramebuffer);

		VkPipelineStageFlags const pipelineSemaphoreStageFlags[] {
			// VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT  -- we are not waiting till all execution of all stages before fragment shader are executed, BUT we are waiting only on the availability of the image to present to
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
		};
		VkSubmitInfo const submitInfo {
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.pNext = nullptr, // there is stuff for timeline semaphores, protected submit
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = &m_semaphoreImageAvailable[currentFramebuffer],
			.pWaitDstStageMask = pipelineSemaphoreStageFlags, // where in the pipeline the semaphore has to be waited on
			.commandBufferCount = 1,
			.pCommandBuffers = &m_graphicsCmdBufs[currentFramebuffer],
			.signalSemaphoreCount = 1,
			.pSignalSemaphores = &m_semaphoreRenderFinished[currentFramebuffer] // when rendering is done signal this semaphore, that will be waited on to present the image to the screen
		};

		res = vkQueueSubmit(m_queues[0], /*submitCount*/1, &submitInfo, /*fenceToSignal*/m_fenceInFlightFrame[currentFramebuffer]);
		if (res != VK_SUCCESS)
		{
			fprintf(stderr, "failed submitting draw operation!\n");
			switch (res)
			{
				case VK_ERROR_OUT_OF_HOST_MEMORY: fprintf(stderr, "VK_ERROR_OUT_OF_HOST_MEMORY\n"); break;
				case VK_ERROR_OUT_OF_DEVICE_MEMORY: fprintf(stderr, "VK_ERROR_OUT_OF_DEVICE_MEMORY\n"); break;
				case VK_ERROR_DEVICE_LOST: fprintf(stderr, "VK_ERROR_DEVICE_LOST\n"); break;
			}
			return APP_GENERIC_ERR;
		}

		// once drawing is completed, we can present the image
		VkPresentInfoKHR const presentInfo {
			.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
			.pNext = nullptr,
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = &m_semaphoreRenderFinished[currentFramebuffer],
			.swapchainCount = 1,
			.pSwapchains = &m_swapchain,
			.pImageIndices = &imageIdx,
			.pResults = nullptr // pointer to an array containing swapchainCount results, indicating outcome of each presentation operation
		};
		
		res = vkQueuePresentKHR(m_queues[0], &presentInfo);
		if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR) // TODO lacks if window was resized
		{
			fprintf(stderr, "Houston, we have a problem\n");
			return APP_REQUIRES_RESIZE_ERR;
		}
		if (res != VK_SUCCESS)
		{
			fprintf(stderr, "failed to present image!\n");
			return APP_GENERIC_ERR;
		}

		//printf("currentFramebuffer = %u\n", currentFramebuffer);
		currentFramebuffer = (currentFramebuffer + 1) % m_swapchainImages.size();
		return APP_SUCCESS;
	}

	template <template<class> class AllocTemplate>
	auto Renderer<AllocTemplate>::resize(uint32_t width, uint32_t height) & -> status_t
	{
		vkDeviceWaitIdle(m_device);

		vkDestroyPipeline(m_device, m_graphicsPipeline, /*VkAllocationCallbacks**/nullptr);
		vkDestroyPipelineLayout(m_device, m_graphicsPipelineLayout, /*VkAllocationCallbacks**/nullptr);

		// TODO vkDestroyFramebuffer times 3, then free memory 
		for (uint32_t i = 0; i < m_swapchainImages.size(); ++i)
		{
			vkDestroyFramebuffer(m_device, m_framebuffers[i], /*VkAllocationCallbacks**/nullptr);
		}
		
		// destroy depth buffer
		printf("remember to clean up device memory!\n");
		vkFreeMemory(m_device, m_depthImageMemory, /*VkAllocationCallbacks**/nullptr);
		vkDestroyImageView(m_device, m_depthImageView, /*VkAllocationCallbacks**/nullptr);
		vkDestroyImage(m_device, m_depthImage, /*VkAllocationCallbacks**/nullptr);
		
		vkFreeCommandBuffers(m_device, m_graphicsCmdPool, static_cast<uint32_t>(m_swapchainImages.size()), m_graphicsCmdBufs.data());

		for (uint32_t i = 0u; i < m_swapchainImages.size(); ++i)
		{
			vkDestroyImageView(m_device, m_swapchainImageViews[i], /*VkAllocationCallback**/nullptr);
		}

		setupSwapchain(width, height);
		setupDepthImage();
		setupDepthDeviceMemory();
		setupFramebuffers();
		setupGraphicsPipeline();

		return APP_SUCCESS;
	}

	template <template<class> class AllocTemplate>
	auto Renderer<AllocTemplate>::progress_incomplete() const & -> status_t 
	{
		status_t const status = m_progressStatus ^ (
			VERTEX_INPUT_BOUND |
			SYNCHRONIZATION_OBJECTS_CREATED |
			GRAPHICS_PIPELINE_CREATED | 
			GRAPHICS_PIPELINE_LAYOUT_CREATED |
			FRAMEBUFFERS_CREATED |
			DEPTH_MEMORY_ALLOCATED |
			DEPTH_IMAGE_CREATED |
			RENDERPASS_CREATED |
			COMMAND_BUFFER_ALLOCATED |
			SWAPCHAIN_IMAGE_VIEWS_CREATED |
			SWAPCHAIN_CREATED |
			MXC_RENDERER_INSERT_MESSENGER |
			INITIALIZED |
			INSTANCE_CREATED |
			SURFACE_CREATED |
			PHY_DEVICE_GOT |
			DEVICE_CREATED 
		);
		printf("renderer status is %x\n", static_cast<uint32_t>(status));
		return status;
	}

	class app
	{
	public:
		app() : m_window(nullptr), m_renderer(), m_progressStatus(0u) {}
		~app();

		// window and vulkan initialization
		auto init() -> status_t;
	
		// application execution
		// NOTE: IT CONTROLS IF ALL USED BITS IN PROGRESS ARE SET, if you add some, change this
		auto run() -> status_t;
	public: // function utilities	
		auto progress_incomplete() -> status_t;

	private: // functions


	private: // data
		// main components
		GLFWwindow* m_window;
		Renderer<Mallocator> m_renderer;

	private: // utilities functions
		auto static framebufferResizeCallbackGLFW(GLFWwindow* window, int32_t width, int32_t height) -> void;
		auto static errorCallbackGLFW(int errCode, char const * errMsg) -> void;

	private: // utilities data members
		uint32_t m_progressStatus; // BIT FIELD {glfw lib initialized?, glfw window created?}
		/**NOTE
		 * if this pattern of defining an m_progressStatus member with its own progress enum type and
		 * defining a switch which check all the states, it could THEORETICALLY be done with static
		 * reflection automatically to spare time on class definition TODO look into that
		 */
		#define MXC_APP_PROGRESS_UNUSED_BITS 0x3fffffff
		enum m_Progress_t : uint32_t
		{
			GLFW_INITIALIZED = 0x80000000,
			GLFW_WINDOW_CREATED = 0x40000000,
		};
	};

	auto app::framebufferResizeCallbackGLFW(GLFWwindow* window, int32_t width, int32_t height) -> void
	{
		// while the window is minimized, wait
		while (width == 0 | height == 0)
		{
			glfwWaitEvents();
		}
		auto renderer = reinterpret_cast<Renderer<Mallocator>*>(glfwGetWindowUserPointer(window));
		renderer->resize(width, height);
	}

	auto app::errorCallbackGLFW(int errCode, char const * errMsg) -> void
	{
		switch (errCode)
		{
			case GLFW_NOT_INITIALIZED:
 				fprintf(stderr, "GLFW has not been initialized.\n"); break;
			case GLFW_NO_CURRENT_CONTEXT:
				 fprintf(stderr, "No context is current for this thread.\n"); break;
			case GLFW_INVALID_ENUM:
				 fprintf(stderr, "One of the enum parameters for the function was given an invalid enum.\n"); break;
			case GLFW_INVALID_VALUE:
				 fprintf(stderr, "One of the parameters for the function was given an invalid value.\n"); break;
			case GLFW_OUT_OF_MEMORY:
				 fprintf(stderr, "A memory allocation failed.\n"); break;
			case GLFW_API_UNAVAILABLE:
				 fprintf(stderr, "GLFW could not find support for the requested client API on the system.\n"); break;
			case GLFW_VERSION_UNAVAILABLE:
				 fprintf(stderr, "The requested client API version is not available.\n"); break;
			case GLFW_PLATFORM_ERROR:
				 fprintf(stderr, "A platform-specific error occurred that does not match any of the more specific categories.\n"); break;
			case GLFW_FORMAT_UNAVAILABLE:
				 fprintf(stderr, "The clipboard did not contain data in the requested format.\n"); break;
		}
	}

	auto app::init() -> status_t
	{
		glfwSetErrorCallback(reinterpret_cast<GLFWerrorfun>(errorCallbackGLFW));
		// TODO refactor all in a big if statement, eg.:
		// if (  operation1
		//    && operation2
		//    && operation3 ...
		// 	  )
		// {
		//     return APP_SUCCESS;
		// }
		// else
		// {
		//     return APP_GENERIC_ERR;
		// }
		// try to initialize the glfw library and see if you can find eligeable vulkan drivers
		if (glfwInit() == GLFW_FALSE)
		{
			fprintf(stderr, "glfw failed to init!\n");
			return APP_GENERIC_ERR;
		}
		
		if (glfwVulkanSupported() == GLFW_FALSE)
		{
			fprintf(stderr, "vulkan NOT supported!\n");
			return APP_GENERIC_ERR;
		}

		m_progressStatus |= GLFW_INITIALIZED;
		
		/*** GLFW window creation ***/
		// Specify we do not want to use OpenGL, we are going to setup Vulkan
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

		// TODO = for now we disable resizing
		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
		
		// assuming we don't need more than 1 monitor and to specify properties of the display itself
		// we can skip creation of GLFWvidmode and GLFWmonitor
		m_window = glfwCreateWindow(
			WINDOW_WIDTH, 
			WINDOW_HEIGHT, 
			"window_title",
			nullptr,        // GLFWmonitor to use, required only in fullscreen and windowed fullscreen mode
			nullptr			// other GLFWwindow with which the new window will share resources
		);
		if (!m_window)
		{
			fprintf(stderr, "failed to create window!\n");
			return APP_GENERIC_ERR;
		}

		m_progressStatus |= GLFW_WINDOW_CREATED;

		/**NOTE:
		 * when using OpenGL/OpenGL_ES, GLFW windows will also have a OpenGL context, because OpenGL is more tightly coupled to the platform specific windowing system (whose creation we disabled with
		 * 
		 * GLFW_NO_API above). When managing more windows, with their set of context (shared or exclusive), there are some functions which require to know, which window is being used, a.k.a. which is 
		 * the "current context", example when swapping buffers. Since we are using Vulkan, we'll do none of that and instead use the KHR extension WSI, to interface with the windowing system
		 */
		/*** instance support and physical device presentation support and creation ***/
		uint32_t glfwInstanceExtensionCnt;
		char const** glfwInstanceExtensions = glfwGetRequiredInstanceExtensions(&glfwInstanceExtensionCnt);
		/**NOTE:
		 * if you don't need any additional extension other than those required by GLFW, then you can pass this pointer directly to ppEnabledExtensions in VkInstanceCreateInfo.
		 * Assuming we could need more in the future, I will store them as state by a dynamically allocated buffer in the renderer
		 */
		std::vector<char const*, Mallocator<char const*>> desiredInstanceExtensions;
		desiredInstanceExtensions.reserve(glfwInstanceExtensionCnt + 1);
		for (uint32_t i = 0; i < glfwInstanceExtensionCnt; ++i)
		{
			desiredInstanceExtensions.emplace_back(glfwInstanceExtensions[i]);
		}
		#ifndef NDEBUG
		desiredInstanceExtensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		#endif

		std::vector<char const*, Mallocator<char const*>> desiredDeviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
		std::vector<Vertex, Mallocator<Vertex>> vertexInput {
			{{0.f, -0.4f, 0.f}, {1.f, 0.f, 0.f}},
			{{-0.4f, 0.4f, 0.f}, {0.f, 1.f, 0.f}},
			{{0.4f, 0.4f, 0.f}, {0.f, 0.f, 1.f}}
		};
		std::vector<uint32_t, Mallocator<uint32_t>> indexInput {
			0, 1, 2
		};

		if (m_renderer.init(std::span(desiredInstanceExtensions.begin(), desiredInstanceExtensions.end()), 
							std::span(desiredDeviceExtensions.begin(), desiredDeviceExtensions.end()), m_window, 
							WINDOW_WIDTH, WINDOW_HEIGHT,
							vertexInput, indexInput) == APP_GENERIC_ERR)
		{
			return APP_GENERIC_ERR;
		}

		// setup code for resizing window
		glfwSetWindowUserPointer(m_window, reinterpret_cast<void*>(&m_renderer));
		glfwSetFramebufferSizeCallback(m_window, reinterpret_cast<GLFWframebuffersizefun>(framebufferResizeCallbackGLFW));
		
		// TODO add app initialized status when finish everything
		return APP_SUCCESS;
	}	

	auto app::progress_incomplete() -> status_t
	{
		status_t app_status = m_progressStatus ^ (
			app::GLFW_INITIALIZED |
			app::GLFW_WINDOW_CREATED
		);
		printf("app status, before bitwise or with renderer status, is %x, while m_progressStatus is %x\n", app_status, m_progressStatus);
		app_status = app_status | m_renderer.progress_incomplete(); // TODO
		printf("app status is %x\n", app_status);
		return app_status;
	}

	auto app::run() -> status_t
	{
		printf("run has been called\n");
		if (progress_incomplete())
		{
			fprintf(stderr, "\ninitialization failed, closing app\n");
			return APP_INIT_FAILURE;
		}
		while (!glfwWindowShouldClose(m_window))
		{
			/*** rendering stuff ***/
			status_t res = m_renderer.draw();
			if (res == APP_REQUIRES_RESIZE_ERR)
			{
				int32_t width, height;
				glfwGetFramebufferSize(m_window, &width, &height);
				printf("new width: %d, new height: %d\n", width, height);
				m_renderer.resize(width, height);
			}
			else if (res != APP_SUCCESS)
			{
				return APP_GENERIC_ERR;
			}

			/*** reset command buffers ***/
			

			/*** poll events. NOTE TODO: use glfwPollEvents() when doing intense rendering. Using it
			 *** in the beginning would just waste processing time in polling
			 ***/
			glfwWaitEvents();
		}
		
		return APP_SUCCESS;
	}

	app::~app()
	{
		// implementing conditional destruction of state with switch fallthrough
		switch (m_progressStatus)
		{
			case m_Progress_t::GLFW_INITIALIZED | m_Progress_t::GLFW_WINDOW_CREATED: 
				glfwDestroyWindow(m_window);
			case m_Progress_t::GLFW_INITIALIZED:
				glfwTerminate();
		}
	}
}

auto main(int32_t argc, char* argv[]) -> int
{
	mxc::app app_instance;
	app_instance.init();
	if (app_instance.run())
	{
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
