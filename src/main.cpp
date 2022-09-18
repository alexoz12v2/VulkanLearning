#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <cstddef>
#include <cstdint>
#include <cstdlib> // TODO check for possible removal
#include <cstdio> // TODO check its usage for possible removal in non debug builds


// NOTE: glfwGetGammaRamp to get the monitor's gamma
// NOTE: to get started more "smoothly", I WILL NOT DO CUSTOM ALLOCATION IN THIS MOCK APPLICATION
// 		 subsequent Vulkan trainings will include more as I learn from scratch graphics development

namespace mxc
{
	// GLFW and Vulkan uses 32 bit unsigned int as bool, i'll provide my definition as well
	using bool32_t = uint32_t;
	#define APP_FALSE 0
	#define APP_TRUE 1

	class renderer
	{
	public:
		// constructors
		renderer() : m_instance(NULL), m_phy_device(NULL), m_device(NULL) {}

		// public functions
		auto setup_instance() -> bool32_t;
		auto cleanup_step() -> void;
	private:
		VkInstance m_instance;
		VkPhysicalDevice m_phy_device;
		VkDevice m_device;
	};

	auto renderer::setup_instance() -> bool32_t
	{
     	// find supported version of vulkan
		uint32_t supported_vk_api_version;
    	vkEnumerateInstanceVersion(&supported_vk_api_version);	
       	if (supported_vk_api_version < VK_MAKE_API_VERSION(/*variant*/0,/*major*/1,/*minor*/2,/*patch*/0))
       	{
			// TODO substitute with a primitive logging with pretty printing
       		fprintf(stderr, "system does not support vulkan 1.1 or higher!");
			return APP_FALSE;
       	}
	
		
		// create the Vkinstance TODO will be modified to include validation layers
		const VkApplicationInfo application_info = {
			.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
			.pNext = nullptr,
			.pApplicationName = "my first vulkan app",
			.applicationVersion = 1u,
			.pEngineName = "my first vulkan app",
			.engineVersion = 1u,
			.apiVersion = VK_MAKE_API_VERSION(/*variant*/0,/*major*/1,/*minor*/2,/*patch*/0) // the only useful thing here
		};

		const VkInstanceCreateInfo instance_create_info{
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

		const VkResult instance_creation_result = vkCreateInstance(&instance_create_info, /*VkAllocationCallbacks**/nullptr, &m_instance);
		if (instance_creation_result != VK_SUCCESS)
		{
			// TODO pretty printing with a primitive logging
			fprintf(stderr, "failed to create instance!");
			m_instance = NULL;
			return APP_FALSE;
		}

		return APP_FALSE;
	}
	
	auto renderer::cleanup_step() -> void
	{
		// if instance != NULL then it must be valid, fortunately we have a default constructor
		vkDestroyInstance(m_instance, /*VkAllocationCallbacks**/nullptr);
	}

	class app
	{
	public:
		// window and vulkan initialization
		auto initialization_step() -> bool32_t;
		// TODO
		auto presentation_setup_step() -> bool32_t;
		// TODO
		auto resource_setup_step() -> bool32_t;
		// TODO
		auto pipeline_setup_step() -> bool32_t;
		// TODO
		auto descriptorsets_setup_step() -> bool32_t;
		// TODO
		auto spirv_shaders_step() -> bool32_t;
		// TODO
		auto record_commands_step() -> bool32_t;

		auto cleanup_step() -> void;

	private: // functions


	private: // data
		GLFWwindow* m_window;
		renderer m_renderer;
	};

	auto app::initialization_step() -> bool32_t
	{
		// TODO refactor all in a big if statement, eg.:
		// if (  operation1
		//    && operation2
		//    && operation3 ...
		// 	  )
		// {
		//     return APP_TRUE;
		// }
		// else
		// {
		//     return APP_FALSE;
		// }
		// try to initialize the glfw library and see if you can find a eligeable vulkan loader
		if (glfwInit() * glfwVulkanSupported() == APP_FALSE)
		{
			// TODO find vulkan library manually locating pointers to functions??
			return APP_FALSE;
		}

		/***  instance support and physical device presentation support and creation ***/
		m_renderer.setup_instance();
		
		return APP_TRUE;
	}
		
	auto app::presentation_setup_step() -> bool32_t
	{
		bool32_t result = APP_FALSE;
		// Specify we do not want to use OpenGL, we are going to setup Vulkan
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

		// assuming the initialization_step was run, TODO add macros, then we can setup our VkSurface and swapchain 

		// TODO = for now we disable resizing
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

		return result;
	}

	auto app::resource_setup_step() -> bool32_t 
	{
		return APP_FALSE;	
	}

	auto app::pipeline_setup_step() -> bool32_t 
	{
		return APP_FALSE;
	}
	
	auto app::descriptorsets_setup_step() -> bool32_t 
	{
		return APP_FALSE;
	}
	
	auto app::spirv_shaders_step() -> bool32_t 
	{
		return APP_FALSE;
	}
	
	auto app::record_commands_step() -> bool32_t 
	{
		return APP_FALSE;
	}

	auto app::app::cleanup_step() -> void
	{
		m_renderer.cleanup_step();
		glfwTerminate();
	}
}

auto main(int32_t argc, char* argv[]) -> int
{
	mxc::app app_instance;
	if (app_instance.initialization_step())
	{
		return EXIT_FAILURE;
	}

	// TODO figure out what needs to be in the destructor and what doesn't. For instance: glfwTerminate may be called before glfwInit, but 
	app_instance.cleanup_step();

	return EXIT_SUCCESS;
}
