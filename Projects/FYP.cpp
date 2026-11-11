#include <iostream>
#include <cstdint>
#include <string>
#include <cstring>

// Refined ANSI Color Palette
#define RESET         "\033[0m"
#define FREE_COLOR    "\033[34m" // Blue for Free blocks
#define ALLOC_COLOR   "\033[35m" // Magenta for Allocated blocks
#define STATS_COLOR   "\033[36m" // Cyan for Stats & Headers
#define MENU_COLOR    "\033[33m" // Yellow for Menus
#define SUCCESS_COLOR "\033[32m" // Green for Success
#define ERROR_COLOR   "\033[31m" // Red for Errors

constexpr size_t MAX_ALLOCATIONS = 64; // Fixed allocation table capacity

struct BlockHeader {
    size_t size;
    bool isFree;
    BlockHeader* next;
};

class MemoryPool {
private:
    size_t totalPoolSize;
    char* pool;
    BlockHeader* head;

public:
    MemoryPool(size_t size) : totalPoolSize(size) {
        pool = new char[size];
        head = reinterpret_cast<BlockHeader*>(pool);
        head->size = totalPoolSize - sizeof(BlockHeader);
        head->isFree = true;
        head->next = nullptr;
    }

    ~MemoryPool() {
        delete[] pool;
    }

    // Disable copy constructors to prevent double-free bugs
    MemoryPool(const MemoryPool&) = delete;
    MemoryPool& operator=(const MemoryPool&) = delete;

    void* allocate(size_t requestedSize) {
        if (requestedSize == 0) {
            std::cout << ERROR_COLOR << "[Error] Cannot allocate 0 bytes!\n" << RESET;
            return nullptr;
        }
        if (requestedSize > totalPoolSize) {
            std::cout << ERROR_COLOR << "[Error] Requested size exceeds total pool capacity!\n" << RESET;
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
        std::cout << ERROR_COLOR << "[Error] Out of Memory! No contiguous free block large enough.\n" << RESET;
        return nullptr;
    }

    void deallocate(void* ptr) {
        if (!ptr) return;
        BlockHeader* targetBlock = reinterpret_cast<BlockHeader*>(ptr) - 1;
        targetBlock->isFree = true;

        // Coalescing engine: merges adjacent free blocks
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

    void visualize() const {
        std::cout << STATS_COLOR << "\n================== MEMORY POOL MAP ==================\n" << RESET;
        BlockHeader* current = head;
        int index = 0;
        size_t freeSpace = 0, usedSpace = 0;

        while (current != nullptr) {
            const char* status = current->isFree ? "FREE" : "ALLOCATED";
            const char* color = current->isFree ? FREE_COLOR : ALLOC_COLOR;

            std::cout << color << "[" << index << "] " << status 
                      << " (" << current->size << " bytes)" << RESET << " -> ";

            if (current->isFree) freeSpace += current->size;
            else usedSpace += current->size;

            current = current->next;
            index++;
        }
        std::cout << "nullptr\n";
        std::cout << STATS_COLOR << "-----------------------------------------------------\n";
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
    bool active;
};

// Helper search function replacing std::find_if
int findRecordIndex(const AllocationRecord records[], size_t count, int targetId) {
    for (size_t i = 0; i < count; ++i) {
        if (records[i].active && records[i].id == targetId) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

int main() {
    size_t initialSize;
    std::cout << STATS_COLOR << "=== WELCOME TO ARENAALLOC INTERACTIVE MANAGER ===\n" << RESET;
    std::cout << "Enter custom memory pool size (in bytes, e.g., 1024 or 4096): ";
    if (!(std::cin >> initialSize)) return 0;

    MemoryPool myPool(initialSize);
    AllocationRecord activeAllocs[MAX_ALLOCATIONS] = {}; // Raw array tracking table
    size_t allocCount = 0;
    int nextId = 1;

    int choice;
    do {
        std::cout << MENU_COLOR << "\n--- CHOOSE AN ACTION ---\n" << RESET;
        std::cout << "1. Allocate Memory\n";
        std::cout << "2. Write Data into Allocated Block\n";
        std::cout << "3. View All Stored Data Content\n";
        std::cout << "4. Deallocate Memory (Free)\n";
        std::cout << "5. View Memory Map\n";
        std::cout << "6. Exit\n";
        std::cout << "Enter your choice (1-6): ";
        
        if (!(std::cin >> choice)) {
            std::cout << ERROR_COLOR << "[Error] Invalid input type!\n" << RESET;
            std::cin.clear();
            std::cin.ignore(10000, '\n');
            continue;
        }

        switch (choice) {
            case 1: {
                if (allocCount >= MAX_ALLOCATIONS) {
                    std::cout << ERROR_COLOR << "[Error] Active allocation tracking table is full!\n" << RESET;
                    break;
                }
                size_t bytes;
                std::cout << "Enter number of bytes to allocate: ";
                if (!(std::cin >> bytes)) {
                    std::cout << ERROR_COLOR << "[Error] Invalid byte count input!\n" << RESET;
                    std::cin.clear();
                    std::cin.ignore(10000, '\n');
                    break;
                }
                void* p = myPool.allocate(bytes);
                if (p) {
                    std::memset(p, 0, bytes);
                    
                    activeAllocs[allocCount] = {nextId, p, bytes, true};
                    allocCount++;

                    std::cout << SUCCESS_COLOR << "[Success] Allocated " << bytes << " bytes. Assigned Allocation ID: " << nextId << "\n" << RESET;
                    nextId++;
                }
                myPool.visualize();
                break;
            }
            case 2: { // Write Data
                bool anyActive = false;
                for (size_t i = 0; i < allocCount; ++i) {
                    if (activeAllocs[i].active) {
                        anyActive = true;
                        break;
                    }
                }
                if (!anyActive) {
                    std::cout << ERROR_COLOR << "[Notice] No active allocations to write into!\n" << RESET;
                    break;
                }

                std::cout << "\nActive Allocations:\n";
                for (size_t i = 0; i < allocCount; ++i) {
                    if (activeAllocs[i].active) {
                        std::cout << "  [Allocation ID: " << activeAllocs[i].id << "] -> Capacity: " << activeAllocs[i].size << " bytes\n";
                    }
                }

                int targetId;
                std::cout << "Enter Allocation ID to write text into: ";
                std::cin >> targetId;

                int idx = findRecordIndex(activeAllocs, allocCount, targetId);
                if (idx != -1) {
                    std::string textInput;
                    std::cout << "Enter text to store (max " << activeAllocs[idx].size - 1 << " bytes): ";
                    std::cin.ignore();
                    std::getline(std::cin, textInput);

                    char* dest = static_cast<char*>(activeAllocs[idx].ptr);
                    bool hasExistingData = (dest[0] != '\0');

                    size_t strLen = textInput.size();
                    size_t maxCap = activeAllocs[idx].size - 1;
                    size_t copyLen = (strLen < maxCap) ? strLen : maxCap;

                    std::memcpy(dest, textInput.c_str(), copyLen);
                    dest[copyLen] = '\0';

                    if (hasExistingData) {
                        std::cout << SUCCESS_COLOR << "[Success] Updated data in Allocation ID " << targetId << "!\n" << RESET;
                    } else {
                        std::cout << SUCCESS_COLOR << "[Success] Data written to Allocation ID " << targetId << "!\n" << RESET;
                    }
                } else {
                    std::cout << ERROR_COLOR << "[Error] Invalid Allocation ID!\n" << RESET;
                }
                break;
            }
            case 3: { // View Stored Data
                bool anyActive = false;
                for (size_t i = 0; i < allocCount; ++i) {
                    if (activeAllocs[i].active) {
                        anyActive = true;
                        break;
                    }
                }
                if (!anyActive) {
                    std::cout << ERROR_COLOR << "[Notice] No active allocations to inspect!\n" << RESET;
                    break;
                }

                std::cout << STATS_COLOR << "\n================ STORED DATA INSPECTOR ================\n" << RESET;
                for (size_t i = 0; i < allocCount; ++i) {
                    if (activeAllocs[i].active) {
                        char* dataPtr = static_cast<char*>(activeAllocs[i].ptr);
                        std::cout << "ID: " << activeAllocs[i].id << " | Size: " << activeAllocs[i].size << " bytes\n";
                        std::cout << "Stored Content : \"" << (dataPtr[0] != '\0' ? dataPtr : "(empty buffer)") << "\"\n";
                        std::cout << "-------------------------------------------------------\n";
                    }
                }
                std::cout << RESET;
                break;
            }
            case 4: { // Deallocate
                bool anyActive = false;
                for (size_t i = 0; i < allocCount; ++i) {
                    if (activeAllocs[i].active) {
                        anyActive = true;
                        break;
                    }
                }
                if (!anyActive) {
                    std::cout << ERROR_COLOR << "[Notice] No active allocations to free!\n" << RESET;
                    break;
                }

                std::cout << "\nActive Allocations:\n";
                for (size_t i = 0; i < allocCount; ++i) {
                    if (activeAllocs[i].active) {
                        std::cout << "  [Ref ID: " << activeAllocs[i].id << "] -> Size: " << activeAllocs[i].size << " bytes\n";
                    }
                }

                int targetId;
                std::cout << "Enter the Allocation ID you want to free: ";
                std::cin >> targetId;

                int idx = findRecordIndex(activeAllocs, allocCount, targetId);
                if (idx != -1) {
                    myPool.deallocate(activeAllocs[idx].ptr);
                    std::cout << SUCCESS_COLOR << "[Success] Freed Allocation ID " << targetId << "!\n" << RESET;
                    activeAllocs[idx].active = false;
                } else {
                    std::cout << ERROR_COLOR << "[Error] Invalid Allocation ID!\n" << RESET;
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
                std::cout << ERROR_COLOR << "Invalid choice! Please enter 1-6.\n" << RESET;
        }
    } while (choice != 6);

    return 0;
}

