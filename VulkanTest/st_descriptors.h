#pragma once
#include "st_device.h"
// std
#include <memory>
#include <unordered_map>
#include <vector>
namespace st {
    class StDescriptorSetLayout {
    public:
        class Builder {
        public:
            Builder(StDevice &stDevice) : stDevice{stDevice} {}
            Builder &addBinding(
                uint32_t binding,
                VkDescriptorType descriptorType,
                VkShaderStageFlags stageFlags,
                uint32_t count = 1);
            std::unique_ptr<StDescriptorSetLayout> build() const;
        private:
            StDevice &stDevice;
            std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings{};
        };
        StDescriptorSetLayout(
            StDevice &stDevice, std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings);
        ~StDescriptorSetLayout();
        StDescriptorSetLayout(const StDescriptorSetLayout &) = delete;
        StDescriptorSetLayout &operator=(const StDescriptorSetLayout &) = delete;
        VkDescriptorSetLayout getDescriptorSetLayout() const { return descriptorSetLayout; }
    private:
        StDevice &stDevice;
        VkDescriptorSetLayout descriptorSetLayout;
        std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings;
        friend class StDescriptorWriter;
    };
    class StDescriptorPool {
    public:
        class Builder {
        public:
            Builder(StDevice &stDevice) : stDevice{stDevice} {}
            Builder &addPoolSize(VkDescriptorType descriptorType, uint32_t count);
            Builder &setPoolFlags(VkDescriptorPoolCreateFlags flags);
            Builder &setMaxSets(uint32_t count);
            std::unique_ptr<StDescriptorPool> build() const;
        private:
            StDevice &stDevice;
            std::vector<VkDescriptorPoolSize> poolSizes{};
            uint32_t maxSets = 1000;
            VkDescriptorPoolCreateFlags poolFlags = 0;
        };
        StDescriptorPool(
            StDevice &stDevice,
            uint32_t maxSets,
            VkDescriptorPoolCreateFlags poolFlags,
            const std::vector<VkDescriptorPoolSize> &poolSizes);
        ~StDescriptorPool();
        StDescriptorPool(const StDescriptorPool &) = delete;
        StDescriptorPool &operator=(const StDescriptorPool &) = delete;
        bool allocateDescriptorSet(
            const VkDescriptorSetLayout descriptorSetLayout, VkDescriptorSet &descriptor) const;
        void freeDescriptors(std::vector<VkDescriptorSet> &descriptors) const;
        void resetPool();
    private:
        StDevice &stDevice;
        VkDescriptorPool descriptorPool;
        friend class StDescriptorWriter;
    };
    class StDescriptorWriter {
    public:
        StDescriptorWriter(StDescriptorSetLayout &setLayout, StDescriptorPool &pool);
        StDescriptorWriter &writeBuffer(uint32_t binding, VkDescriptorBufferInfo *bufferInfo);
        StDescriptorWriter &writeImage(uint32_t binding, VkDescriptorImageInfo *imageInfo);
        bool build(VkDescriptorSet &set);
        void overwrite(VkDescriptorSet &set);
    private:
        StDescriptorSetLayout &setLayout;
        StDescriptorPool &pool;
        std::vector<VkWriteDescriptorSet> writes;
    };
}