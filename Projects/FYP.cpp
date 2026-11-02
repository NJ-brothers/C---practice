#include <iostream>
#include <vector>
#include <cstdint>
#include <iomanip>
#include <string>

// ANSI Color Codes for Terminal Visualization
#define RESET   "\033[0m"
#define GREEN   "\033[32m" // Free Block
#define RED     "\033[31m" // Allocated Block
#define YELLOW  "\033[33m" // Metadata / Borders
#define CYAN    "\033[36m" // Stats

class MemoryPool {
private:
    struct BlockHeader {
        size_t size;          // Size of the payload data block
        bool isFree;          // True if available, false if allocated
        BlockHeader* next;    // Pointer to the next block in memory order
    };

    size_t totalPoolSize;
    std::vector<char> pool;   // The raw backing memory buffer
    BlockHeader* head;        // Start of the block list

public:
    MemoryPool(size_t size) : totalPoolSize(size), pool(size) {
        // Initialize the entire pool as one big free block
        head = reinterpret_cast<BlockHeader*>(pool.data());
        head->size = totalPoolSize - sizeof(BlockHeader);
        head->isFree = true;
        head->next = nullptr;
        std::cout << CYAN << "[Init] Memory Pool initialized with " << totalPoolSize << " bytes.\n" << RESET;
    }

    // Custom Allocate Function (First-Fit Strategy)
    void* allocate(size_t requestedSize) {
        // Align requested size to 8-byte boundaries for CPU efficiency
        size_t alignedSize = (requestedSize + 7) & ~7;
        
        BlockHeader* current = head;
        while (current != nullptr) {
            if (current->isFree && current->size >= alignedSize) {
                // Check if we can split the block
                if (current->size >= alignedSize + sizeof(BlockHeader) + 8) {
                    BlockHeader* nextBlock = reinterpret_cast<BlockHeader*>(
                        reinterpret_cast<char*>(current + 1) + alignedSize
                    );
                    
                    nextBlock->size = current->size - alignedSize - sizeof(BlockHeader);
                    nextBlock->isFree = true;
                    nextBlock->next = current->next;

                    current->size = alignedSize;
                    current->next = nextBlock;
                }

                current->isFree = false;
                return static_cast<void*>(current + 1); // Return pointer to payload
            }
            current = current->next;
        }

        std::cerr << RED << "[Error] Out of Memory! Allocation failed for " << requestedSize << " bytes.\n" << RESET;
        return nullptr;
    }

    // Custom Deallocate Function with Coalescing
    void deallocate(void* ptr) {
        if (!ptr) return;

        // Retrieve header from payload pointer
        BlockHeader* targetBlock = reinterpret_cast<BlockHeader*>(ptr) - 1;
        targetBlock->isFree = true;

        // Coalesce adjacent free blocks to prevent fragmentation
        BlockHeader* current = head;
        while (current != nullptr && current->next != nullptr) {
            if (current->isFree && current->next->isFree) {
                current->size += sizeof(BlockHeader) + current->next->size;
                current->next = current->next->next;
            } else {
                current = current->next;
            }
        }
    }

    // Terminal Visualizer & Statistics Dashboard
    void visualize() {
        std::cout << "\n" << CYAN << "================== MEMORY POOL MAP ==================\n" << RESET;
        
        BlockHeader* current = head;
        int blockIndex = 0;
        size_t totalAllocated = 0;
        size_t totalFree = 0;

        while (current != nullptr) {
            std::string status = current->isFree ? "FREE" : "ALLOCATED";
            std::string color = current->isFree ? GREEN : RED;

            std::cout << color << "[" << blockIndex << "] " 
                      << status << " (" << current->size << " bytes)" << RESET << " -> ";

            if (current->isFree) totalFree += current->size;
            else totalAllocated += current->size;

            current = current->next;
            blockIndex++;
        }
        std::cout << YELLOW << "nullptr\n" << RESET;

        std::cout << CYAN << "-----------------------------------------------------\n";
        std::cout << "Total Pool Capacity : " << totalPoolSize << " bytes\n";
        std::cout << "Active Allocated    : " << totalAllocated << " bytes\n";
        std::cout << "Available Free Space: " << totalFree << " bytes\n";
        std::cout << "=====================================================\n\n" << RESET;
    }
};

// Interactive Test Program
int main() {
    // Create a memory pool of 1024 bytes
    MemoryPool myPool(102004);

    myPool.visualize();

    // Allocate some blocks
    std::cout << "[Action] Allocating 128 bytes (p1)...\n";
    void* p1 = myPool.allocate(128);
    myPool.visualize();

    std::cout << "[Action] Allocating 256 bytes (p2)...\n";
    void* p2 = myPool.allocate(256);
    myPool.visualize();

    std::cout << "[Action] Allocating 64 bytes (p3)...\n";
    void* p3 = myPool.allocate(64);
    myPool.visualize();

    // Free a block to demonstrate coalescing
    std::cout << "[Action] Deallocating p2 (256 bytes)...\n";
    myPool.deallocate(p2);
    myPool.visualize();

    std::cout << "[Action] Deallocating p1 (128 bytes)...\n";
    myPool.deallocate(p1);
    myPool.visualize();

    // Clean up remaining
    myPool.deallocate(p3);

    return 0;
}