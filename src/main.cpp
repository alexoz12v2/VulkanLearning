#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <cstddef>
#include <cstdint>
#include <cstdlib> // TODO check for possible removal
#include <cstdio> // TODO check its usage for possible removal in non debug builds
#include <cstring>
#include <cassert>

// possibly change to use filesystem
#include <filesystem>
#include <fstream>
#include <vector> // TODO remove

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

// NOTE: glfwGetGammaRamp to get the monitor's gamma
// NOTE: to get started more "smoothly", I WILL NOT DO CUSTOM ALLOCATION IN THIS MOCK APPLICATION
// 		 subsequent Vulkan trainings will include more as I learn from scratch graphics development

#define WINDOW_WIDTH 600
#define WINDOW_HEIGHT 480

namespace mxc
{
	// GLFW and Vulkan uses 32 bit unsigned int as bool, i'll provide my definition as well
	using bool32_t = uint32_t;
	#define APP_FALSE 0
	#define APP_TRUE 1

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
	//... more errors
	
	// TODO change return conventions into more meaningful and specific error carrying type to convey a more 
	// 		specific status report
	/*** WARNING: most of the functions will return APP_TRUE if successful, and APP_FALSE if unsuccessful ***/

	class renderer
	{
	public: // constructors
		renderer();
		renderer(renderer const&) = delete;
		auto operator=(renderer const&) -> renderer = delete;
		// TODO define move semantics
		~renderer();
		
	public: // public functions, initialization procedures
		auto init(uint32_t const ext_count, char const** ext_names, GLFWwindow* window) & -> status_t; // TODO init arguments refactored in a customizeable struct
		auto setupInstance(uint32_t const ext_count, char const** ext_names) & -> status_t;
		auto setupSurfaceKHR(GLFWwindow* window) & -> status_t; // TODO refactor
		auto setupPhyDevice() & -> status_t;
		auto setupDeviceAndQueues() & -> status_t;
		auto setupSwapchain() & -> status_t;
		auto setupCommandBuffers() & -> status_t;
		// auto setupStagingBuffer() & -> status_t;
		auto setupDepthImage() & -> status_t;
		auto setupDepthDeviceMemory() & -> status_t;
		auto setupRenderPass() & -> status_t;
		auto setupFramebuffers() & -> status_t;
		auto setupGraphicsPipeline() & -> status_t;
		auto recordCommands(uint32_t framebufferIdx) & -> status_t;
		auto setupSynchronizationObjects() & -> status_t;
		auto draw() & -> status_t;

	public: // public function, utilities
		auto progress_incomplete() const & -> status_t; // TODO: const correct and ref correct members
												  
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
		std::vector<VkCommandBuffer> m_graphicsCmdBufs; // these two are created/allocated for the m_queueIdx.graphics queue

		VkRenderPass m_renderPass;
		// color output attachment and depth output attachment
		#define MXC_RENDERER_ATTACHMENT_COUNT 1
		//VkImage m_depthImage;
		//VkImageView m_depthImageView;
		//VkDeviceMemory m_depthImageMemory;

		std::vector<VkFramebuffer> m_framebuffers; // triple buffering
			
		#define MXC_RENDERER_SHADERS_COUNT 2
		VkPipeline m_graphicsPipeline;
		VkPipelineLayout m_graphicsPipelineLayout;

		std::vector<VkFence> m_fenceInFlightFrame;
		// we are using triple buffering, so at least 2 pairs of semaphores should be created, to be safe we will associate a pair of semaphores to each framebuffer
		std::vector<VkSemaphore> m_semaphoreImageAvailable;
		std::vector<VkSemaphore> m_semaphoreRenderFinished;

		// presentation WSI extension
		VkSurfaceKHR m_surface;
		VkSurfaceFormatKHR m_surfaceFormatUsed;
		VkPresentModeKHR m_presentModeUsed;
		VkSurfaceCapabilitiesKHR m_surfaceCapabilities; // TODO remember to check these in the creation of swapchain, pipeline, ...
		VkSwapchainKHR m_swapchain;
		VkImage* m_pSwapchainImages; // swapchainImages and swapchainImageViews are dynamically allocated, free them. see setupSwapchain for allocation
		VkImageView* m_pSwapchainImageViews;
		uint32_t m_swapchainImagesCnt;
		VkExtent2D m_surfaceExtent;

		// other vulkan related
		//VkFormat m_depthImageFormat;

#ifndef NDEBUG // CMAKE_BUILD_TYPE=Debug
		VkDebugUtilsMessengerEXT m_dbgMessenger;
		#define MXC_RENDERER_INSERT_MESSENGER MESSENGER_CREATED
#elif
		#define MXC_RENDERER_INSERT_MESSENGER 0
#endif // ifndef NDEBUG

		// renderer state machine to TODO refactor. type is below
		uint32_t m_progressStatus;

	private: // data members, utilities
		class extensions_configurator
		{
		public: 
			extensions_configurator(): m_registered_exts(nullptr), m_registered_ext_count(0u), m_registered_ext_capacity(0u),
					 				   m_registered_ext_buffer(nullptr) {}
			~extensions_configurator();

		public:
			auto register_extensions(uint32_t const in_ext_count, char const** in_ext_names) & -> status_t;
			auto init() & -> status_t;

		public:
			// TODO I don't want to hold onto extensions forever. When can they be destroyed? move them
			// TODO change this array in a form of contiguous binary tree, to guarantee uniqueness
			char** m_registered_exts;
			uint32_t m_registered_ext_count;
			uint32_t m_registered_ext_capacity;
			char(* m_registered_ext_buffer)[VK_MAX_EXTENSION_NAME_SIZE]; // VK_MAX_EXTENSION_NAME_SIZE = 256u
		};

		// the enum class is a fully fledged type. All we need are scoped aliases for numbers and to perform bitwise operations
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
			//DEPTH_IMAGE_CREATED = 0x00100000,
			//DEPTH_MEMORY_ALLOCATED = 0x00008000,
			FRAMEBUFFERS_CREATED = 0x00080000,
			GRAPHICS_PIPELINE_LAYOUT_CREATED = 0x00040000,
			GRAPHICS_PIPELINE_CREATED = 0x00020000,
			SYNCHRONIZATION_OBJECTS_CREATED = 0x00010000
		};
	
	private: // functions, utilities
	};

	renderer::renderer() 
			: m_instance(VK_NULL_HANDLE), m_phyDevice(VK_NULL_HANDLE), m_device(VK_NULL_HANDLE)
			, m_queueIdxArr{-1}, m_queues{VK_NULL_HANDLE} // TODO Don't forget to update m_queueIdxArr when adding queue types
			, m_graphicsCmdPool(VK_NULL_HANDLE), m_graphicsCmdBufs(std::vector<VkCommandBuffer>(0)), m_renderPass(VK_NULL_HANDLE)
			, m_framebuffers(std::vector<VkFramebuffer>(0)), m_graphicsPipeline(VK_NULL_HANDLE), m_graphicsPipelineLayout(VK_NULL_HANDLE)
			, m_fenceInFlightFrame(std::vector<VkFence>(0)), m_semaphoreImageAvailable(std::vector<VkSemaphore>(0)), m_semaphoreRenderFinished(std::vector<VkSemaphore>(0))//, m_depthImage(VK_NULL_HANDLE), m_depthImageView(VK_NULL_HANDLE)
			, /*m_depthImageMemory(VK_NULL_HANDLE),*/ m_surface(VK_NULL_HANDLE), m_surfaceFormatUsed(VK_FORMAT_UNDEFINED), m_presentModeUsed(VK_PRESENT_MODE_FIFO_KHR), m_surfaceCapabilities({0})
			, m_swapchain(VK_NULL_HANDLE), m_pSwapchainImages(nullptr), m_pSwapchainImageViews(nullptr), m_swapchainImagesCnt(0)
			, m_surfaceExtent(VkExtent2D{0,0})//, m_depthImageFormat(VK_FORMAT_D32_SFLOAT)
#ifndef NDEBUG // CMAKE_BUILD_TYPE=Debug
			, m_dbgMessenger(VK_NULL_HANDLE)
#endif
			, m_progressStatus(0u)
	{
	}				

	auto renderer::init(uint32_t const ext_count, char const** ext_names, GLFWwindow* window) & -> status_t
	{
		if (setupInstance(ext_count, ext_names)
			|| setupSurfaceKHR(window)
			|| setupPhyDevice()
			|| setupDeviceAndQueues()
			|| setupSwapchain()
			|| setupCommandBuffers()
			//|| setupDepthImage()
			//|| setupDepthDeviceMemory()
			|| setupRenderPass()
			|| setupFramebuffers()
			|| setupGraphicsPipeline()
			|| setupSynchronizationObjects())
		{
			return APP_GENERIC_ERR;
		}

		m_progressStatus |= m_Progress_t::INITIALIZED;
		return APP_SUCCESS;
	}

	auto renderer::extensions_configurator::init() & -> status_t
	{
		// allocate initial buffer to store up to 32 supported extensions
		void* memory_needed = malloc(32u * (sizeof(char[VK_MAX_EXTENSION_NAME_SIZE]) + sizeof(char*)));
		if (!memory_needed)
		{
			return APP_MEMORY_ERR;
		}

		m_registered_ext_buffer = static_cast<char(*)[VK_MAX_EXTENSION_NAME_SIZE]>(memory_needed);
		m_registered_exts = reinterpret_cast<char**>(reinterpret_cast<unsigned char*>(memory_needed) + 32u * sizeof(char[VK_MAX_EXTENSION_NAME_SIZE])); // TODO change to std::byte
		m_registered_ext_capacity = 32u;

		return APP_SUCCESS;	
	}

	renderer::extensions_configurator::~extensions_configurator()
	{
		if (m_registered_ext_buffer)
		{
			free(m_registered_ext_buffer); // don't free m_registered_exts too cause it's only 1 buffer
		}
	}

	renderer::~renderer()
	{
		printf("time to destroy renderer!\n");
		// wait until the device is idle to clean resources so we don't break anything with VkFences
		if (m_progressStatus & DEVICE_CREATED)
		{	
			vkDeviceWaitIdle(m_device);
		}

		// Then we can clean everything up
		if (m_progressStatus & SYNCHRONIZATION_OBJECTS_CREATED)
		{
			for (uint32_t i = 0u; i < m_swapchainImagesCnt; ++i)
			{
				vkDestroyFence(m_device, m_fenceInFlightFrame[i], /*VkAllocationCallbacks**/nullptr);
				vkDestroySemaphore(m_device, m_semaphoreImageAvailable[i], /*VkAllocationCallbacks**/nullptr);
				vkDestroySemaphore(m_device, m_semaphoreRenderFinished[i], /*VkAllocationCallbacks**/nullptr);
			}
		}

		if (m_progressStatus & GRAPHICS_PIPELINE_CREATED)
		{
			vkDestroyPipeline(m_device, m_graphicsPipeline, /*VkAllocationCallbacks**/nullptr);
		}

		if (m_progressStatus & GRAPHICS_PIPELINE_LAYOUT_CREATED)
		{
			vkDestroyPipelineLayout(m_device, m_graphicsPipelineLayout, /*VkAllocationCallbacks**/nullptr);
		}

		if (m_progressStatus & FRAMEBUFFERS_CREATED)
		{
			// TODO vkDestroyFramebuffer times 3, then free memory 
			for (uint32_t i = 0; i < m_swapchainImagesCnt; ++i)
			{
				vkDestroyFramebuffer(m_device, m_framebuffers[i], /*VkAllocationCallbacks**/nullptr);
			}
		}
		
		//if (m_progressStatus & DEPTH_MEMORY_ALLOCATED)
		//{
		//	printf("remember to clean up device memory!\n");
		//	vkFreeMemory(m_device, m_depthImageMemory, /*VkAllocationCallbacks**/nullptr);
		//}
		//
		//if (m_progressStatus & DEPTH_IMAGE_CREATED)
		//{
		//	vkDestroyImageView(m_device, m_depthImageView, /*VkAllocationCallbacks**/nullptr);
		//	vkDestroyImage(m_device, m_depthImage, /*VkAllocationCallbacks**/nullptr);
		//}
		
		if (m_progressStatus & RENDERPASS_CREATED)
		{
			vkDestroyRenderPass(m_device, m_renderPass, /*VkAllocationCallbacks**/nullptr);
		}
		
		if (m_progressStatus & COMMAND_BUFFER_ALLOCATED)
		{
			vkFreeCommandBuffers(m_device, m_graphicsCmdPool, m_swapchainImagesCnt, m_graphicsCmdBufs.data());
			vkDestroyCommandPool(m_device, m_graphicsCmdPool, /*VkAllocationCallbacks**/nullptr);
		}

		if (m_progressStatus & SWAPCHAIN_IMAGE_VIEWS_CREATED)
		{
			// frees both swapchain images and images views, they share the memory buffer
			free(m_pSwapchainImages);

			for (uint32_t i = 0u; i < m_swapchainImagesCnt; ++i)
			{
				vkDestroyImageView(m_device, m_pSwapchainImageViews[i], /*VkAllocationCallback**/nullptr);
			}
		}

		if (m_progressStatus & SWAPCHAIN_CREATED)
		{
			vkDestroySwapchainKHR(m_device, m_swapchain, /*VkAllocationCallbacks**/nullptr);
		}

		if (m_progressStatus & SURFACE_CREATED)
		{
			vkDestroySurfaceKHR(m_instance, m_surface, /*VkAllocationCallbacks**/nullptr);
		}

		if (m_progressStatus & DEVICE_CREATED)
		{
			// all its child objects need to be destroyed before doing this
			vkDestroyDevice(m_device, /*VkAllocationCallbacks**/nullptr);
		}

#ifndef NDEBUG // CMAKE_BUILD_TYPE=Debug
		if (m_progressStatus & MESSENGER_CREATED)
		{
			auto const vkDestroyDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
				vkGetInstanceProcAddr(m_instance, "vkDestroyDebugUtilsMessengerEXT")
			);
			if (vkDestroyDebugUtilsMessengerEXT)
			{
				vkDestroyDebugUtilsMessengerEXT(m_instance, m_dbgMessenger, /*pAllocationCallbacks**/nullptr);
			}
		}
#endif

		if (m_progressStatus & (INITIALIZED | INSTANCE_CREATED))
		{
			vkDestroyInstance(m_instance, /*VkAllocationCallbacks*/nullptr);
		}
		printf("destroyed renderer!\n");
	}
	
	// TODO this has to be modified when implementing a binary tree and custom allocations with
	// suballocation
	// TODO assumes in_ext_names are UNIQUE
	// TODO DISTINGUISH BETWEEN INSTANCE EXTENSIONS AND DEVICE EXTENSIONS _------------------------------------------=-=-=-=--0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-
	auto renderer::extensions_configurator::register_extensions(uint32_t const in_ext_count, char const** in_ext_names) & -> status_t
	{
		// check if given extensions are actually supported by our vulkan instance
		uint32_t extension_cnt;
		vkEnumerateInstanceExtensionProperties(
			nullptr,        // const char* pLayerName
			&extension_cnt,
			nullptr         // VkExtensionProperties* pProperties 
		);

		auto* const ext_properties_buf = static_cast<VkExtensionProperties*>(malloc(extension_cnt * (sizeof(VkExtensionProperties) + sizeof(uint32_t))));
		if (!ext_properties_buf)
		{
			return APP_MEMORY_ERR;
		}
		vkEnumerateInstanceExtensionProperties(nullptr, &extension_cnt, ext_properties_buf);

		uint32_t* const ext_supported_indices = 
				static_cast<uint32_t*>(static_cast<void*>(ext_properties_buf + extension_cnt));
		uint32_t ext_supported_indices_cnt = 0u;
		// TODO rework this step
		uint32_t* current_empty_index = ext_supported_indices;
		for (uint32_t i = 0u; i < in_ext_count; ++i)
		{
			for (uint32_t j = 0u; j < extension_cnt; ++j)
			{
				if (0 == strncmp(in_ext_names[i], ext_properties_buf[j].extensionName, VK_MAX_EXTENSION_NAME_SIZE))
				{
					*current_empty_index++ = i;
					++ext_supported_indices_cnt;
					break;
				}
			}
		}

		// allocate more registered extension names if buffer is not big enough
		if (ext_supported_indices_cnt + m_registered_ext_count > m_registered_ext_capacity)
		{
			void* tmp_buf = malloc(2u * m_registered_ext_capacity * (sizeof(char[VK_MAX_EXTENSION_NAME_SIZE]) + sizeof(char*)));
			if (!tmp_buf)
			{
				free(ext_properties_buf);
				return APP_MEMORY_ERR;
			}
			
			memcpy(tmp_buf, 
				   m_registered_ext_buffer,
				   sizeof(char[VK_MAX_EXTENSION_NAME_SIZE])*m_registered_ext_count);
			void* old = m_registered_ext_buffer;
			m_registered_ext_buffer = static_cast<char(*)[VK_MAX_EXTENSION_NAME_SIZE]>(tmp_buf);

			memcpy(m_registered_ext_buffer + m_registered_ext_capacity*2u,
				   m_registered_exts,
				   sizeof(char*)*m_registered_ext_count);
			m_registered_exts = reinterpret_cast<char**>(reinterpret_cast<unsigned char*>(tmp_buf) + sizeof(char[VK_MAX_EXTENSION_NAME_SIZE])*m_registered_ext_capacity*2u); // TODO unsigned char could be changed to std::byte

			free(old);
			m_registered_ext_capacity *= 2u;
		}
		// names could also NOT be stored contiguously in the format specified by the class
		// therefore we need to copy them one by one, by checking each character for the nul terminator
		// (i.e. using strcpy)
		uint32_t registered_ext_count_working_copy = m_registered_ext_count;
		for (uint32_t i = 0u; i < ext_supported_indices_cnt; ++i)
		{
			char* cpy_address = reinterpret_cast<char*>((m_registered_ext_buffer + registered_ext_count_working_copy++));
			strncpy(cpy_address, in_ext_names[i], VK_MAX_EXTENSION_NAME_SIZE);
			
			// now setup pointers in m_registered_exts
			uint32_t j = i + m_registered_ext_count;
			m_registered_exts[j] = cpy_address;
		}

		m_registered_ext_count = registered_ext_count_working_copy;

		free(ext_properties_buf);
		printf("about to print the content of m_registered_ext\n");
		for (uint32_t i = 0u; i < m_registered_ext_count; ++i)
		{
			printf("%s\n", m_registered_exts[i]);
		}
		printf("total extensions: %u\n", m_registered_ext_count);
		return APP_SUCCESS;
	}

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


#endif // ifndef NDEBUG
	
	auto renderer::setupInstance(uint32_t const ext_count, char const** ext_names) & -> status_t
	{
     	// find supported version of vulkan
		uint32_t supported_vk_api_version;
    	vkEnumerateInstanceVersion(&supported_vk_api_version);	
       	if (supported_vk_api_version < VK_MAKE_API_VERSION(/*variant*/0,/*major*/1,/*minor*/2,/*patch*/0))
       	{
			// TODO substitute with a primitive logging with pretty printing, enabled only in Debug
       		fprintf(stderr, "system does not support vulkan 1.2 or higher!");
			return APP_LACK_OF_VULKAN_1_2_SUPPORT_ERR;
       	}

		extensions_configurator instance_exts;
		if (status_t status = instance_exts.init(); status != APP_SUCCESS)
		{
			return status;
		}

		instance_exts.register_extensions(ext_count, ext_names);

		// create the Vkinstance TODO will be modified to include validation layers
		VkApplicationInfo const application_info = {
			.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
			.pNext = nullptr,
			.pApplicationName = "my first vulkan app",
			.applicationVersion = 1u,
			.pEngineName = "my first vulkan app",
			.engineVersion = 1u,
			.apiVersion = VK_MAKE_API_VERSION(/*variant*/0,/*major*/1,/*minor*/2,/*patch*/0) // the only useful thing here
		};

		// extensions required by glfw and other extensions are registered by m_registered_ext_buffer
		// while layers are queried here with the use of the NDEBUF macro, which is set by cmake in 
		// a release build
		// TODO rework this section so that debug and release code are fully separated, even if redundant
		#ifndef NDEBUG // CMAKE_BUILD_TYPE=Debug
		
		// by default validation layers output to the standard output, which can be customized and altered by setting a MESSAGE CALLBACK
		// thanks to the extension VK_EXT_DEBUG_UTILS_EXTENSION_NAME, macro which resolves to "VK_EXT_debug_utils" TODO
		char const* msg_callback_ext_name = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
		if (instance_exts.register_extensions(1u, &msg_callback_ext_name))
		{
			return APP_GENERIC_ERR;
		}

		// "DEBUG CALLBACK": managed through a Vulkan Object Handle, of type VkDebugUtilsMessengerEXT, which should live as long as the vulkan
		// app requires to use vulkan with validation layers, therefore it is added as a member to our renderer class
		// NOTE: the extension specs shows more ways to setup a VkDebugUtilsMessengerEXT
		// NOTE: put "before instance creation even though to create a VkDebugUtilsMessengerEXT we require an instance" because to test the
		// 		 instance itself with a messenger, we need to pass in the VkInstanceCreateInfo.pNext a pointer to the dbg_msger_create_info,
		// 		 which will trigger the automatic creation and destruction of another VkDebugUtilsMessengerEXT
		VkDebugUtilsMessengerCreateInfoEXT const dbg_msger_create_info = {
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
	
		// desired validation layers, which then are checked against vkEnumerateInstanceLayerProperties to see if they are supported
		char const* const desired_validation_layers[] = {
			"VK_LAYER_KHRONOS_validation"
		};
		uint32_t const desired_validation_layers_cnt = sizeof(desired_validation_layers)/sizeof(char*);

		uint32_t layer_properties_cnt;
		vkEnumerateInstanceLayerProperties(&layer_properties_cnt, /*VkLayerProperties* props*/nullptr);
		
		// TODO: abstract details of memory
		// TODO change this to a struct of 2 arrays, which is essentially what I am creating here
		// allocating contiguous buffer which stores layer_properties, layer names in a flat contiguous format, and pointers to each string
		VkLayerProperties* const layer_properties = 
				static_cast<VkLayerProperties*>(malloc(layer_properties_cnt * (sizeof(VkLayerProperties) + sizeof(char*))));
		if (!layer_properties)
		{
			return APP_MEMORY_ERR;
		}

		vkEnumerateInstanceLayerProperties(&layer_properties_cnt, layer_properties);

		// since vulkan requires an array of pointers, traverse our contiguous structure to setup pointers
		char const** enabled_layers = reinterpret_cast<decltype(enabled_layers)>(layer_properties + layer_properties_cnt); // TODO this is just bad code
		uint32_t enabled_layers_cnt = 0u;
		for (uint32_t i = 0u; i < desired_validation_layers_cnt; ++i)
		{
			for (uint32_t j = 0u; j < layer_properties_cnt; ++j)
			{
				if (0 == strncmp(layer_properties[j].layerName, desired_validation_layers[i], VK_MAX_EXTENSION_NAME_SIZE))
				{
					enabled_layers[i] = layer_properties[j].layerName;
					++enabled_layers_cnt;
					break;
				}
			}
		}
		if (enabled_layers_cnt == 0u)
		{
			enabled_layers = nullptr;
			fprintf(stderr, "no layers were available!\n");
		}

		#endif // ifndef NDEBUG
		
		VkInstanceCreateInfo const instance_create_info{
			.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
#ifndef NDEBUG // CMAKE_BUILD_TYPE=Debug
			.pNext = &dbg_msger_create_info,
#elif
			.pNext = nullptr,
#endif // ifndef NDEBUG
			// Bitmask on some options that define the BEHAVIOUR OF THE INSTANCE
			.flags = 0x00000000,
			.pApplicationInfo = &application_info,
			#ifndef NDEBUG // CMAKE_BUILD_TYPE=Debug
			.enabledLayerCount = enabled_layers_cnt, // enabling ALL layers for now TODO
			.ppEnabledLayerNames = enabled_layers,
			#elif // CMAKE_BUILD_TYPE=Release
			.enabledLayerCount = 0u,
			.ppEnabledLayerNames = nullptr,
			#endif
			.enabledExtensionCount = instance_exts.m_registered_ext_count,
			.ppEnabledExtensionNames = instance_exts.m_registered_exts // TODO will change when data structure changes
		};

		#ifndef NDEBUG // CMAKE_BUILD_TYPE=Debug

		FILE* const dbg_ext_layers_names_fd = fopen("./ext_and_layers_names.txt", "w");
		if (dbg_ext_layers_names_fd)
		{
			printf("instance extensions and layers debug file created!\n");
			fprintf(dbg_ext_layers_names_fd, "extensions:\n");
			for (uint32_t i = 0u; i < instance_exts.m_registered_ext_count; ++i)
			{
				fprintf(dbg_ext_layers_names_fd, "%s\n", instance_exts.m_registered_exts[i]);
			}
			fprintf(dbg_ext_layers_names_fd, "validation layers:\n");
			for (uint32_t i = 0u; i < enabled_layers_cnt; ++i)
			{
				fprintf(dbg_ext_layers_names_fd, "%s\n", enabled_layers[i]);
			}
			fclose(dbg_ext_layers_names_fd);
		}
		else
		{
			printf("no debug file could be created!");
		}

		#endif // ifndef NDEBUG

		VkResult const instance_creation_result = vkCreateInstance(&instance_create_info, /*VkAllocationCallbacks**/nullptr, &m_instance);

		#ifndef NDEBUG // CMAKE_BUILD_TYPE=Debug
		for (uint32_t i = 0u; i < enabled_layers_cnt; ++i)
		{
			printf("what is this, layer %u is \"%s\"\n", i, enabled_layers[i]);
		}
		free(layer_properties);
		#endif // ifndef NDEBUG
		
		if (instance_creation_result != VK_SUCCESS)
		{
			fprintf(stderr, "instance creation result:\n");
			switch (instance_creation_result)
			{
				case VK_ERROR_OUT_OF_HOST_MEMORY:
					fprintf(stderr, "VK_ERROR_OUT_OF_HOST_MEMORY\n");
					break;
				case VK_ERROR_OUT_OF_DEVICE_MEMORY:
					fprintf(stderr, "VK_ERROR_OUT_OF_DEVICE_MEMORY\n");
					break;
				case VK_ERROR_INITIALIZATION_FAILED:
					fprintf(stderr, "VK_ERROR_INITIALIZATION_FAILED\n");
					break;
				case VK_ERROR_LAYER_NOT_PRESENT:
					fprintf(stderr, "VK_ERROR_LAYER_NOT_PRESENT\n");
					break;
				case VK_ERROR_EXTENSION_NOT_PRESENT:
					fprintf(stderr, "VK_ERROR_EXTENSION_NOT_PRESENT\n");
					break;
				case VK_ERROR_INCOMPATIBLE_DRIVER:
					fprintf(stderr, "VK_ERROR_INCOMPATIBLE_DRIVER\n");
					break;
			}
			return APP_GENERIC_ERR;
		}

		m_progressStatus |= m_Progress_t::INSTANCE_CREATED;
		printf("instance created!\n");

#ifndef NDEBUG // CMAKE_BUILD_TYPE=Debug
		// "DEBUG CALLBACK": managed through a Vulkan Object Handle, of type VkDebugUtilsMessengerEXT, which should live as long as the vulkan
		// app requires to use vulkan with validation layers, therefore it is added as a member to our renderer class
		// NOTE: the extension specs shows more ways to setup a VkDebugUtilsMessengerEXT

		/** function signature for the creation of a VkDebugUtilsMessengerEXT TODO move messenger creation after instance
		 * VkResult vkCreateDebugUtilsMessageEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT*, conts VkAllocationCallbacks*, VkDebugUtilsMessengerEXT* out_param)
		 * "BUT since this is an extension function, it is not automatically loaded and we need to call vkGetInstanceProcAddr(insance, funcname) ourselves" 
		 */
		auto const vkCreateDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(m_instance, "vkCreateDebugUtilsMessengerEXT"));
		if (!vkCreateDebugUtilsMessengerEXT)
		{
			return VK_ERROR_EXTENSION_NOT_PRESENT;
		}

		if (vkCreateDebugUtilsMessengerEXT(m_instance, &dbg_msger_create_info, /*VkAllocationCallbacks**/nullptr, &m_dbgMessenger) != VK_SUCCESS)
		{
			return APP_GENERIC_ERR;
		}

		m_progressStatus |= MESSENGER_CREATED;

#endif // ifndef NDEBUG	
		return APP_SUCCESS;
	}

	auto renderer::setupPhyDevice() & -> status_t
	{
		assert(m_progressStatus & INSTANCE_CREATED);
		assert(m_progressStatus & SURFACE_CREATED);

		// retrieve list of vulkan capable physical devices
		uint32_t phy_devices_cnt;
		vkEnumeratePhysicalDevices(m_instance, &phy_devices_cnt, nullptr);

		auto phy_devices = static_cast<VkPhysicalDevice*>(malloc(phy_devices_cnt * sizeof(VkPhysicalDevice)));
		if (!phy_devices)
		{
			return APP_MEMORY_ERR;
		}

		vkEnumeratePhysicalDevices(m_instance, &phy_devices_cnt, phy_devices);

		// then you can query the properties of the physical device with vkGetPhysicalDeviceProperties and choose the one
		// that suits your needs. I understand none of that, therefore I'm going to choose the first available DEDICATED GPU
		// you have no idea how many more get properties functions there are.
		// we will also query the available queue families. for now we need a graphics queue and a presentation enabled queue (WSI)
		for (uint32_t i = 0u; i < phy_devices_cnt; ++i)
		{
			// query properties
			VkPhysicalDeviceProperties phy_device_properties;
			vkGetPhysicalDeviceProperties(phy_devices[i], &phy_device_properties);
			if (phy_device_properties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
			{
				continue;
			}

			// query features TODO

			// query available queue families with their associated indices. First query how many there are to allocate a buffer
			uint32_t queue_family_properties_cnt;
			vkGetPhysicalDeviceQueueFamilyProperties(phy_devices[i], &queue_family_properties_cnt, nullptr);
			if (queue_family_properties_cnt == 0u)
			{
				continue;
			}

			auto queue_family_properties = static_cast<VkQueueFamilyProperties*>(malloc(queue_family_properties_cnt * sizeof(VkQueueFamilyProperties)));
			if (!queue_family_properties)
			{
				free(phy_devices);
				free(queue_family_properties);
				return APP_MEMORY_ERR;
			}
			
			// get queue family properties to check which queue families are available
			vkGetPhysicalDeviceQueueFamilyProperties(phy_devices[i], &queue_family_properties_cnt, queue_family_properties);
			for (uint32_t j = 0u; j < queue_family_properties_cnt; ++j)
			{
				if (m_queueIdx.graphics == -1 
					&& 0 != queue_family_properties->queueFlags & VK_QUEUE_GRAPHICS_BIT) // I care only for very few simple things
				{
					m_queueIdx.graphics = j;
				}
				
				// query for the given index presentation support for the given queue family
				if (m_queueIdx.presentation == -1)
				{
					VkBool32 presentation_supported;
					vkGetPhysicalDeviceSurfaceSupportKHR(phy_devices[i], /*queue family idx*/j, m_surface, &presentation_supported);
					if (presentation_supported)
					{
						m_queueIdx.presentation = j;
						break; // TODO move this break when adding more queue types
					}
				}
			}

			// now check if all necessary queues are present, if not continue with next device 
			if (m_queueIdx.graphics == -1 || m_queueIdx.presentation == -1)
			{
				free(queue_family_properties);
				continue;
			}

			// get and store the surface capabilities, an intersection between the display capabilities
			// and the physical device capabilities regarding presentation. Needed to create a swapchain
			vkGetPhysicalDeviceSurfaceCapabilitiesKHR(phy_devices[i], m_surface, &m_surfaceCapabilities);
			
			// now check for surface format support, we want R8G8B8A8 format, with sRGB nonlinear colorspace. PS=there are more advanced query functions
			// to check for more specific features, such as specifics about compression support
			uint32_t surface_format_cnt;
			vkGetPhysicalDeviceSurfaceFormatsKHR(phy_devices[i], m_surface, &surface_format_cnt, nullptr);
			if (surface_format_cnt == 0)
			{
				free(queue_family_properties);
				continue;
			}
	
			auto surface_formats = static_cast<VkSurfaceFormatKHR*>(malloc(surface_format_cnt * sizeof(VkSurfaceFormatKHR)));
			if (!surface_formats)
			{
				free(queue_family_properties);
				free(phy_devices);
				return APP_MEMORY_ERR;
			}

			vkGetPhysicalDeviceSurfaceFormatsKHR(phy_devices[i], m_surface, &surface_format_cnt, surface_formats);
			{
				uint32_t i = 0u;
				for (; i < surface_format_cnt; ++i)
				{
					// NOTE: format undefined, returned by vkGetPhysicalDeviceSurfaceFormatsKHR means all formats are supported under the associated color space
					if (surface_formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
						&& (surface_formats[i].format == VK_FORMAT_UNDEFINED || surface_formats[i].format == VK_FORMAT_R8G8B8A8_UNORM || VK_FORMAT_B8G8R8A8_UNORM))
					{
						m_surfaceFormatUsed = surface_formats[i];
						break;
					}
				}
				if (i == surface_format_cnt)
				{
					// forceful approach
					// free(queue_family_properties);
					// free(surface_formats);
					// continue;
				
					// sane approach
					m_surfaceFormatUsed = surface_formats[0];
				}
			}

			free(surface_formats);

			// We want as present mode, ie how images in the swapchain are managed during image
			// swap, mailbox presentation mode, which means that 1) swapchain will keep the most 
			// recent image and throw away the older ones 2) images are swapped during the vertical 
			// blank interval only
			uint32_t present_mode_cnt;
			vkGetPhysicalDeviceSurfacePresentModesKHR(phy_devices[i], m_surface, &present_mode_cnt, nullptr);
			if (present_mode_cnt == 0)
			{
				free(queue_family_properties);
				continue;
			}
	
			auto surface_present_modes = static_cast<VkPresentModeKHR*>(malloc(present_mode_cnt * sizeof(VkPresentModeKHR)));
			if (!surface_present_modes)
			{
				free(queue_family_properties);
				free(phy_devices);
				return APP_MEMORY_ERR;
			}
			
			vkGetPhysicalDeviceSurfacePresentModesKHR(phy_devices[i], m_surface, &present_mode_cnt, surface_present_modes);
			for (uint32_t i = 0u; i < present_mode_cnt; ++i)
			{
				if (surface_present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR);
				{
					m_presentModeUsed = VK_PRESENT_MODE_MAILBOX_KHR;
					break;
				}
			}
			// if loop finishes and m_presentModeUsed is not set, its default value (set by the renderer class constructor) is VK_PRESENT_MODE_FIFO_KHR
		
			free(surface_present_modes);

			// cleanup and assignment
			free(queue_family_properties);
			m_phyDevice = phy_devices[i];

			m_progressStatus |= PHY_DEVICE_GOT;
			printf("got physical device!\n");
			break;
		}

		free(phy_devices);
		return APP_SUCCESS;
	}

	auto renderer::setupDeviceAndQueues() & -> status_t
	{
		assert(m_progressStatus & PHY_DEVICE_GOT);
		// we can also map a device to multiple physical devices, a "device group", physical devices that can access each other's memory. I'don't care
		
		// TODO it can be refactored in a struct of arrays to be more readable
		// allocate necessary memory
		// for each queue family (of which we'll save 1 queue each), store index, priority and device queue create info
		auto queue_buffer = static_cast<unsigned char*>(malloc(MXC_RENDERER_QUEUES_COUNT * (sizeof(uint32_t) + sizeof(float) + sizeof(VkDeviceQueueCreateInfo))));
		if (!queue_buffer)
		{
			return APP_MEMORY_ERR;
		}

		// "QUEUES" need to be setup BEFORE setting up the device, as queues are "CREATED ALONGSIDE THE DEVICE"
		/* for each DISTICT queue we need to create its associated VkDeviceQueueCreateInfo, "BUT", a queue having presentation feature can be the same queue
		 * we will be using for graphics operation. Therefore we need to create a set of queue indices
		 * "IMPORTANT NOTE" TODO we might need again this set of queue indices. If that is the case, move this code
		 */
		uint32_t* queue_unique_indices = reinterpret_cast<uint32_t*>(queue_buffer);
		uint32_t queue_unique_indices_cnt = 1u;
		queue_unique_indices[0] = m_queueIdxArr[0];
		for (uint32_t i = 1u; i < MXC_RENDERER_QUEUES_COUNT; ++i)
		{
			uint32_t j = 0u;
			for (; j < queue_unique_indices_cnt; ++j)
			{
				if (m_queueIdxArr[i] == queue_unique_indices[j])
				{
					break;
				}
			}

			if (j == queue_unique_indices_cnt)
			{
				queue_unique_indices[j] = m_queueIdxArr[i];
				++queue_unique_indices_cnt;
			}
		}

		// each queue must have a "priority", between 0.0 and 1.0. the higher the priority the more
		// important the queue is, and such information MIGHT be used by the implementation to eg.
		// schedule the queues, give processing time to them. Vulkan makes NO GUARANTEES
		float* queue_priorities = reinterpret_cast<float*>(queue_unique_indices + queue_unique_indices_cnt);
		for (uint32_t i = 0u; i < queue_unique_indices_cnt; ++i)
		{
			// TODO i could have just hardcoded 1.0f as priority, but allocating unique values makes it more future proof?
			queue_priorities[i] = 1.0f;
		}
		
		// for each unique queue we need a VkDeviceQueueCreateInfo
		auto dev_queues_create_infos = reinterpret_cast<VkDeviceQueueCreateInfo*>(queue_buffer + queue_unique_indices_cnt * (sizeof(uint32_t) + sizeof(float))); // TODO change to std::byte. Note to self: allocate bytes instead of void*
		for (uint32_t i = 0u; i < queue_unique_indices_cnt; ++i)
		{
			dev_queues_create_infos[i] = {
				.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
				.pNext = nullptr, // TODO ?
				.flags = 0, // it has just the value VK_DEVICE_QUEUE_CREATE_PROTECTED_BIT, idk what is it, type is an enum called VkDeviceQueueCreateFlags
				.queueFamilyIndex = queue_unique_indices[i],
				.queueCount = 1, // every queue family supports at least 1 queue. For more than one, you check the Physical device queue family properties TODO
				.pQueuePriorities = queue_priorities 
			};
		}

		// TODO this is to refactor and move to physical device selection code
		// specify needed extensions
		uint32_t constexpr dev_exts_cnt = 1;
		char const* dev_exts[dev_exts_cnt] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME}; // TODO here you specify externally needed device extensions. export this. init will take params
		bool dev_exts_checked[dev_exts_cnt] = {0}; // zero initialized

		// then check against actual support of needed extensions for the device
		uint32_t supported_dev_exts_cnt;
		vkEnumerateDeviceExtensionProperties(m_phyDevice, nullptr, &supported_dev_exts_cnt, nullptr);

		auto supported_dev_exts = static_cast<VkExtensionProperties*>(malloc(supported_dev_exts_cnt * sizeof(VkExtensionProperties)));
		if (!supported_dev_exts)
		{
			free(queue_buffer);
			return APP_MEMORY_ERR;
		}

		vkEnumerateDeviceExtensionProperties(m_phyDevice, nullptr, &supported_dev_exts_cnt, supported_dev_exts);

		for (uint32_t i = 0u; i < supported_dev_exts_cnt; ++i)
		{
			for (uint32_t j = 0u; j < dev_exts_cnt; ++j)
			{
				if (0 == strcmp(supported_dev_exts[i].extensionName, dev_exts[j]))
				{
					dev_exts_checked[j] = true;
				}
			}
		}

		free(supported_dev_exts);

		{
			uint32_t i = 0;
			for (; i < dev_exts_cnt && dev_exts_checked[i]; ++i);
	
			if (i != dev_exts_cnt)
			{
				free(queue_buffer);
				return APP_GENERIC_ERR;
			}
		}

		VkDeviceCreateInfo const device_create_info = {
			.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
			.pNext = nullptr, // TODO ?
			.queueCreateInfoCount = queue_unique_indices_cnt,
			.pQueueCreateInfos = dev_queues_create_infos, 
			.enabledExtensionCount = dev_exts_cnt, // TODO ?
			.ppEnabledExtensionNames = dev_exts,
			.pEnabledFeatures = nullptr // feature specification is OPTIONAL, VkPhysicalDeviceFeatures* TODO add---------------------------------------------------------- init params
		}; 

		if (vkCreateDevice(m_phyDevice, &device_create_info, /*VkAllocationCallbacks*/nullptr, &m_device) != VK_SUCCESS)
		{
			free(queue_buffer);
			return APP_DEVICE_CREATION_ERR;
		}

		// TODO VK_EXT_device_memory_report, in 4.2 in vkspec
		
		// if anything goes wrong with the device when executing a command, it can become "lost", to query with "VK_ERROR_DEVICE_LOST", which is a return
		// value of some operations using the device. When a device is lost, "its child objects (anything created/allocated whose creation function has as"
		// "first param the device), are not destroyed"
		
		m_progressStatus |= DEVICE_CREATED;
		printf("created device!\n");

		/* ~Queues store~ */
		for (uint32_t i = 0u; i < MXC_RENDERER_QUEUES_COUNT; ++i)
		{
			vkGetDeviceQueue(m_device, 
							 m_queueIdx.graphics, 
							 /*queue index within the family*/0u, // TODO change when supporting more than 1 queue within a queue family
							 &m_queues[i]);
		}

		free(queue_buffer);
		return APP_SUCCESS;
	}

	// TODO refactor
	auto renderer::setupSurfaceKHR(GLFWwindow* window) & -> status_t
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
	auto renderer::setupSwapchain() & -> status_t
	{
		// m_surfaceCapabilities.currentExtent, needed for swapchain creation, can contain either 
		// the current width and height of the surface or the special value {0xfffffffff,0xffffffff), indicating
		// that I can choose image size, which in such case will be the window size 
		VkExtent2D const extent_used = m_surfaceCapabilities.currentExtent.width == 0xffffffff 
				? m_surfaceCapabilities.currentExtent 
				: VkExtent2D{.width=WINDOW_WIDTH, .height=WINDOW_HEIGHT};

		// if the graphics and presentation queue (TODO might change as these change) belong to the 
		// same queue family, or not. the swapchain needs to know about how many and which queue families it will
		// deal with. TODO to refactor out when structure of family queue indices will change. in particular, if and when we decide to use more than one queue per family this has to change
		uint32_t const queue_family_cnt = 1u + (m_queueIdx.graphics == m_queueIdx.presentation);
		uint32_t const queue_family_idxs[2] = {static_cast<uint32_t>(m_queueIdx.graphics), static_cast<uint32_t>(m_queueIdx.presentation)};
		
		// swapchain requires we specify if the image is going to be accessed by one queue at a time (sharing mode = exclusive) or by more than one (sharing mode = concurrent).
		// this depends on whether or not the presentation queue and the graphics queue are one and the same.
		VkSharingMode imageSharingMode = queue_family_cnt == 1 ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT;

		VkSwapchainCreateInfoKHR const swapchain_create_info = {
			.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
			.pNext = nullptr,
			.flags = 0u, 
			.surface = m_surface,
			.minImageCount = m_surfaceCapabilities.minImageCount + 1, // increment by one to allow for TRIBLE BUFFERING
			.imageFormat = m_surfaceFormatUsed.format,
			.imageColorSpace = m_surfaceFormatUsed.colorSpace,
			.imageExtent = extent_used,
			.imageArrayLayers = 1, // number of views in a multiview surface. >1 only for stereoscopic apps, like VR
			.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, // bitfield expressing in how many ways an image will be used. TODO for now only colour
			.imageSharingMode = imageSharingMode, // either EXCLUSIVE or CONCURRENT, first means that an image will be exclusive to family queues, other concurrent, we have only one graphics family queue for now TODO
			.queueFamilyIndexCount = queue_family_cnt,
			.pQueueFamilyIndices = queue_family_idxs,
			.preTransform = m_surfaceCapabilities.currentTransform, // transform applied before presentation to image. will use current transform given by surface, which means do nothing
			.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR, // type VkCompositeAlphaFlagBitsKHR, indicates which alpha compositing mode to use after fragment shader
			.presentMode = m_presentModeUsed,
			.clipped = VK_FALSE, // specifies whether to not execute fragment shader on not visible pixels due to other windows on top of the app or window partially out of display bounds (if set to VK_TRUE). NOTE TODO DO NOT SET IT TO TRUE IF YOU WILL FURTHER PROCESS THE CONTENT OF THE FRAMEBUFFER
			.oldSwapchain = VK_NULL_HANDLE // special Vulkan object handle value specifying an invalid object
		};

		VkResult res = vkCreateSwapchainKHR(m_device, &swapchain_create_info, /*vkAllocationCallbacks**/nullptr, &m_swapchain);
		if (res != VK_SUCCESS)
		{
			return APP_SWAPCHAIN_CREATION_ERR;
		}

		m_progressStatus |= SWAPCHAIN_CREATED;
		printf("swapchain created!\n");

		// get images from swapchain and create associated image views
		vkGetSwapchainImagesKHR(m_device, m_swapchain, &m_swapchainImagesCnt, nullptr);
		auto image_buffer = static_cast<unsigned char*>(malloc(m_swapchainImagesCnt * (sizeof(VkImage)+sizeof(VkImageView))));
		if (!image_buffer)
		{
			return APP_MEMORY_ERR;
		}

		m_pSwapchainImages = reinterpret_cast<VkImage*>(image_buffer);
		vkGetSwapchainImagesKHR(m_device, m_swapchain, &m_swapchainImagesCnt, m_pSwapchainImages);
		
		// TODO refactor image view creation
		m_pSwapchainImageViews = reinterpret_cast<VkImageView*>(m_pSwapchainImages + m_swapchainImagesCnt);
		for (uint32_t i = 0u; i < m_swapchainImagesCnt; ++i)
		{
			VkImageViewCreateInfo const image_view_create_info = {
				.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
				.pNext = nullptr, // TODO what
				.flags = static_cast<VkImageViewCreateFlagBits>(0),
				.image = m_pSwapchainImages[i],
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

			VkResult const res = vkCreateImageView(m_device, &image_view_create_info, /*VkAllocationCallbacks**/nullptr, &m_pSwapchainImageViews[i]);
			if (res != VK_SUCCESS)
			{
				free(image_buffer);
				return APP_GENERIC_ERR;
			}
		}

		m_surfaceExtent.width = m_surfaceCapabilities.currentExtent.width != 0xffffffff ? m_surfaceCapabilities.currentExtent.width : WINDOW_WIDTH, // TODO remove this is duplicated code
		m_surfaceExtent.height = m_surfaceCapabilities.currentExtent.height != 0xffffffff ? m_surfaceCapabilities.currentExtent.height : WINDOW_HEIGHT,
		
		m_progressStatus |= SWAPCHAIN_IMAGE_VIEWS_CREATED;
		return APP_SUCCESS;
	}

	auto renderer::setupCommandBuffers() & -> status_t
	{
		assert(m_progressStatus & DEVICE_CREATED && "command pools are child objects of devices, hence we need a device\n");

		// create command buffer pool
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

		// allocate command buffer (if fails destroy command pool)
		VkCommandBufferAllocateInfo const graphicsCmdBufAllocInfo{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.pNext = nullptr,
			.commandPool = m_graphicsCmdPool,
			.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY, // VkCommandBufferLevel, can either be PRIMARY=surface level commands, executed by queues or SECONDARY = can be called by primary/secondary command buffers, a bit like functions
			.commandBufferCount = m_swapchainImagesCnt
		};

		m_graphicsCmdBufs.resize(m_swapchainImagesCnt);

		res = vkAllocateCommandBuffers(m_device, &graphicsCmdBufAllocInfo, m_graphicsCmdBufs.data());
		if (res != VK_SUCCESS)
		{
			vkDestroyCommandPool(m_device, m_graphicsCmdPool, /*VkAllocationCallbacks**/nullptr);
			fprintf(stderr, "failed to allocate a command buffer to the graphics command pool!\n");
			return APP_VK_ALLOCATION_ERR;
		}

		m_progressStatus |= COMMAND_BUFFER_ALLOCATED;
		printf("created command pool and allocated a resettable and transient command pool");
		return APP_SUCCESS;
	}

	//auto renderer::setupDepthImage() & -> status_t
	//{
	//	assert(m_progressStatus & DEVICE_CREATED);

	//	uint32_t graphicsIdxsUnsigned[MXC_RENDERER_GRAPHICS_QUEUES_COUNT] = {static_cast<uint32_t>(m_queueIdx.graphics)}; // TODO change, not futureproof

	//	// create depth image
	//	VkImageCreateInfo const depthImgCreateInfo {
	//		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
	//		.pNext = nullptr,
	//		.flags = 0,// specifies additional properties of the image. BE AWARE THAT MANY THINGS, such as 2darray, cubemap, sparce memory, multisampling, ... require a flag
	//		.imageType = VK_IMAGE_TYPE_2D,// number of dimensions
	//		.format = m_depthImageFormat,
	//		.extent = VkExtent3D{
	//				.width = m_surfaceCapabilities.currentExtent.width != 0xffffffff ? m_surfaceCapabilities.currentExtent.width : WINDOW_WIDTH,
	//				.height = m_surfaceCapabilities.currentExtent.height != 0xffffffff ? m_surfaceCapabilities.currentExtent.height : WINDOW_HEIGHT,
	//				.depth = 0
	//		},
	//		.mipLevels = 1, // numbers of levels of detail 
	//		.arrayLayers = 1, // numbers of layers in the image (Photoshop sense)
	//		.samples = VK_SAMPLE_COUNT_1_BIT,//VkSampleCountFlagBits
	//		.tiling = VK_IMAGE_TILING_OPTIMAL, // how image is laid out in memory, between optimal, linear, drm(requires extension, linux only)
	//		.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
	//		.sharingMode = VK_SHARING_MODE_EXCLUSIVE, // either exclusive or concurrent
	//		.queueFamilyIndexCount = MXC_RENDERER_GRAPHICS_QUEUES_COUNT,
	//		.pQueueFamilyIndices = graphicsIdxsUnsigned, // assuming device and queues have been setup
	//		.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
	//	};

	//	VkResult res = vkCreateImage(m_device, &depthImgCreateInfo, /*VkAllocationCallbacks**/nullptr, &m_depthImage);
	//	if (res != VK_SUCCESS)
	//	{
	//		fprintf(stderr, "failed to create depth image!\n");
	//		return APP_GENERIC_ERR;
	//	}

	//	// create depth image view
	//	// they are non dispatchable handles NOT accessed by shaders but represent a range in an image with associated metadata
	//	// special values useful for creation = VK_REMAINING_ARRAY_LAYERS, VK_REMAINING_MIP_LEVELS
	//	VkImageViewCreateInfo const depthImgViewCreateInfo {
	//		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
	//		.pNext = nullptr,
	//		.flags = 0,// there is one to specify that view will be read during fragment density stage? and one for reading the view in CPU at the end of the command buffer execution
	//		.image = m_depthImage,
	//		.viewType = VK_IMAGE_VIEW_TYPE_2D,// dimentionality and type of view, must be "less then or equal" to image
	//		.format = m_depthImageFormat,
	//		.components = VkComponentMapping{// VkComponentMapping, all swizzle identities
	//			.r = VK_COMPONENT_SWIZZLE_IDENTITY,
	//			.g = VK_COMPONENT_SWIZZLE_IDENTITY,
	//			.b = VK_COMPONENT_SWIZZLE_IDENTITY,
	//			.a = VK_COMPONENT_SWIZZLE_IDENTITY
	//		},
	//		.subresourceRange = VkImageSubresourceRange{// specifies subrange accessible from view
	//			.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
	//			.baseMipLevel = 0,
	//			.levelCount = VK_REMAINING_MIP_LEVELS,
	//			.baseArrayLayer = 0,
	//			.layerCount = VK_REMAINING_ARRAY_LAYERS
	//		}
	//	};

	//	res = vkCreateImageView(m_device, &depthImgViewCreateInfo, /*VkAllocationCallbacks**/nullptr, &m_depthImageView);
	//	if (res != VK_SUCCESS)
	//	{
	//		vkDestroyImage(m_device, m_depthImage, /*VkAllocationCallbacks**/nullptr);
	//		fprintf(stderr, "failed to create depth image view!\n");
	//		return APP_GENERIC_ERR;
	//	}

	//	m_progressStatus |= DEPTH_IMAGE_CREATED;
	//	return APP_SUCCESS;
	//}

	//auto renderer::setupDepthDeviceMemory() & -> status_t
	//{
	//	assert((m_progressStatus & (DEVICE_CREATED | DEPTH_IMAGE_CREATED)) && "VkDevice required to allocate VkDeviceMemory!\n");

	//	// get image memory requirements for depth image
	//	VkMemoryRequirements depthImageMemoryRequirements; // size, alignment, memory type bits
	//	vkGetImageMemoryRequirements(m_device, m_depthImage, &depthImageMemoryRequirements);

	//	// allocate memory
	//	VkPhysicalDeviceMemoryProperties depthImageMemoryProperties; // typecount, types, heapcount, heaps
	//	vkGetPhysicalDeviceMemoryProperties(m_phyDevice, &depthImageMemoryProperties);

	//	// -- for each memory type present in the device, find one that matches our memory requirement
	//	uint32_t i = 0u;
	//	for (; i < depthImageMemoryProperties.memoryTypeCount; ++i)
	//	{
	//		// VkMemoryRequirements::memoryTypeBits is a bitfield (uint32) whose bits are set to one if the corresponding memory type index(==power of 2) can be used by the resource
	//		// so we will: 1) check if current memory type index can be used by resource
	//		// 			   2) check if the memory type's properties are suitable to OUR needs
	//		if ( depthImageMemoryRequirements.memoryTypeBits & (1 << i) // check if i-th bit is a type usable by resource
	//		 	 && depthImageMemoryProperties.memoryTypes[i].propertyFlags & (VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)) // check if type's properties match our needs
	//		{
	//			break;
	//		}
	//	}
	//	if (i == depthImageMemoryProperties.memoryTypeCount)
	//	{
	//		fprintf(stderr, "could not find any suitable memory types to allocate memory for depth image!\n");
	//		return APP_GENERIC_ERR;
	//	}

	//	VkMemoryAllocateInfo const allocateInfo {
	//		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
	//		.pNext = nullptr, // There are many extentions, most of them even platform-specific
	//		.allocationSize = depthImageMemoryRequirements.size,
	//		.memoryTypeIndex = i // type of memory required, must be supported by device
	//	};
	//	VkResult const res = vkAllocateMemory(m_device, &allocateInfo, /*VkAllocationCallbacks**/nullptr, &m_depthImageMemory);

	//	// bind allocated memory to depth image
	//	vkBindImageMemory(m_device, m_depthImage, m_depthImageMemory, 0); // last parameter is offset TODO this means that in one memory heap we can allocate more than one image!

	//	m_progressStatus |= DEPTH_MEMORY_ALLOCATED;
	//	printf("allocated device memory for depth buffer!\n");
	//	return APP_SUCCESS;
	//}

	auto renderer::setupRenderPass() & -> status_t
	{
		// render pass output attachment descriptions array (for the render pass, we will also need attachment references for the subpasses, which are handles decorated with some data to this array)
		VkAttachmentDescription const outAttachmentDescriptions[MXC_RENDERER_ATTACHMENT_COUNT] {
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
			//{
			//	.flags = 0,
			//    // VK_FORMAT_D32_SFLOAT: 32-bit float for depth
			//    // VK_FORMAT_D32_SFLOAT_S8_UINT: 32-bit signed float for depth and 8 bit stencil component
			//    // VK_FORMAT_D24_UNORM_S8_UINT: 24-bit float for depth and 8 bit stencil component
			//	.format = m_depthImageFormat, // no need to check for support of such a format, as the 3 of them are MANDATORY BY THE VULKAN SPECIFICATION. see setupPhysical device and TODO refactor
			//	.samples = VK_SAMPLE_COUNT_1_BIT,
			//	.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR, //(VkAttachmentLoadOp)
			//	.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			//	.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			//	.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			//	.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			//	.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
			//}
		};

		// output attachment references
		VkAttachmentReference const outAttachmentRefs[MXC_RENDERER_ATTACHMENT_COUNT] {
			// color attachment ref
			{
				.attachment = 0, // index of the attachment used as stored in the array of attachment descriptions
				.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL// VkImageLayout, specifies the layout of the attachment used during the subpass
			},
			// depth attachment ref
			//{
			//	.attachment = 1,
			//	.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
			//}
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

		// subpass dependency creation
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

		// render pass creation
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

	auto renderer::setupFramebuffers() & -> status_t
	{
		assert(m_progressStatus & RENDERPASS_CREATED);

		m_framebuffers.resize(m_swapchainImagesCnt);
		// create vulkan non dispachable handles VkFramebuffers
		VkImageView usedAttachments[MXC_RENDERER_ATTACHMENT_COUNT] { m_pSwapchainImageViews[0]/*, m_depthImageView*/}; // we have 2 attachments per swapchain image, 1st is color, 2nd depth. Depth doesn't change

		VkFramebufferCreateInfo framebufferCreateInfo {
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.pNext = nullptr, // TODO look into that
			.flags = 0, // there is, as of now, 1 flag only, to create an "imageless" framebuffer
			.renderPass = m_renderPass,
			.attachmentCount = MXC_RENDERER_ATTACHMENT_COUNT,
			.pAttachments = usedAttachments, // as it's a pointer, all I have to do to create 3 different framebuffers is to change the color output attachment in the loop
			.width = m_surfaceCapabilities.currentExtent.width != 0xffffffff ? m_surfaceCapabilities.currentExtent.width : WINDOW_WIDTH, // TODO remove this is duplicated code
			.height = m_surfaceCapabilities.currentExtent.height != 0xffffffff ? m_surfaceCapabilities.currentExtent.height : WINDOW_HEIGHT,
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

			usedAttachments[0] = m_pSwapchainImageViews[i+1]; // will go out of bounds in last iteration, but won't be read
		}

		printf("%u framebuffers created!\n", m_framebuffers.size());
		m_progressStatus |= FRAMEBUFFERS_CREATED;
		return APP_SUCCESS;
	}

	auto renderer::setupGraphicsPipeline() & -> status_t
	{
		assert(m_progressStatus & (FRAMEBUFFERS_CREATED | RENDERPASS_CREATED) && "graphics pipeline creation requires a renderpass and framebuffers!\n");

		// creation of all information about pipeline steps: layout, and then in order of execution
		// -- pipeline layout TODO update when adding descriptor sets
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
		m_progressStatus |= GRAPHICS_PIPELINE_LAYOUT_CREATED;

		// -- VkPipelineShaderStageCreateInfo describes the shaders to use in the graphics pipeline TODO number of stages hardcoded
		// ---- creation of the shader modules
		// ------ evaluate file sizes
		namespace fs = std::filesystem;
		fs::path const relativeShaderPaths[MXC_RENDERER_SHADERS_COUNT] {"shaders/triangle.vert.spv","shaders/triangle.frag.spv"};// TODO hardcode of shaders numbers
		fs::path const shaderPaths[MXC_RENDERER_SHADERS_COUNT] = {fs::current_path() / relativeShaderPaths[0], fs::current_path() / relativeShaderPaths[1]}; 
		std::error_code ecs[MXC_RENDERER_SHADERS_COUNT];
		size_t const shaderSizes[MXC_RENDERER_SHADERS_COUNT] {fs::file_size(shaderPaths[0], ecs[0]), fs::file_size(shaderPaths[1], ecs[1])}; // TODO check the error code, whose codes themselves are platform specific
		
		// ------ allocate char buffers to store shader binary data
	#define APP_USE_STRING_DEBUG
	#ifndef APP_USE_STRING_DEBUG
		auto shadersBuf = static_cast<char*>(malloc(shaderSizes[0] + shaderSizes[1] + 2/*NUL*/));
		if (!shadersBuf)
		{
			return APP_MEMORY_ERR;
		}
		std::ifstream shaderStreams[MXC_RENDERER_SHADERS_COUNT] {std::ifstream(shaderPaths[0]), std::ifstream(shaderPaths[1])};
		shaderStreams[0].read(shadersBuf, shaderSizes[0]);
		shadersBuf[shaderSizes[0]] = '\0';
		shaderStreams[1].read(shadersBuf+shaderSizes[0]+1, shaderSizes[1]);
		shadersBuf[shaderSizes[0]+shaderSizes[1]+1] = '\0';

		printf("%s\n", shadersBuf);
		printf("%s\n", shadersBuf+shaderSizes[0]+1);

		VkShaderModuleCreateInfo const shaderModuleCreateInfos[MXC_RENDERER_SHADERS_COUNT] {
			{ // Vertex Shader Module Create Info
				.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
				.pNext = nullptr,
				.flags = 0,
				.codeSize = shaderSizes[0],
				.pCode = reinterpret_cast<uint32_t*>(shadersBuf)
			},
			{ // Fragment Shader Module Create Info
				.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
				.pNext = nullptr,
				.flags = 0,
				.codeSize = shaderSizes[1],
				.pCode = reinterpret_cast<uint32_t*>(shadersBuf+shaderSizes[0]+1)
			}
		};

		VkShaderModule shaders[MXC_RENDERER_SHADERS_COUNT] = {VK_NULL_HANDLE};
		if ( vkCreateShaderModule(m_device, &shaderModuleCreateInfos[0],/*VkAllocationCallbacks**/nullptr, &shaders[0]) != VK_SUCCESS 
			|| vkCreateShaderModule(m_device, &shaderModuleCreateInfos[1], /*VkAllocationCallbacks**/nullptr, &shaders[1]) != VK_SUCCESS)
		{
			free(shadersBuf);
			vkDestroyPipelineLayout(m_device, m_graphicsPipelineLayout, /*VkAllocationCallbacks**/nullptr);
			return APP_GENERIC_ERR;
		}
	#else
		std::vector<char> shadersBuf[2];
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
	#endif
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

		// -- VkPipelineVertexInputStateCreateInfo describes the vertices to be passed, without actually passing the data (number, stride, buffer to data). Ignored if Mesh shader is used
		// ---- Creation of VkVertexAttributeDescriptions(in which bindings is the vertex data distributed ?) and VkVertexInputAttributeDescription(in which layout is each vertex in the bindings formatted?)
		VkVertexInputBindingDescription const vertInputBindingDescriptions[] {
			{
				.binding = 0, // Binding number which this structure is describing. You need a description for each binding in use. TODO we have no binding now
				.stride = 0, // distance between successive elements in bytes (if attributes are stored interleaved, then stride = sizeof(Vertex))
				.inputRate = VK_VERTEX_INPUT_RATE_VERTEX // VkVertexInputRate specifies whether attributes stored in the buffer are PER VERTEX or PER INSTANCE
			}
		};

		VkVertexInputAttributeDescription const vertInputAttributeDescriptions[] { // TODO for now I have no attributes but just setting up the skeleton for future use
			{
				.location = 0,
				.binding = 0,
				.format = VK_FORMAT_R32G32B32_SFLOAT, // VkFormat
				.offset = 0 // in bytes, from the start of the current element
			}
		};

		VkPipelineVertexInputStateCreateInfo const graphicsPipelineVertexInputStateCI {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.vertexBindingDescriptionCount = 0, // TODO change this when you add some vertex attributes
			.pVertexBindingDescriptions = vertInputBindingDescriptions,
			.vertexAttributeDescriptionCount = 0,
			.pVertexAttributeDescriptions = vertInputAttributeDescriptions
		};

		// -- VkPipelineInputAssemblyStateCreateInfo specifies behaviour of the input assembly stage, i.e. vertex attributes, topology,...
		// with topology triangle list we specify each triangle separately. This means duplicate data for adjacent triangles, but it is the easier to start with. others include point list/strip, line list/strip, triangle list/strip/fan, (these 3,except fan, with or without adjacency(which means that some verts specifies an ADJACENCY EDGE, an edge not displayed whose points are accessible only to geo shader. purpose=primitive processing)), or patch list(see bezier curves, vertices are control points)
		VkPipelineInputAssemblyStateCreateInfo const graphicsPipelineInputAssemblyStateCI {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,// chooses a PRIMITIVE TOPOLOGY, ie how consequent vertices are arranged in primitives during input assembly and kept up to the rasterization stage(if not altered by tesselation and/or geometry shader). In case of mesh shader, it is the latter that defines the topology used.
			.primitiveRestartEnable = VK_FALSE // allows restart of topology (there are some topologies which can take as many verts and create a continuous mesh), and it will do that if, DURING INDEXED DRAWS(ONLY), the special value 0xfff...(number of f's depends on index type). Discards vertices of incomplete primitive (eg first of the 2 verts required to continue triangle strip)
		};

		// -- VkPipelineTessellationStateCreateInfo specifies the tessellation state used by the tessellation shaders, TODO future
		VkPipelineTessellationStateCreateInfo const graphicsPipelineTessellationStateCI {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO,
			.pNext = nullptr, // NULL or to an instance of domain origin state create info
			.flags = 0,
			.patchControlPoints = 0 // number of control points per patch. TODO we are not going to use tessellation for now
		};

		// -- VkPipelineViewportStateCreateInfo defines the window viewport, i.e. an rectangle+a set of scissors. NUM. SCISSORS = NUM. VIEWPORTS < VkPhysicalDeviceLimits::maxViewports
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

		VkPipelineViewportStateCreateInfo const graphicsPipelineViewportStateCI {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
			.pNext = nullptr, 
			.flags = 0,
			.viewportCount = 1, // greater than 1 requires multiviewport extension
			.pViewports = &viewport,
			.scissorCount = 1,
			.pScissors = &scissor
		};

		// -- VkPipelineRasterizationStateCreateInfo specifies how the vertices will be rasterized, and stores the rasterization state
		VkPipelineRasterizationStateCreateInfo const graphicsPipelineRasterizationStateCI {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
			.pNext = nullptr, // lots of extensions here
			.flags = 0,
			.depthClampEnable = VK_FALSE, // whether or not to enable depth clamp, i.e. anything with depth resulting from the depth test > farplane(1)
			.rasterizerDiscardEnable = VK_FALSE, // are primitives discarded just before rasterization (no image produced), used only if you want to achieve some computation on vertex data, e.g. physics or animation
			.polygonMode = VK_POLYGON_MODE_FILL, // TRIANGLE RENDERING MODE, this has nothing to do with the primitive topology, which defines how we want to GROUP vertex data to process and IDENTIFY SEPARATE OBJECTS. Here, we want to define if we want to color the points of each triangle, lines, or fill them completely
			.cullMode = VK_CULL_MODE_BACK_BIT,// which face/faces of triangle to hide and which to display, relates to TRIANGLE WINDING, we want to setup the CCW orientation as front face, and cull the back face
			.frontFace = VK_FRONT_FACE_CLOCKWISE, // TODO introduce transforms and swap this back to counter clockwise winding for front face
			.depthBiasEnable = VK_FALSE, // TODO later, when doing shadow maps
			.depthBiasConstantFactor = 0.f,
			.depthBiasClamp = 0.f,
			.depthBiasSlopeFactor = 0.f,
			.lineWidth = 1.f,
		};

		// -- VkPipelineMultisampleStateCreateInfo structure defining multisampling state when multisampling is used
		VkPipelineMultisampleStateCreateInfo const graphicsPipelineMultisampleStateCI {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT, // number of samples used in the rasterization
			.sampleShadingEnable = VK_FALSE, // see sample shading TODO later
			.minSampleShading = 0.f, // disabled if no sample shading
			.pSampleMask = nullptr, // disabled 
			.alphaToCoverageEnable = VK_FALSE,
			.alphaToOneEnable = VK_FALSE // see multisample coverage
		};

		// -- VkPipelineDepthStencilCreateInfo specifies depth/stencil state when access, rasterization and render of depth/stencil buffers are enabled
		VkPipelineDepthStencilStateCreateInfo const graphicsPipelineDepthStencilStateCI {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0, // extension order attachment access? TODO 
			.depthTestEnable = VK_FALSE,// VK_TRUE,
			.depthWriteEnable = VK_FALSE, //VK_TRUE,
			.depthCompareOp = VK_COMPARE_OP_LESS,
			.depthBoundsTestEnable = VK_FALSE, // enables DEPTH BOUND TEST, requires depth test. You specify a range with min and max, and any depth value outside that range will fail the test 
			.stencilTestEnable = VK_FALSE, // TODO enable stencil test
			.front = VkStencilOpState{},
			.back = VkStencilOpState{},
			.minDepthBounds = 0.f,
			.maxDepthBounds = 0.f
		};

		// -- VkPipelineColorBlendStateCreateInfo specifies how color outputs of the fragment stage will be blended and manages the blend state. All of this requires of course a color attachment
		// Blending is only defined for floating-point, UNORM, SNORM, and sRGB formats. Within those formats, the implementation may only support blending on some subset of them. Which formats support blending is indicated by
		// VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BLEND_BIT
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

		VkPipelineColorBlendStateCreateInfo const graphicsPipelineColorBlendStateCI {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0, // extension rasterization order attachment access
			.logicOpEnable = VK_FALSE, // enables a logic operation instead of a mathematical operation to perform blending. No. we want linear interpolation alpha_dest * c_dest + (1-alpha_dest) * c_src, also requires logicOp feature
			.logicOp = VkLogicOp{},
			.attachmentCount = 1u, // WARNING POSSIBLE ERROR we will perform blending only on color
			.pAttachments = colorBlendAttachmentStates,
			.blendConstants = {} // array of constant values specifying factors for 3 color components + alpha 
		};

		// -- VkPipelineDynamicStateCreateInfo defines which properties of the pipeline state objects are dynamic and can be changed after the pipeline's creation. TODO set it up for resizeable screen. Can be null
		VkPipelineDynamicStateCreateInfo const graphicsPipelineDynamicStateCI {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
			.pNext = nullptr,// no exts here for now 
			.flags = 0, // no flags here for now
			.dynamicStateCount = 0, // TODO setup framebuffer/surface/viewport/scissor size to be dynamic so that the window is resizeable
			.pDynamicStates = nullptr
		};

		// finally assemble the graphics pipeline
		VkGraphicsPipelineCreateInfo const graphicsPipelineCreateInfo {
			.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
			.pNext = nullptr, // TODO you have no idea how many extension structures and flags there are
			.flags = 0,
			.stageCount = 2, // TODO change that
			.pStages = graphicsPipelineShaderStageCIs,
			.pVertexInputState = &graphicsPipelineVertexInputStateCI,
			.pInputAssemblyState = &graphicsPipelineInputAssemblyStateCI,
			.pTessellationState = &graphicsPipelineTessellationStateCI,
			.pViewportState = &graphicsPipelineViewportStateCI,
			.pRasterizationState = &graphicsPipelineRasterizationStateCI,
			.pMultisampleState = &graphicsPipelineMultisampleStateCI,
			.pDepthStencilState = &graphicsPipelineDepthStencilStateCI,
			.pColorBlendState = &graphicsPipelineColorBlendStateCI,
			.layout = m_graphicsPipelineLayout,
			.renderPass = m_renderPass,
			.subpass = 0, // subpass index in the renderpass. A pipeline will execute 1 subpass only.
			.basePipelineHandle = VK_NULL_HANDLE, // recycle old pipeline, requires PIPELINE_DERIVATIVES flag.
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

	#ifndef APP_USE_STRING_DEBUG
		free(shadersBuf);
	#endif
		shaderStreams[0].close();
		shaderStreams[1].close();
		for (uint32_t i = 0u; i < MXC_RENDERER_SHADERS_COUNT; ++i)
			vkDestroyShaderModule(m_device, shaders[i], /*VkAllocationCallbacks**/nullptr);
		m_progressStatus |= GRAPHICS_PIPELINE_CREATED;
		printf("pipeline created!\n");
		return APP_SUCCESS;
	}

	auto renderer::setupSynchronizationObjects() & -> status_t
	{
		assert((m_progressStatus & DEVICE_CREATED) && "device is required to create synchronization primitives!\n");
		
		m_fenceInFlightFrame.resize(m_swapchainImagesCnt);
		m_semaphoreImageAvailable.resize(m_swapchainImagesCnt);
		m_semaphoreRenderFinished.resize(m_swapchainImagesCnt);

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

		uint16_t created[m_swapchainImagesCnt] = {};
		VkResult res;
		for (uint32_t i = 0u; i < m_swapchainImagesCnt; ++i)
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

	auto renderer::recordCommands(uint32_t framebufferIdx) & -> status_t
	{
		assert(framebufferIdx < m_swapchainImagesCnt && "framebuffer index out of bounds");
		assert((m_progressStatus & (GRAPHICS_PIPELINE_CREATED | COMMAND_BUFFER_ALLOCATED)) && "command buffer recording requires a pipeline and a command buffer!\n");

		// should be called begin RECORDING, we are not executing any command here
		VkCommandBufferBeginInfo const cmdBufBeginInfo {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.pNext = nullptr, // can be pointer to vkgroupcommandbufferbegininfo if you setup a device group
			.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,// we have 3: one time submit = buffer will be reset after first use, render pass continue = for secondary cmd bufs only, specifies that this 2ndary cmd buf is entirely in renderpass, simultaneous use = allows to resubmit buffer to a queue while it is in the pending state, aka not finished
			.pInheritanceInfo = nullptr // used if this is a secondary buffer, defines the state that the secondary buffer will inherit from primary command buffer (render pass, subpass, framebuffer, and pipeline statistics)
		};
		vkBeginCommandBuffer(m_graphicsCmdBufs[framebufferIdx], &cmdBufBeginInfo); // TODO rework when command buffers become > 1
		{
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
			vkCmdBeginRenderPass(m_graphicsCmdBufs[framebufferIdx], &renderPassBeginInfo, /*VkSubpassContents*/VK_SUBPASS_CONTENTS_INLINE); // inline = no secondary buffers are executed in each subpass, while secondary means that subpass is recorded in a secondary command buffer
			{
				// bind graphics pipeline to render pass
				vkCmdBindPipeline(m_graphicsCmdBufs[framebufferIdx], /*VkPipelineBindPoint*/VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline); // bind point = type of pipeline to bind

				// TODO we didn't specify viewport and scissor to be dynamic for now, so no need to vkCmdSet them, but I'll come back

				// draw command
				vkCmdDraw(m_graphicsCmdBufs[framebufferIdx], /*vertexCount*/3, /*instance count*/1, /*firstVertexID*/0, /*firstInstanceID*/0); // vertex count == how many times to call the vertex shader != how many vertices we have stored in a buffer, instance count == number of times to draw the same primitives
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

	auto renderer::draw() & -> status_t
	{
		uint32_t static currentFramebuffer = 0u;

		// wait for the previous frame to finish, as otherwise we would continue to submit command buffers indefinitely without knowing if the GPU has finished the previous one or not
		vkWaitForFences(m_device, /*fenceCount*/1, &m_fenceInFlightFrame[currentFramebuffer], /*wait all or any*/VK_TRUE, /*timeout*/0xffffffffffffffff); // TODO swap that hex for std::numeric_limits
		//printf("TIME TO DRAW\n");
		// ok next frame incoming. close the fence so that no other draw submission can get through until this one has finished. We do this because fences are not automatically closed
		vkResetFences(m_device, /*fenceCount*/1, &m_fenceInFlightFrame[currentFramebuffer]);

		// then acquire next available image from the swapchain. To be safe that it is not being presented, we will wait on semaphoreImageAvailable
		uint32_t imageIdx;
		vkAcquireNextImageKHR(m_device, m_swapchain, /*timeout in ns*/0xffffffffffffffff, m_semaphoreImageAvailable[currentFramebuffer], /*fenceToSignal*/VK_NULL_HANDLE, &imageIdx);
		
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

		VkResult res = vkQueueSubmit(m_queues[0], /*submitCount*/1, &submitInfo, /*fenceToSignal*/m_fenceInFlightFrame[currentFramebuffer]);
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
		if (res != VK_SUCCESS)
		{
			fprintf(stderr, "failed to present image!\n");
			return APP_GENERIC_ERR;
		}

		//printf("currentFramebuffer = %u\n", currentFramebuffer);
		currentFramebuffer = (currentFramebuffer + 1) % m_swapchainImagesCnt;
		return APP_SUCCESS;
	}

	auto renderer::progress_incomplete() const & -> status_t 
	{
		status_t const status = m_progressStatus ^ (
			SYNCHRONIZATION_OBJECTS_CREATED |
			GRAPHICS_PIPELINE_CREATED | 
			GRAPHICS_PIPELINE_LAYOUT_CREATED |
			FRAMEBUFFERS_CREATED |
			//DEPTH_MEMORY_ALLOCATED |
			//DEPTH_IMAGE_CREATED |
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
		// TODO
		auto presentation_setup_step() -> status_t;
		// TODO
		auto resource_setup_step() -> status_t;
		// TODO
		auto pipeline_setup_step() -> status_t;
		// TODO
		auto descriptorsets_setup_step() -> status_t;
		// TODO
		auto spirv_shaders_step() -> status_t;
		// TODO
		auto record_commands_step() -> status_t;
		
		// application execution
		// NOTE: IT CONTROLS IF ALL USED BITS IN PROGRESS ARE SET, if you add some, change this
		auto run() -> status_t;
	public: // function utilities	
		auto progress_incomplete() -> status_t;

	private: // functions


	private: // data
		// main components
		GLFWwindow* m_window;
		renderer m_renderer;

	private: // utilities functions
	
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

	auto errorCallbackGLFW(int errCode, char const * errMsg) -> void
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
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
		
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
		uint32_t required_instance_glfwextension_count;
		char const** required_instance_glfwextensions = glfwGetRequiredInstanceExtensions(&required_instance_glfwextension_count);
		/**NOTE:
		 * if you don't need any additional extension other than those required by GLFW, then you can pass this pointer directly to ppEnabledExtensions in VkInstanceCreateInfo.
		 * Assuming we could need more in the future, I will store them as state by a dynamically allocated buffer in the renderer
		 */

		if (m_renderer.init(required_instance_glfwextension_count, required_instance_glfwextensions, m_window) == APP_GENERIC_ERR)
		{
			return APP_GENERIC_ERR;
		}

		// TODO add app initialized status when finish everything
		return APP_SUCCESS;
	}
		
	auto app::presentation_setup_step() -> status_t
	{
		status_t result = APP_GENERIC_ERR;
		// assuming the initialization_step was run, TODO add macros, then we can setup our VkSurface and swapchain 
		return result;
	}

	auto app::resource_setup_step() -> status_t 
	{
		return APP_GENERIC_ERR;	
	}

	auto app::pipeline_setup_step() -> status_t 
	{
		return APP_GENERIC_ERR;
	}
	
	auto app::descriptorsets_setup_step() -> status_t 
	{
		return APP_GENERIC_ERR;
	}
	
	auto app::spirv_shaders_step() -> status_t 
	{
		return APP_GENERIC_ERR;
	}
	
	auto app::record_commands_step() -> status_t 
	{
		return APP_GENERIC_ERR;
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
			if (APP_SUCCESS != m_renderer.draw())
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
