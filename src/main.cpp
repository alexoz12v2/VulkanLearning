#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <cstddef>
#include <cstdint>
#include <cstdlib> // TODO check for possible removal
#include <cstdio> // TODO check its usage for possible removal in non debug builds
#include <cstring>

// TODO remove all std:: usage (except for platform abstraction and type support facilities). example: remove std::vector
// TODO setup macro for compiler specific restrict keyword. Eg. __restrict__ for g++/clang, __restrict for MSVC, and forceinline, attribute(force_inline) for g++/clang, and __force_inline__ for MSVC
// TODO add constexpr where fit
// TODO setup documentation before source grows too much
// TODO modify app class so that it can support having multiple layers
// TODO custom allocation

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
	//... more errors
	
	// TODO change return conventions into more meaningful and specific error carrying type to convey a more 
	// 		specific status report
	/*** WARNING: most of the functions will return APP_TRUE if successful, and APP_FALSE if unsuccessful ***/

	class renderer
	{
	public: // constructors
		renderer() : m_instance(NULL), m_phy_device(NULL), m_device(NULL), 
					 m_registered_ext_count(0u), m_registered_ext_capacity(0u), m_registered_ext_buffer(nullptr), 
					 m_progress_status(0u) {} 
		~renderer();
		
	public: // public functions, initialization procedures
		auto init() -> status_t;
		auto setup_instance() -> status_t;

	public: // public function, utilities
		auto register_extension(uint32_t const in_ext_count, char const** in_ext_names) -> status_t;

	private: // data members, dispatchable vulkan objects handles
		VkInstance m_instance;
		VkPhysicalDevice m_phy_device;
		VkDevice m_device;

	private: // data members, utilities
		// TODO change this array in a form of contiguous binary tree, to guarantee uniqueness
		uint32_t m_registered_ext_count;
		uint32_t m_registered_ext_capacity;
		char(* m_registered_ext_buffer)[VK_MAX_EXTENSION_NAME_SIZE]; // VK_MAX_EXTENSION_NAME_SIZE = 256u
		uint32_t m_progress_status; // BIT FIELD: {initialized?, instance created?, ...}
		// the enum class is a fully fledged type. All we need are scoped aliases for numbers and to perform bitwise operations
		enum m_progress_t : uint32_t
		{
			INITIALIZED = 0x80000000,
			INSTANCE_CREATED = 0x40000000
		};
	};

	auto renderer::init() -> status_t 
	{
		// allocate initial buffer to store up to 32 supported extensions
		m_registered_ext_buffer = 
				static_cast<decltype(m_registered_ext_buffer)>(malloc(32u * sizeof(char[VK_MAX_EXTENSION_NAME_SIZE])));
		if (!m_registered_ext_buffer)
		{
			return APP_MEMORY_ERR;
		}
		
		m_registered_ext_capacity = 32u;
		m_progress_status |= m_progress_t::INITIALIZED;
		return APP_SUCCESS;
	}

	renderer::~renderer()
	{
		// destruction performed by using the switch statement fallthrough
		switch (m_progress_status)
		{
			case m_progress_t::INITIALIZED | m_progress_t::INSTANCE_CREATED:
				vkDestroyInstance(m_instance, /*VkAllocationCallbacks*/nullptr);
			case m_progress_t::INITIALIZED:
				free(m_registered_ext_buffer);
		}
	}
	
	auto renderer::register_extension(uint32_t const in_ext_count, char const** in_ext_names) -> status_t
	{
		// TODO this has to be modified when implementing a binary tree
		if (in_ext_count + m_registered_ext_count > m_registered_ext_capacity)
		{
			char (* tmp_buf)[VK_MAX_EXTENSION_NAME_SIZE] = 
				static_cast<decltype(tmp_buf)>(malloc(2u * m_registered_ext_capacity));
			if (!tmp_buf)
			{
				return APP_MEMORY_ERR;
			}

			memcpy(tmp_buf, 
				   m_registered_ext_buffer,
				   sizeof(char[VK_MAX_EXTENSION_NAME_SIZE])*m_registered_ext_count);
			free(m_registered_ext_buffer);
			m_registered_ext_buffer = tmp_buf;
			m_registered_ext_capacity *= 2u;
		}

		// names could also NOT be stored contiguously in the format specified by the class
		// therefore we need to copy them one by one, by checking each character for the nul terminator
		// (i.e. using strcpy)
		for (uint32_t i = 0u; i < in_ext_count; ++i)
		{
			auto cpy_address = m_registered_ext_buffer 
							 + VK_MAX_EXTENSION_NAME_SIZE*(m_registered_ext_count + i);
			strncpy(cpy_address[0], in_ext_names[i], VK_MAX_EXTENSION_NAME_SIZE);
		}
		m_registered_ext_count += in_ext_count;

		return APP_SUCCESS;
	}
	
	auto renderer::setup_instance() -> status_t
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
	
		// check for required extensions support	
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

		VkInstanceCreateInfo const instance_create_info{
			.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
			.pNext = nullptr, // TODO
			// Bitmask on some options that define the BEHAVIOUR OF THE INSTANCE
			.flags = 0x00000000,
			.pApplicationInfo = &application_info,
			.enabledLayerCount = 0u, // TODO setup them IMMEDIATELY
			.ppEnabledLayerNames = nullptr,
			.enabledExtensionCount = 0u, // TODO will change
			.ppEnabledExtensionNames = nullptr
		};

		VkResult const instance_creation_result = vkCreateInstance(&instance_create_info, /*VkAllocationCallbacks**/nullptr, &m_instance);
		if (instance_creation_result != VK_SUCCESS)
		{
			// TODO pretty printing with a primitive logging
			fprintf(stderr, "failed to create instance!");
			m_instance = NULL;
			return APP_GENERIC_ERR;
		}

		m_progress_status |= m_progress_t::INSTANCE_CREATED;
		return APP_SUCCESS;
	}

	class app
	{
	public:
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

	private: // functions


	private: // data
		// main components
		GLFWwindow* m_window;
		renderer m_renderer;

	private: // utilities functions
		auto all_progress_done() -> bool;
	
	private: // utilities data members
		uint32_t m_progress_status; // BIT FIELD {glfw lib initialized?, glfw window created?}
		/**NOTE
		 * if this pattern of defining an m_progress_status member with its own progress enum type and
		 * defining a switch which check all the states, it could THEORETICALLY be done with static
		 * reflection automatically to spare time on class definition TODO look into that
		 */
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

		/**NOTE:
		 * when using OpenGL/OpenGL_ES, GLFW windows will also have a OpenGL context, because OpenGL is more tightly coupled to the platform specific windowing system (whose creation we disabled with
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
		if (m_renderer.register_extension(required_instance_glfwextension_count, required_instance_glfwextensions)
			&& m_renderer.setup_instance())
		{
			fprintf(stderr, "failed to create instance!\n");
		}

		printf("VkInstance created!\n");

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

	auto app::all_progress_done() -> bool
	{
		return m_progress_status & (
			app::GLFW_INITIALIZED |
			app::GLFW_WINDOW_CREATED
		);
	}

	auto app::run() -> status_t
	{
		if (!all_progress_done())
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
