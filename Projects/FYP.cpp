#include <iostream>
#include <vector>
#include <cstdint>
#include <string>
#include <cstring>
#include <algorithm>

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
        if (requestedSize == 0) {
            std::cout << RED << "[Error] Cannot allocate 0 bytes!\n" << RESET;
            return nullptr;
        }
        if (requestedSize > totalPoolSize) {
            std::cout << RED << "[Error] Requested size exceeds total pool capacity!\n" << RESET;
            return nullptr;
        }

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
        std::cout << RED << "[Error] Out of Memory! No contiguous free block large enough.\n" << RESET;
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
    if (!(std::cin >> initialSize)) return 0;

    MemoryPool myPool(initialSize);
    std::vector<AllocationRecord> activeAllocs;
    int nextId = 1;

    int choice;
    do {
        std::cout << YELLOW << "\n--- CHOOSE AN ACTION ---\n" << RESET;
        std::cout << "1. Allocate Memory\n";
        std::cout << "2. Write Data into Allocated Block\n";
        std::cout << "3. View All Stored Data Content\n";
        std::cout << "4. Deallocate Memory (Free)\n";
        std::cout << "5. View Memory Map\n";
        std::cout << "6. Exit\n";
        std::cout << "Enter your choice (1-6): ";
        
        if (!(std::cin >> choice)) {
            std::cout << RED << "[Error] Invalid input type!\n" << RESET;
            std::cin.clear();
            std::cin.ignore(10000, '\n');
            continue;
        }

        switch (choice) {
            case 1: {
                size_t bytes;
                std::cout << "Enter number of bytes to allocate: ";
                std::cin >> bytes;
                void* p = myPool.allocate(bytes);
                if (p) {
                    // Initialize block with empty string/zeros for safety
                    std::memset(p, 0, bytes);
                    activeAllocs.push_back({nextId, p, bytes});
                    std::cout << GREEN << "[Success] Allocated " << bytes << " bytes. Assigned Allocation ID: " << nextId << "\n" << RESET;
                    nextId++;
                }
                myPool.visualize();
                break;
            }
            case 2: { // Write Data
                if (activeAllocs.empty()) {
                    std::cout << RED << "[Notice] No active allocations to write into!\n" << RESET;
                    break;
                }
                std::cout << "\nActive Allocations:\n";
                for (const auto& rec : activeAllocs) {
                    std::cout << "  [Allocation ID: " << rec.id << "] -> Capacity: " << rec.size << " bytes\n";
                }
                int targetId;
                std::cout << "Enter Allocation ID to write text into: ";
                std::cin >> targetId;

                auto it = std::find_if(activeAllocs.begin(), activeAllocs.end(), [targetId](const AllocationRecord& r) {
                    return r.id == targetId;
                });

                if (it != activeAllocs.end()) {
                    std::string textInput;
                    std::cout << "Enter text to store (max " << it->size - 1 << " bytes): ";
                    std::cin.ignore(); // Clear newline buffer
                    std::getline(std::cin, textInput);

                    // Safely copy text into the raw memory address pointer
                    char* dest = static_cast<char*>(it->ptr);
                    size_t copyLen = std::min(textInput.size(), it->size - 1);
                    std::memcpy(dest, textInput.c_str(), copyLen);
                    dest[copyLen] = '\0'; // Null-terminate string

                    std::cout << GREEN << "[Success] Data written to Allocation ID " << targetId << "!\n" << RESET;
                } else {
                    std::cout << RED << "[Error] Invalid Allocation ID!\n" << RESET;
                }
                break;
            }
            case 3: { // View Stored Data
                if (activeAllocs.empty()) {
                    std::cout << RED << "[Notice] No active allocations to inspect!\n" << RESET;
                    break;
                }
                std::cout << CYAN << "\n================ STORED DATA INSPECTOR ================\n" << RESET;
                for (const auto& rec : activeAllocs) {
                    char* dataPtr = static_cast<char*>(rec.ptr);
                    std::cout << "ID: " << rec.id << " | Size: " << rec.size << " bytes\n";
                    std::cout << "Stored Content : \"" << (dataPtr[0] != '\0' ? dataPtr : "(empty buffer)") << "\"\n";
                    std::cout << "-------------------------------------------------------\n";
                }
                std::cout << RESET;
                break;
            }
            case 4: { // Deallocate
                if (activeAllocs.empty()) {
                    std::cout << RED << "[Notice] No active allocations to free!\n" << RESET;
                    break;
                }
                std::cout << "\nActive Allocations:\n";
                for (const auto& rec : activeAllocs) {
                    std::cout << "  [Ref ID: " << rec.id << "] -> Size: " << rec.size << " bytes\n";
                }
                int targetId;
                std::cout << "Enter the Allocation ID you want to free: ";
                std::cin >> targetId;

                auto it = std::find_if(activeAllocs.begin(), activeAllocs.end(), [targetId](const AllocationRecord& r) {
                    return r.id == targetId;
                });

                if (it != activeAllocs.end()) {
                    myPool.deallocate(it->ptr);
                    std::cout << GREEN << "[Success] Freed Allocation ID " << targetId << "\n" << RESET;
                    activeAllocs.erase(it);
                } else {
                    std::cout << RED << "[Error] Invalid Allocation ID!\n" << RESET;
                }
                myPool.visualize();
                break;
            }
            case 5:
                myPool.visualize();
                break;
            case 6:
                std::cout << "Exiting ArenaAlloc. Good luck with your FYP defense!\n";
                break;
            default:
                std::cout << RED << "Invalid choice! Please enter 1-6.\n" << RESET;
        }
    } while (choice != 6);

    return 0;
}