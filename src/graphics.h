#pragma once

#include <vulkan/vulkan.h>
#include <glfw_window.h>
#include <vector>
#include <vertex.h>
#include <buffer_handle.h>
#include <texture_handle.h>

namespace veng {

    struct DrawIndexedIndirectCommand {
        uint32_t indexCount;
        uint32_t instanceCount;
        uint32_t firstIndex;
        int32_t  vertexOffset;
        uint32_t firstInstance;
    };

    class Graphics final {
    public:
        Graphics(gsl::not_null<Window*> window);
        ~Graphics();

        bool BeginFrame();
        void BeginRenderPass();
        void BuildHiZPyramid();
        void EndRenderPass();
        void SetModelMatrix(glm::mat4 model);
        void SetViewProjection(glm::mat4 view, glm::mat4 projection);
        void RenderBuffer(BufferHandle handle, uint32_t vertex_count);
        void RenderIndexedBuffer(
            BufferHandle vertex_handle, 
            BufferHandle index_handle, 
            uint32_t count, 
            uint32_t first_index = 0, 
            int32_t vertex_offset = 0
        );
        void RenderIndirect(
            BufferHandle vertex_handle,
            BufferHandle index_handle,
            BufferHandle indirect_handle,
            uint32_t draw_count,
            uint32_t stride = sizeof(VkDrawIndexedIndirectCommand)
        );
        void CullFrustum(
            VkDescriptorSet cull_set,
            BufferHandle output_cmds,
            BufferHandle count_buffer,
            const glm::mat4& view_proj,
            uint32_t total_objects
        );
        void RenderIndirectCount(
            BufferHandle vertex_handle,
            BufferHandle index_handle,
            BufferHandle indirect_handle,
            BufferHandle count_handle,
            uint32_t max_draw_count,
            uint32_t stride = sizeof(VkDrawIndexedIndirectCommand)
        );
        VkDescriptorSet CreateCullDescriptorSet(
            BufferHandle aabb_buffer,
            BufferHandle in_cmds_buffer,
            BufferHandle out_cmds_buffer,
            BufferHandle count_buffer,
            BufferHandle visible_instance_ids_buffer
        );
        
        void EndFrame();

        BufferHandle CreateVertexBuffer(gsl::span<Vertex> vertices);
        BufferHandle CreateIndexBuffer(gsl::span<std::uint32_t> indices);
        BufferHandle CreateIndirectBuffer(gsl::span<DrawIndexedIndirectCommand> commands);
        BufferHandle CreateStorageBuffer(VkDeviceSize size, const void* data);
        void DestroyBuffer(BufferHandle handle);
        TextureHandle CreateTexture(gsl::czstring path);
        TextureHandle CreateTextureArray(gsl::span<const std::string> paths);
        void DestroyTexture(TextureHandle handle);
        void SetTexture(TextureHandle handle);

        VkDescriptorSet CreateExpandDescriptorSet(
            BufferHandle visible_instance_count_buf,
            BufferHandle visible_instance_ids_buf,
            BufferHandle mesh_cluster_ranges_buf,
            BufferHandle candidate_cluster_indices_buf,
            BufferHandle candidate_counter_buf
        );

        VkDescriptorSet CreateClusterCullDescriptorSet(
            BufferHandle all_clusters_buf,
            BufferHandle candidate_cluster_indices_buf,
            BufferHandle candidate_counter_buf,
            BufferHandle output_draw_cmds_buf,
            BufferHandle final_draw_count_buf
        );

        void ExpandAndCullClusters(
            VkDescriptorSet expand_set,
            VkDescriptorSet cluster_cull_set,
            BufferHandle candidate_counter_buf,
            BufferHandle output_draw_cmds_buf,
            BufferHandle final_draw_count_buf,
            const glm::mat4& view_proj,
            const glm::vec3& camera_pos,
            uint32_t total_scene_clusters
        );

    private:

        struct QueueFamilyIndices {
            std::optional<std::uint32_t> graphics_family = std::nullopt;
            std::optional<std::uint32_t> presentation_family = std::nullopt;

            bool IsValid() const { return graphics_family.has_value() && presentation_family.has_value(); }
        };

        struct SwapChainProperties {
            VkSurfaceCapabilitiesKHR capabilities;
            std::vector<VkSurfaceFormatKHR> formats;
            std::vector<VkPresentModeKHR> present_modes;

            bool IsValid() const { return !formats.empty() && !present_modes.empty(); }
        };

        
        void InitializeVulkan();
        void CreateInstance();
        void SetupDebugMesssenger();
        void PickPhysicalDevice();
        void CreateLogicalDeviceAndQueues();
        void CreateSurface();
        void CreateSwapChain();
        void CreateImageViews();
        void CreateGraphicsPipeline();
        void CreateRenderPass();
        void CreateFramebuffers();
        void CreateCommandPool();
        void CreateCommandBuffer();
        void CreateSignals();
        void CreateDescriptorSetLayouts();
        void CreateDescriptorPools();
        void CreateDescriptorSets();
        void CreateTextureSampler();
        void CreateDepthResources();
        void CreateHiZSampler();
        void CreateCullPipeline();
        void CreateClusterPipelines();

        void RecreateSwapChain();
        void CleanUpSwapChain();

        std::vector<gsl::czstring> GetRequiredInstanceExtensions();

        
        void BeginCommands();
        void EndCommands();

        static gsl::span<gsl::czstring> GetSuggestedInstanceExtensions();
        static std::vector<VkExtensionProperties> GetSupportedInstanceExtensions();
        static bool AreAllExtensionsSupported(gsl::span<gsl::czstring> extensions);

        static std::vector<VkLayerProperties> GetSupportedValidationLayers();
        static bool AreAllLayersSupported(gsl::span<gsl::czstring> extensions);

        QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device);
        SwapChainProperties GetSwapChainProperties(VkPhysicalDevice device);

        std::vector<VkExtensionProperties> GetDeviceAvailableExtensions(VkPhysicalDevice device);
        bool AreAllDeviceExtensionsSupported(VkPhysicalDevice device);
        bool IsDeviceSuitable(VkPhysicalDevice device);
        std::vector<VkPhysicalDevice> GetAvailableDevices();    

        VkSurfaceFormatKHR ChooseSwapSurfaceFormat(gsl::span<VkSurfaceFormatKHR> formats);
        VkPresentModeKHR ChooseSwapPresentMode(gsl::span<VkPresentModeKHR> modes);
        VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
        std::uint32_t ChooseSwapImageCount(const VkSurfaceCapabilitiesKHR& capabilities);

        VkShaderModule CreateShaderModule(gsl::span<uint8_t> buffer);

        std::uint32_t FindMemoryTypes(std::uint32_t type_bits_filter, VkMemoryPropertyFlags required_properties);

        BufferHandle CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
        VkCommandBuffer BeginTransientCommandBuffer();
        void EndTransientCommandBuffer(VkCommandBuffer command_buffer);
        void CreateUniformBuffers();

        TextureHandle CreateImage(
            glm::ivec2 size, 
            VkFormat image_format, 
            VkBufferUsageFlags usage, 
            VkMemoryPropertyFlags properties, 
            uint32_t mip_levels = 1
        );
        void TransitionImageLayout(VkImage image, VkImageLayout old_layout, VkImageLayout new_layout);
        void CopyBufferToImage(VkBuffer buffer, VkImage image, glm::ivec2 image_size);
        VkImageView CreateImageView(
            VkImage image, 
            VkFormat format, 
            VkImageAspectFlags aspect_flag, 
            uint32_t mip_levels = 1
        );
        
        VkViewport GetViewport();
        VkRect2D GetScissor();

        std::array<gsl::czstring, 1> required_device_extensions_ = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
        };

        VkInstance instance_;
        VkDebugUtilsMessengerEXT debug_messenger_;

        VkPhysicalDevice physical_device_ = VK_NULL_HANDLE;
        VkDevice logical_device_ = VK_NULL_HANDLE;
        VkQueue graphics_queue_ = VK_NULL_HANDLE;
        VkQueue present_queue_ = VK_NULL_HANDLE;
        
        VkSurfaceKHR surface_ = VK_NULL_HANDLE;
        VkSwapchainKHR swap_chain_ = VK_NULL_HANDLE;
        VkSurfaceFormatKHR surface_format_;
        VkPresentModeKHR present_mode_;
        VkExtent2D extent_;

        std::vector<VkImage> swap_chain_images_;
        std::vector<VkImageView> swap_chain_image_views_;
        std::vector<VkFramebuffer> swap_chain_framebuffers_;

        VkPipelineLayout pipeline_layout_ = VK_NULL_HANDLE;
        VkRenderPass render_pass_ = VK_NULL_HANDLE;
        VkPipeline pipeline_ = VK_NULL_HANDLE;

        VkCommandPool command_pool_ = VK_NULL_HANDLE;
        VkCommandBuffer command_buffer_ = VK_NULL_HANDLE;

        VkSemaphore image_available_signal_ = VK_NULL_HANDLE;
        VkSemaphore render_finished_signal_ = VK_NULL_HANDLE;
        VkFence still_rendering_fence_ = VK_NULL_HANDLE;

        std::uint32_t current_image_index_ = 0;

        VkDescriptorSetLayout uniform_set_layout_ = VK_NULL_HANDLE;
        VkDescriptorPool uniform_pool_ = VK_NULL_HANDLE;
        VkDescriptorSet uniform_set_ = VK_NULL_HANDLE;
        BufferHandle uniform_buffer_;
        void* uniform_buffer_location_;

        VkDescriptorSetLayout texture_set_layout_ = VK_NULL_HANDLE;
        VkDescriptorPool texture_pool_ = VK_NULL_HANDLE;
        VkSampler texture_sampler_ = VK_NULL_HANDLE;
        TextureHandle depth_texture_;

        VkDescriptorSetLayout cull_descriptor_set_layout_ = VK_NULL_HANDLE;
        VkPipelineLayout cull_pipeline_layout_ = VK_NULL_HANDLE;
        VkPipeline cull_pipeline_ = VK_NULL_HANDLE;
        VkDescriptorPool cull_descriptor_pool_ = VK_NULL_HANDLE;

        VkImageView depth_hiz_view_;
        VkSampler hiz_sampler_ = VK_NULL_HANDLE;

        uint32_t depth_mip_levels_ = 0;
        std::vector<VkImageView> depth_mip_views_;
        VkDescriptorSetLayout hiz_descriptor_set_layout_ = VK_NULL_HANDLE;
        VkPipelineLayout hiz_pipeline_layout_ = VK_NULL_HANDLE;
        VkPipeline hiz_pipeline_ = VK_NULL_HANDLE;
        VkDescriptorPool hiz_descriptor_pool_ = VK_NULL_HANDLE;
        std::vector<VkDescriptorSet> hiz_descriptor_sets_;

        VkDescriptorSetLayout expand_descriptor_set_layout_ = VK_NULL_HANDLE;
        VkPipelineLayout expand_pipeline_layout_ = VK_NULL_HANDLE;
        VkPipeline expand_pipeline_ = VK_NULL_HANDLE;

        VkDescriptorSetLayout cluster_cull_descriptor_set_layout_ = VK_NULL_HANDLE;
        VkPipelineLayout cluster_cull_pipeline_layout_ = VK_NULL_HANDLE;
        VkPipeline cluster_cull_pipeline_ = VK_NULL_HANDLE;

        VkDescriptorPool cluster_descriptor_pool_ = VK_NULL_HANDLE;

        void CreateHiZPipeline();

        gsl::not_null<Window*> window_;
        bool validation_enabled_ = false;
    };

}