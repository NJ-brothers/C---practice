#include <iostream>
#include <vector>
#include <cstdint>
#include <string>

// ANSI Color Codes for Terminal Dashboard
#define RESET   "\033[0m"
#define GREEN   "\033[32m" // Free Block
#define RED     "\033[31m" // Allocated Block
#define CYAN    "\033[36m" // Stats
#define YELLOW  "\033[33m" // Menu

class MemoryPool {
private:
    struct BlockHeader {
        size_t size;
        bool isFree;
        BlockHeader* next;
    };

    size_t totalPoolSize;
    std::vector<char> pool;
    BlockHeader* head;

public:
    MemoryPool(size_t size) : totalPoolSize(size), pool(size) {
        head = reinterpret_cast<BlockHeader*>(pool.data());
        head->size = totalPoolSize - sizeof(BlockHeader);
        head->isFree = true;
        head->next = nullptr;
    }

    void* allocate(size_t requestedSize) {
        if (requestedSize == 0) return nullptr;
        size_t alignedSize = (requestedSize + 7) & ~7;
        BlockHeader* current = head;
        
        while (current != nullptr) {
            if (current->isFree && current->size >= alignedSize) {
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
                return static_cast<void*>(current + 1);
            }
            current = current->next;
        }
        std::cout << RED << "[Error] Out of Memory! Requested size too large.\n" << RESET;
        return nullptr;
    }

    void deallocate(void* ptr) {
        if (!ptr) return;
        BlockHeader* targetBlock = reinterpret_cast<BlockHeader*>(ptr) - 1;
        targetBlock->isFree = true;

        // Coalescing engine
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

    void visualize() {
        std::cout << CYAN << "\n================== MEMORY POOL MAP ==================\n" << RESET;
        BlockHeader* current = head;
        int index = 0;
        size_t freeSpace = 0, usedSpace = 0;

        while (current != nullptr) {
            std::string status = current->isFree ? "FREE" : "ALLOCATED";
            std::string color = current->isFree ? GREEN : RED;

            std::cout << color << "[" << index << "] " << status 
                      << " (" << current->size << " bytes)" << RESET << " -> ";

            if (current->isFree) freeSpace += current->size;
            else usedSpace += current->size;

            current = current->next;
            index++;
        }
        std::cout << "nullptr\n";
        std::cout << CYAN << "-----------------------------------------------------\n";
        std::cout << "Total Pool Capacity : " << totalPoolSize << " bytes\n";
        std::cout << "Active Allocated    : " << usedSpace << " bytes\n";
        std::cout << "Available Free Space: " << freeSpace << " bytes\n";
        std::cout << "=====================================================\n" << RESET;
    }
};

struct AllocationRecord {
    int id;
    void* ptr;
    size_t size;
};

int main() {
    size_t initialSize;
    std::cout << CYAN << "=== WELCOME TO ARENAALLOC INTERACTIVE MANAGER ===\n" << RESET;
    std::cout << "Enter custom memory pool size (in bytes, e.g., 1024 or 4096): ";
    std::cin >> initialSize;

    MemoryPool myPool(initialSize);
    std::vector<AllocationRecord> activeAllocs;
    int nextId = 1;

    int choice;
    do {
        std::cout << YELLOW << "\n--- CHOOSE AN ACTION ---\n" << RESET;
        std::cout << "1. Allocate Memory\n";
        std::cout << "2. Deallocate Memory (Free)\n";
        std::cout << "3. View Memory Map\n";
        std::cout << "4. Exit\n";
        std::cout << "Enter your choice (1-4): ";
        std::cin >> choice;

        switch (choice) {
            case 1: {
                size_t bytes;
                std::cout << "Enter number of bytes to allocate: ";
                std::cin >> bytes;
                void* p = myPool.allocate(bytes);
                if (p) {
                    activeAllocs.push_back({nextId, p, bytes});
                    std::cout << GREEN << "[Success] Allocated " << bytes << " bytes. Assigned Allocation ID: " << nextId << "\n" << RESET;
                    nextId++;
                }
                myPool.visualize();
                break;
            }
            case 2: {
                if (activeAllocs.empty()) {
                    std::cout << RED << "[Notice] No active allocations to free!\n" << RESET;
                    break;
                }
                std::cout << "\nActive Allocations:\n";
                for (size_t i = 0; i < activeAllocs.size(); ++i) {
                    std::cout << "  [Ref ID: " << activeAllocs[i].id << "] -> Size: " << activeAllocs[i].size << " bytes\n";
                }
                int targetId;
                std::cout << "Enter the Allocation ID you want to free: ";
                std::cin >> targetId;

                bool found = false;
                for (auto it = activeAllocs.begin(); it != activeAllocs.end(); ++it) {
                    if (it->id == targetId) {
                        myPool.deallocate(it->ptr);
                        std::cout << GREEN << "[Success] Freed Allocation ID " << targetId << "\n" << RESET;
                        activeAllocs.erase(it);
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    std::cout << RED << "[Error] Invalid Allocation ID!\n" << RESET;
                }
                myPool.visualize();
                break;
            }
            case 3:
                myPool.visualize();
                break;
            case 4:
                std::cout << "Exiting ArenaAlloc. Good luck with your FYP defense!\n";
                break;
            default:
                std::cout << RED << "Invalid choice! Please enter 1-4.\n" << RESET;
        }
    } while (choice != 4);

    return 0;
}