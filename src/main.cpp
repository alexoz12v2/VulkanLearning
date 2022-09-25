#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <cstddef>
#include <cstdint>
#include <cstdlib> // TODO check for possible removal
#include <cstdio> // TODO check its usage for possible removal in non debug builds
#include <cstring>
#include <cassert>

// TODO change naming convention from snake_case to camelCase for vars and PascalCase for types
// TODO register_extensions and setup_instance manage a lot of memory, and they should have allocated
// 		void* in the beginning as working buffers, instead of allocating on demand, therefore reducing
// 		number of allocations and casts needed
// TODO remove all std:: usage (except for platform abstraction and type support facilities). example: remove std::vector
// TODO setup macro for compiler specific restrict keyword. Eg. __restrict__ for g++/clang, __restrict for MSVC, and forceinline, attribute(force_inline) for g++/clang, and __force_inline__ for MSVC
// TODO add constexpr where fit
// TODO setup documentation before source grows too much
// TODO modify app class so that it can support having multiple layers
// TODO custom allocation
// TODO once you write a user-defined constructor, syntesized move constructor and move assignment operators are disabled by default. Write them

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
	//... more errors
	
	// TODO change return conventions into more meaningful and specific error carrying type to convey a more 
	// 		specific status report
	/*** WARNING: most of the functions will return APP_TRUE if successful, and APP_FALSE if unsuccessful ***/

	class renderer
	{
	public: // constructors
		renderer() : m_instance(nullptr), m_phy_device(nullptr), m_device(nullptr), 
					 m_queues({-1}), // TODO Don't forget to update when adding queue types
#ifndef NDEBUG //CMAKE_BUILD_TYPE=Debug
					 m_dbg_msger(nullptr),
#endif // ifndef NDEBUG	
					 m_progress_status(0u) {} 
		~renderer();
		
	public: // public functions, initialization procedures
		auto init() & -> status_t;
		auto setup_instance(uint32_t const ext_count, char const** ext_names) & -> status_t;
		auto setup_phy_device() & -> status_t;
		auto setup_device() & -> status_t;

	public: // public function, utilities
		auto progress_incomplete() const & -> status_t; // TODO: const correct and ref correct members
												  
	private: // data members, dispatchable vulkan objects handles
		VkInstance m_instance;
		VkPhysicalDevice m_phy_device;
		VkDevice m_device;

	private: // vulkan related
		// anonymous unions cannot possess anonymous structs
		struct queues_t
		{
			int32_t graphics;
		};
#define APP_RENDERER_QUEUES_COUNT 1 // "BE READY TO CHANGE IT"
		union 
		{
			int32_t m_queues[APP_RENDERER_QUEUES_COUNT];
			queues_t m_queue_idx;
		};

#ifndef NDEBUG // CMAKE_BUILD_TYPE=Debug
		VkDebugUtilsMessengerEXT m_dbg_msger;
#endif // ifndef NDEBUG

	private: // data members, utilities_queues.idx.graphics = 
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

		uint32_t m_progress_status; // BIT FIELD: {initialized?, instance created?, ...}
#ifndef NDEBUG // CMAKE_BUILD_TYPE=Debug
		#define MXC_RENDERER_PROGRESS_UNUSED_BITS 0x1fffffff
#elif
		#define MXC_RENDERER_PROGRESS_UNUSED_BITS 0x3fffffff
#endif // ifndef NDEBUG
		// the enum class is a fully fledged type. All we need are scoped aliases for numbers and to perform bitwise operations
		enum m_progress_t : uint32_t
		{
			INITIALIZED = 0x80000000,
#ifndef NDEBUG // CMAKE_BUILD_TYPE=Debug
			INSTANCE_CREATED = 0x40000000,
			MESSENGER_CREATED = 0x20000000,
#elif
			INSTANCE_CREATED = 0x40000000,
#endif // ifndef NDEBUG
			PHY_DEVICE_GOT = 0x10000000,
			DEVICE_CREATED = 0x08000000
		};
	
	private: // functions, utilities
	};

	auto renderer::init() & -> status_t
	{
	
		m_progress_status |= m_progress_t::INITIALIZED;
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
		m_registered_exts = static_cast<char**>(memory_needed + 32u * sizeof(char[VK_MAX_EXTENSION_NAME_SIZE]));
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
		// destruction performed by using the switch statement fallthrough
		switch (m_progress_status)
		{
#ifndef NDEBUG // CMAKE_BUILD_TYPE=Debug
			case DEVICE_CREATED | MESSENGER_CREATED | m_progress_t::INITIALIZED | m_progress_t::INSTANCE_CREATED:
				// all its child objects need to be destroyed before doing this
				vkDestroyDevice(m_device, /*VkAllocationCallbacks**/nullptr);
			case MESSENGER_CREATED | m_progress_t::INITIALIZED | m_progress_t::INSTANCE_CREATED:
			{
				auto const vkDestroyDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
					vkGetInstanceProcAddr(m_instance, "vkDestroyDebugUtilsMessengerEXT")
				);
				if (vkDestroyDebugUtilsMessengerEXT)
				{
					vkDestroyDebugUtilsMessengerEXT(m_instance, m_dbg_msger, /*pAllocationCallbacks**/nullptr);
				}
			}
#endif // ifndef NDEBUG
			case m_progress_t::INITIALIZED | m_progress_t::INSTANCE_CREATED:
				vkDestroyInstance(m_instance, /*VkAllocationCallbacks*/nullptr);
			// case m_progress_t::INITIALIZED:
		}
	}
	
	// TODO this has to be modified when implementing a binary tree and custom allocations with
	// suballocation
	// TODO assumes in_ext_names are UNIQUE
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
			m_registered_exts = static_cast<char**>(tmp_buf + sizeof(char[VK_MAX_EXTENSION_NAME_SIZE])*m_registered_ext_capacity*2u);

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
	
	auto renderer::setup_instance(uint32_t const ext_count, char const** ext_names) & -> status_t
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
		
		// allocating contiguous buffer which stores layer_properties, layer names in a flat contiguous format, and pointers to each string
		VkLayerProperties* const layer_properties = 
				static_cast<VkLayerProperties*>(malloc(layer_properties_cnt * (sizeof(VkLayerProperties) + sizeof(char*))));
		if (!layer_properties)
		{
			return APP_MEMORY_ERR;
		}

		vkEnumerateInstanceLayerProperties(&layer_properties_cnt, layer_properties);

		char const** enabled_layers = static_cast<decltype(enabled_layers)>(static_cast<void*>(layer_properties + layer_properties_cnt)); // TODO this is just bad code
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
			m_instance = NULL;
			return APP_GENERIC_ERR;
		}

		m_progress_status |= m_progress_t::INSTANCE_CREATED;
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

		if (vkCreateDebugUtilsMessengerEXT(m_instance, &dbg_msger_create_info, /*VkAllocationCallbacks**/nullptr, &m_dbg_msger) != VK_SUCCESS)
		{
			return APP_GENERIC_ERR;
		}

		m_progress_status |= MESSENGER_CREATED;

#endif // ifndef NDEBUG	
		return APP_SUCCESS;
	}

	auto renderer::setup_phy_device() & -> status_t
	{
		assert(m_progress_status & INSTANCE_CREATED);
		// retrieve list of vulkan capable physical devices
		uint32_t phy_devices_cnt;
		vkEnumeratePhysicalDevices(m_instance, &phy_devices_cnt, nullptr);
		auto phy_devices = static_cast<VkPhysicalDevice*>(malloc(phy_devices_cnt * sizeof(VkPhysicalDevice)));
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
			if (!phy_device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
			{
				continue;
			}

			// query available queue families
			uint32_t queue_family_properties_cnt;
			vkGetPhysicalDeviceQueueFamilyProperties(phy_devices[i], &queue_family_properties_cnt, nullptr);
			if (queue_family_properties_cnt == 0u)
			{
				free(phy_devices);
				return APP_GENERIC_ERR;
			}

			auto queue_family_properties = static_cast<VkQueueFamilyProperties*>(malloc(queue_family_properties_cnt * sizeof(VkQueueFamilyProperties)));
			if (!queue_family_properties)
			{
				free(phy_devices);
				free(queue_family_properties);
				return APP_MEMORY_ERR;
			}
			
			vkGetPhysicalDeviceQueueFamilyProperties(phy_devices[i], &queue_family_properties_cnt, queue_family_properties);
			for (uint32_t j = 0u; j < queue_family_properties_cnt; ++j)
			{
				if (0 != queue_family_properties->queueFlags & VK_QUEUE_GRAPHICS_BIT) // I care only for very few simple things
				{
					m_queue_idx.graphics = j;
					break;
				}
			}

			if (m_queue_idx.graphics == -1)
			{
				free(phy_devices);
				free(queue_family_properties);
				return APP_QUEUE_ERR;
			}

			free(queue_family_properties);
			m_phy_device = phy_devices[i];

			m_progress_status |= PHY_DEVICE_GOT;

			break;
		}

		free(phy_devices);

		printf("got physical device!\n");

		return APP_SUCCESS;
	}

	auto renderer::setup_device() & -> status_t
	{
		assert(m_progress_status & PHY_DEVICE_GOT);
		// we can also map a device to multiple physical devices, a "device group", physical devices that can access each other's memory. I'don't care
		
		// allocate necessary memory
		void* queue_buffer = malloc(APP_RENDERER_QUEUES_COUNT * (sizeof(uint32_t) + sizeof(float) + sizeof(VkDeviceQueueCreateInfo)));
		if (!queue_buffer)
		{
			return APP_MEMORY_ERR;
		}

		// "QUEUES" need to be setup BEFORE setting up the device, as queues are "CREATED ALONGSIDE THE DEVICE"
		/* for each DISTICT queue we need to create its associated VkDeviceQueueCreateInfo, "BUT", a queue having presentation feature can be the same queue
		 * we will be using for graphics operation. Therefore we need to create a set of queue indices
		 * "IMPORTANT NOTE" TODO we might need again this set of queue indices. If that is the case, move this code
		 */
		uint32_t* queue_unique_indices = static_cast<uint32_t*>(queue_buffer);
		uint32_t queue_unique_indices_cnt = 1u;
		queue_unique_indices[0] = m_queues[0];
		for (uint32_t i = 1u; i < APP_RENDERER_QUEUES_COUNT; ++i)
		{
			uint32_t j = 0u;
			for (; j < queue_unique_indices_cnt; ++j)
			{
				if (m_queues[i] == queue_unique_indices[j])
				{
					break;
				}
			}

			if (j == queue_unique_indices_cnt)
			{
				queue_unique_indices[j] = m_queues[i];
				++queue_unique_indices_cnt;
			}
		}

		// each queue must have a "priority", between 0.0 and 1.0. the higher the priority the more
		// important the queue is, and such information MIGHT be used by the implementation to eg.
		// schedule the queues, give processing time to them. Vulkan makes NO GUARANTEES
		float* queue_priorities = reinterpret_cast<float*>(queue_unique_indices + queue_unique_indices_cnt);
		for (uint32_t i = 0u; i < queue_unique_indices_cnt; ++i)
		{
			queue_priorities[i] = 1.0f;
		}
		
		// for each unique queue we need a VkDeviceQueueCreateInfo
		auto dev_queues_create_infos = static_cast<VkDeviceQueueCreateInfo*>(queue_buffer + queue_unique_indices_cnt * (sizeof(uint32_t) + sizeof(float)));
		for (uint32_t i = 0u; i < queue_unique_indices_cnt; ++i)
		{
			dev_queues_create_infos[i] = {
				.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
				.pNext = nullptr, // TODO ?
				.flags = 0, // it has just the value VK_DEVICE_QUEUE_CREATE_PROTECTED_BIT, idk what is it
				.queueFamilyIndex = queue_unique_indices[i],
				.queueCount = 1, // every queue family supports at least 1 queue. For more than one, you check the Physical device queue family properties TODO
				.pQueuePriorities = queue_priorities 
			};
		}

		VkDeviceCreateInfo const device_create_info = {
			.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
			.pNext = nullptr, // TODO ?
			.queueCreateInfoCount = queue_unique_indices_cnt,
			.pQueueCreateInfos = dev_queues_create_infos, 
			.enabledExtensionCount = 0u, // TODO ?
			.ppEnabledExtensionNames = nullptr,
			.pEnabledFeatures = nullptr // feature specification is OPTIONAL, VkPhysicalDeviceFeatures*
		}; 

		if (vkCreateDevice(m_phy_device, &device_create_info, /*VkAllocationCallbacks*/nullptr, &m_device) != VK_SUCCESS)
		{
			free(queue_buffer);
			return APP_DEVICE_CREATION_ERR;
		}

		// TODO VK_EXT_device_memory_report, in 4.2 in vkspec
		
		// if anything goes wrong with the device when executing a command, it can become "lost", to query with "VK_ERROR_DEVICE_LOST", which is a return
		// value of some operations using the device. When a device is lost, "its child objects (anything created/allocated whose creation function has as"
		// "first param the device), are not destroyed"
		
		m_progress_status |= DEVICE_CREATED;
		printf("created device!\n");

		free(queue_buffer);
		return APP_SUCCESS;
	}

	auto renderer::progress_incomplete() const & -> status_t 
	{
#ifndef NDEBUG // CMAKE_BUILD_TYPE=Debug
		status_t app_status = m_progress_status ^ (
			MESSENGER_CREATED |
			INITIALIZED |
			INSTANCE_CREATED |
			PHY_DEVICE_GOT |
			DEVICE_CREATED
		);
#elif 
		status_t app_status = m_progress_status ^ (
			INITIALIZED |
			INSTANCE_CREATED |
			PHY_DEVICE_GOT |
			DEVICE_CREATED
		);
#endif // ifndef NDEBUG
		printf("renderer status is %x\n", static_cast<uint32_t>(app_status));
		return app_status;
	}

	class app
	{
	public:
		app() : m_window(nullptr), m_renderer(), m_progress_status(0u) {}
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
		uint32_t m_progress_status; // BIT FIELD {glfw lib initialized?, glfw window created?}
		/**NOTE
		 * if this pattern of defining an m_progress_status member with its own progress enum type and
		 * defining a switch which check all the states, it could THEORETICALLY be done with static
		 * reflection automatically to spare time on class definition TODO look into that
		 */
		#define MXC_APP_PROGRESS_UNUSED_BITS 0x3fffffff
		enum m_progress_t : uint32_t
		{
			GLFW_INITIALIZED = 0x80000000,
			GLFW_WINDOW_CREATED = 0x40000000,
		};
	};

	auto app::init() -> status_t
	{
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
		if (!(glfwInit() || glfwVulkanSupported()))
		{
			fprintf(stderr, "something early on went wrong!\n");
			return APP_GENERIC_ERR;
		}

		m_progress_status |= GLFW_INITIALIZED;
		
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

		m_progress_status |= GLFW_WINDOW_CREATED;

		/**NOTE:
		 * when using OpenGL/OpenGL_ES, GLFW windows will also have a OpenGL context, because OpenGL is more tightly coupled to the platform specific windowing system (whose creation we disabled with
		 * 
		 * GLFW_NO_API above). When managing more windows, with their set of context (shared or exclusive), there are some functions which require to know, which window is being used, a.k.a. which is 
		 * the "current context", example when swapping buffers. Since we are using Vulkan, we'll do none of that and instead use the KHR extension WSI, to interface with the windowing system
		 */
		/*** instance support and physical device presentation support and creation ***/
		m_renderer.init();
		uint32_t required_instance_glfwextension_count;
		char const** required_instance_glfwextensions = glfwGetRequiredInstanceExtensions(&required_instance_glfwextension_count);
		/**NOTE:
		 * if you don't need any additional extension other than those required by GLFW, then you can pass this pointer directly to ppEnabledExtensions in VkInstanceCreateInfo.
		 * Assuming we could need more in the future, I will store them as state by a dynamically allocated buffer in the renderer
		 */
		
		if (m_renderer.setup_instance(required_instance_glfwextension_count, required_instance_glfwextensions)
			|| m_renderer.setup_phy_device()
			|| m_renderer.setup_device())
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
		status_t app_status = m_progress_status ^ (
			app::GLFW_INITIALIZED |
			app::GLFW_WINDOW_CREATED
		);
		printf("app status, before bitwise or with renderer status, is %x, while m_progress_status is %x\n", app_status, m_progress_status);
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

			/*** swap buffers ***/

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
		switch (m_progress_status)
		{
			case m_progress_t::GLFW_INITIALIZED | m_progress_t::GLFW_WINDOW_CREATED: 
				glfwDestroyWindow(m_window);
			case m_progress_t::GLFW_INITIALIZED:
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
