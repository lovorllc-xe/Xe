#pragma once
#include <algorithm>
#include <atomic>
#include <cctype>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <cstdlib>
#include <exception>
#include <fstream>
#include <functional>
#include <iostream>
#include <iomanip>
#include <limits>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <sstream>
#include <type_traits>
#include <typeindex>
#include <functional>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#if defined(__linux__)
#include <sys/syscall.h>
#include <unistd.h>
#endif
#include <new>
#if defined(__unix__) || defined(__APPLE__)
#include <sys/wait.h>
#endif

namespace xe::runtime {
struct RawPointer {
    void* value = nullptr;
    std::size_t size = 0;
    RawPointer(void* address = nullptr, std::size_t bytes = 0) : value(address), size(bytes) {}
    template<class T> operator T*() const;
    explicit operator bool() const { return value != nullptr; }
    friend std::ostream& operator<<(std::ostream& out, RawPointer pointer) { return out << pointer.value; }
    friend bool operator==(RawPointer left, RawPointer right) { return left.value == right.value; }
    friend bool operator!=(RawPointer left, RawPointer right) { return left.value != right.value; }
    template<class T> friend bool operator==(RawPointer left, T* right) { return left.value == static_cast<const void*>(right); }
    template<class T> friend bool operator==(T* left, RawPointer right) { return static_cast<const void*>(left) == right.value; }
    template<class T> friend bool operator!=(RawPointer left, T* right) { return !(left == right); }
    template<class T> friend bool operator!=(T* left, RawPointer right) { return !(left == right); }
};
template<class T> inline void* rawAddress(T value) {
    if constexpr (std::is_pointer_v<T>) return const_cast<void*>(static_cast<const void*>(value));
    else if constexpr (std::is_integral_v<T>) return reinterpret_cast<void*>(static_cast<std::uintptr_t>(value));
    else return nullptr;
}
inline void* rawAddress(RawPointer value) { return value.value; }
template<class D, class S, class N> inline void sysMemCopy(D destination, S source, N bytes) {
    if (bytes < 0) throw std::runtime_error("memory size cannot be negative");
    if (bytes && (!rawAddress(destination) || !rawAddress(source))) throw std::runtime_error("memory copy requires non-null pointers");
    std::memmove(rawAddress(destination), rawAddress(source), static_cast<std::size_t>(bytes));
}
template<class D, class V, class N> inline void sysMemSet(D destination, V value, N bytes) {
    if (bytes < 0) throw std::runtime_error("memory size cannot be negative");
    if (bytes && !rawAddress(destination)) throw std::runtime_error("memory set requires a non-null pointer");
    std::memset(rawAddress(destination), static_cast<unsigned char>(value), static_cast<std::size_t>(bytes));
}
template<class A, class B, class N> inline std::int64_t sysMemCompare(A first, B second, N bytes) {
    if (bytes < 0) throw std::runtime_error("memory size cannot be negative");
    if (bytes && (!rawAddress(first) || !rawAddress(second))) throw std::runtime_error("memory compare requires non-null pointers");
    return std::memcmp(rawAddress(first), rawAddress(second), static_cast<std::size_t>(bytes));
}
template<class P> inline std::int64_t atomicLoad(P address) {
    auto* pointer = static_cast<std::int64_t*>(rawAddress(address));
    if (!pointer || reinterpret_cast<std::uintptr_t>(pointer) % alignof(std::int64_t)) throw std::runtime_error("atomic operation requires an aligned non-null address");
    return std::atomic_ref<std::int64_t>(*pointer).load(std::memory_order_seq_cst);
}
template<class P> inline std::int64_t pointerIsNull(P pointer) { return rawAddress(pointer) == nullptr ? 1 : 0; }
template<class P, class V> inline void atomicStore(P address, V value) {
    auto* pointer = static_cast<std::int64_t*>(rawAddress(address));
    if (!pointer || reinterpret_cast<std::uintptr_t>(pointer) % alignof(std::int64_t)) throw std::runtime_error("atomic operation requires an aligned non-null address");
    std::atomic_ref<std::int64_t>(*pointer).store(static_cast<std::int64_t>(value), std::memory_order_seq_cst);
}
template<class P, class E, class D> inline std::int64_t atomicCompareExchange(P address, E expected, D desired) {
    auto* pointer = static_cast<std::int64_t*>(rawAddress(address));
    if (!pointer || reinterpret_cast<std::uintptr_t>(pointer) % alignof(std::int64_t)) throw std::runtime_error("atomic operation requires an aligned non-null address");
    auto expectedValue = static_cast<std::int64_t>(expected);
    return std::atomic_ref<std::int64_t>(*pointer).compare_exchange_strong(expectedValue, static_cast<std::int64_t>(desired), std::memory_order_seq_cst) ? 1 : 0;
}
template<class P> inline std::int64_t sysHardwareRead8(P address) { return *static_cast<volatile std::uint8_t*>(rawAddress(address)); }
template<class P> inline void sysHardwareWrite8(P address, std::int64_t value) { *static_cast<volatile std::uint8_t*>(rawAddress(address)) = static_cast<std::uint8_t>(value); }
template<class P> inline std::int64_t sysHardwareRead32(P address) { return *static_cast<volatile std::int32_t*>(rawAddress(address)); }
template<class P> inline void sysHardwareWrite32(P address, std::int64_t value) { *static_cast<volatile std::int32_t*>(rawAddress(address)) = static_cast<std::int32_t>(value); }
template<class P> inline RawPointer pointerRead(P address) { return RawPointer(*static_cast<void**>(rawAddress(address)), sizeof(void*)); }
template<class P, class V> inline void pointerWrite(P address, V value) { *static_cast<void**>(rawAddress(address)) = rawAddress(value); }
template<class P> inline std::string* pointerAsString(P address) { return static_cast<std::string*>(rawAddress(address)); }
inline std::int64_t syscallWord(RawPointer value) { return static_cast<std::int64_t>(reinterpret_cast<std::uintptr_t>(value.value)); }
template<class T> inline std::int64_t syscallWord(T value) {
    if constexpr (std::is_pointer_v<T>) return static_cast<std::int64_t>(reinterpret_cast<std::uintptr_t>(value));
    else if constexpr (std::is_integral_v<T>) return static_cast<std::int64_t>(value);
    else return static_cast<std::int64_t>(value);
}
inline std::int64_t syscallWord(const std::string& value) { return static_cast<std::int64_t>(reinterpret_cast<std::uintptr_t>(value.c_str())); }
template<class A, class B, class C, class D> inline std::int64_t syscallInvoke3(A number, B first, C second, D third) {
#if defined(__linux__)
    auto result = ::syscall(syscallWord(number), syscallWord(first), syscallWord(second), syscallWord(third));
    return result < 0 ? -errno : static_cast<std::int64_t>(result);
#else
    throw std::runtime_error("raw syscalls are supported only on Linux");
#endif
}
template<class A, class B, class C, class D, class E, class F, class G> inline std::int64_t syscallInvoke6(A number, B first, C second, D third, E fourth, F fifth, G sixth) {
#if defined(__linux__)
    auto result = ::syscall(syscallWord(number), syscallWord(first), syscallWord(second), syscallWord(third), syscallWord(fourth), syscallWord(fifth), syscallWord(sixth));
    return result < 0 ? -errno : static_cast<std::int64_t>(result);
#else
    throw std::runtime_error("raw syscalls are supported only on Linux");
#endif
}
template<class T> inline std::int64_t address(T* pointer) { return static_cast<std::int64_t>(reinterpret_cast<std::uintptr_t>(pointer)); }
inline std::int64_t address(RawPointer pointer) { return address(pointer.value); }
template<class A, class B> inline auto add(A left, B right) {
    if constexpr (std::is_integral_v<A> && std::is_integral_v<B>) {
        auto a = static_cast<std::int64_t>(left), b = static_cast<std::int64_t>(right);
        if ((b > 0 && a > std::numeric_limits<std::int64_t>::max() - b) || (b < 0 && a < std::numeric_limits<std::int64_t>::min() - b)) throw std::overflow_error("integer overflow");
        return static_cast<std::int64_t>(a + b);
    } else if constexpr (std::is_pointer_v<A> && std::is_void_v<std::remove_pointer_t<A>> && std::is_integral_v<B>) {
        return static_cast<A>(static_cast<void*>(static_cast<std::byte*>(left) + right));
    } else if constexpr (std::is_integral_v<A> && std::is_pointer_v<B> && std::is_void_v<std::remove_pointer_t<B>>) {
        return static_cast<B>(static_cast<void*>(static_cast<std::byte*>(right) + left));
    } else return left + right;
}
template<class A, class B> inline auto subtract(A left, B right) {
    if constexpr (std::is_integral_v<A> && std::is_integral_v<B>) {
        auto a = static_cast<std::int64_t>(left), b = static_cast<std::int64_t>(right);
        if ((b < 0 && a > std::numeric_limits<std::int64_t>::max() + b) || (b > 0 && a < std::numeric_limits<std::int64_t>::min() + b)) throw std::overflow_error("integer overflow");
        return static_cast<std::int64_t>(a - b);
    } else if constexpr (std::is_pointer_v<A> && std::is_void_v<std::remove_pointer_t<A>> && std::is_integral_v<B>) {
        return static_cast<A>(static_cast<void*>(static_cast<std::byte*>(left) - right));
    } else return left - right;
}
template<class A, class B> inline auto multiply(A left, B right) {
    if constexpr (std::is_integral_v<A> && std::is_integral_v<B>) {
        auto a = static_cast<std::int64_t>(left), b = static_cast<std::int64_t>(right);
        bool overflow = false;
        if (a > 0) overflow = b > 0 ? a > std::numeric_limits<std::int64_t>::max() / b : b < std::numeric_limits<std::int64_t>::min() / a;
        else if (a < 0) overflow = b > 0 ? a < std::numeric_limits<std::int64_t>::min() / b : (b < 0 && b < std::numeric_limits<std::int64_t>::max() / a);
        if (overflow) throw std::overflow_error("integer overflow");
        return static_cast<std::int64_t>(a * b);
    } else return left * right;
}
template<class A, class B> inline auto divide(A left, B right) {
    if constexpr (std::is_integral_v<A> && std::is_integral_v<B>) {
        auto a = static_cast<std::int64_t>(left), b = static_cast<std::int64_t>(right);
        if (!b) throw std::runtime_error("division by zero");
        if (a == std::numeric_limits<std::int64_t>::min() && b == -1) throw std::overflow_error("integer overflow");
        return static_cast<std::int64_t>(a / b);
    } else {
        return left / right;
    }
}
template<class A, class B> inline std::int64_t remainder(A left, B right) {
    auto a = static_cast<std::int64_t>(left), b = static_cast<std::int64_t>(right);
    if (!b) throw std::runtime_error("division by zero");
    if (a == std::numeric_limits<std::int64_t>::min() && b == -1) return 0;
    return a % b;
}
template<class T> inline auto negate(T value) {
    if constexpr (std::is_integral_v<T> && std::is_signed_v<T>) {
        auto integer = static_cast<std::int64_t>(value);
        if (integer == std::numeric_limits<std::int64_t>::min()) throw std::overflow_error("integer overflow");
        return static_cast<std::int64_t>(-integer);
    } else return -value;
}
struct ObjectRecord {
    std::type_index type;
    std::function<void(void*)> destroy;
    ObjectRecord(std::type_index objectType, std::function<void(void*)> destructor) : type(objectType), destroy(std::move(destructor)) {}
};
class RegionArena {
    struct Chunk { std::unique_ptr<std::byte[]> data; std::size_t capacity; };
    struct Object { void* address; ObjectRecord record; };
public:
    RegionArena() = default;
    RegionArena(const RegionArena&) = delete;
    RegionArena& operator=(const RegionArena&) = delete;
    ~RegionArena() { destroyObjects(); }
    void* allocate(std::size_t size, std::size_t alignment = alignof(std::max_align_t)) {
        std::lock_guard lock(mutex);
        if (!validAlignment(alignment)) throw std::invalid_argument("alignment must be a power of two at least alignof(void*)");
        constexpr std::size_t usualChunk = 64 * 1024;
        if (chunks.empty() || !fits(chunks.back(), size, alignment)) {
            if (size > std::numeric_limits<std::size_t>::max() - alignment) throw std::bad_alloc();
            auto capacity = std::max(usualChunk, size + alignment);
            chunks.push_back({std::make_unique<std::byte[]>(capacity), capacity}); used = 0;
        }
        auto base = reinterpret_cast<std::uintptr_t>(chunks.back().data.get());
        auto address = base + used;
        auto aligned = (address + alignment - 1) & ~(static_cast<std::uintptr_t>(alignment) - 1);
        used = static_cast<std::size_t>(aligned - base) + size;
        return reinterpret_cast<void*>(aligned);
    }
    bool owns(const void* pointer) const {
        std::lock_guard lock(mutex);
        auto address = reinterpret_cast<std::uintptr_t>(pointer);
        for (const auto& chunk : chunks) {
            auto base = reinterpret_cast<std::uintptr_t>(chunk.data.get());
            if (address >= base && address - base < chunk.capacity) return true;
        }
        return false;
    }
    void registerObject(void* address, ObjectRecord record) { objects.push_back({address, std::move(record)}); }
    template<class T> void constructObject(T* address) {
        std::lock_guard lock(mutex);
        auto object = std::find_if(objects.begin(), objects.end(), [&](const Object& item) { return item.address == address; });
        if (object != objects.end()) {
            if (object->record.type != typeid(T)) throw std::runtime_error("allocation was already converted to a different pointer type");
            return;
        }
        ::new (static_cast<void*>(address)) T{};
        registerObject(address, ObjectRecord{typeid(T), [](void* location) { std::destroy_at(static_cast<T*>(location)); }});
    }
    void destroyObject(void* address) {
        std::lock_guard lock(mutex);
        auto object = std::find_if(objects.begin(), objects.end(), [&](const Object& item) { return item.address == address; });
        if (object == objects.end()) return;
        object->record.destroy(address);
        objects.erase(object);
    }
private:
    static bool validAlignment(std::size_t alignment) {
        return alignment >= alignof(void*) && (alignment & (alignment - 1)) == 0;
    }
    void destroyObjects() noexcept {
        for (auto object = objects.rbegin(); object != objects.rend(); ++object) object->record.destroy(object->address);
        objects.clear();
    }
    bool fits(const Chunk& chunk, std::size_t size, std::size_t alignment) const {
        auto base = reinterpret_cast<std::uintptr_t>(chunk.data.get());
        auto address = base + used;
        auto aligned = (address + alignment - 1) & ~(static_cast<std::uintptr_t>(alignment) - 1);
        auto offset = static_cast<std::size_t>(aligned - base);
        return offset <= chunk.capacity && size <= chunk.capacity - offset;
    }
    std::vector<Chunk> chunks;
    std::vector<Object> objects;
    std::size_t used = 0;
    mutable std::mutex mutex;
};
inline thread_local std::vector<std::shared_ptr<RegionArena>> regions;
inline std::mutex heapAllocationMutex;
inline std::unordered_set<void*> heapAllocations;
inline std::unordered_map<void*, std::size_t> heapAllocationAlignments;
inline std::unordered_map<void*, ObjectRecord> heapObjects;
struct XeThreadState { std::thread worker; std::int64_t result = 0; };
inline std::mutex xeThreadMutex;
inline std::unordered_map<std::int64_t, std::shared_ptr<XeThreadState>> xeThreads;
inline std::atomic<std::int64_t> xeNextThreadId{1};
inline std::vector<std::string> processArguments;
inline void setArguments(int argc, char** argv) {
    processArguments.clear();
    for (int index = 1; index < argc; ++index) processArguments.emplace_back(argv[index]);
}
inline std::int64_t argCount() { return static_cast<std::int64_t>(processArguments.size()); }
inline std::int64_t processExitCode(int status) {
#if defined(__unix__) || defined(__APPLE__)
    if (status == -1) return -1;
    if (WIFEXITED(status)) return WEXITSTATUS(status);
    if (WIFSIGNALED(status)) return 128 + WTERMSIG(status);
#endif
    return status;
}
inline std::int64_t runProcess(const std::string& command) { return processExitCode(std::system(command.c_str())); }
inline std::int64_t spawnProcess(const std::string& command) {
#if defined(__unix__) || defined(__APPLE__)
    auto child = ::fork();
    if (child < 0) return -1;
    if (child == 0) {
        ::execl("/bin/sh", "sh", "-c", command.c_str(), static_cast<char*>(nullptr));
        ::_exit(127);
    }
    return static_cast<std::int64_t>(child);
#else
    (void)command;
    return -1;
#endif
}
inline std::int64_t waitProcess(std::int64_t process) {
#if defined(__unix__) || defined(__APPLE__)
    int status = 0;
    pid_t result;
    do { result = ::waitpid(static_cast<pid_t>(process), &status, 0); } while (result < 0 && errno == EINTR);
    if (result < 0) return -1;
    return processExitCode(status);
#else
    (void)process;
    return -1;
#endif
}
inline const std::string& arg(std::int64_t index) {
    if (index < 0 || static_cast<std::uint64_t>(index) >= processArguments.size()) throw std::out_of_range("argument index is out of range");
    return processArguments[static_cast<std::size_t>(index)];
}
inline std::string environmentValue(const std::string& key) {
    const char* value = std::getenv(key.c_str());
    return value ? std::string(value) : std::string{};
}
template<class F, class A> inline std::int64_t threadStart(F entry, A argument) {
    using Entry = std::int64_t (*)(void*);
    auto callback = reinterpret_cast<Entry>(rawAddress(entry));
    void* callbackArgument = rawAddress(argument);
    auto state = std::make_shared<XeThreadState>();
    const auto id = xeNextThreadId.fetch_add(1, std::memory_order_relaxed);
    state->worker = std::thread([state, callback, callbackArgument] { state->result = callback(callbackArgument); });
    { std::lock_guard lock(xeThreadMutex); xeThreads.emplace(id, state); }
    return id;
}
inline std::int64_t threadJoin(std::int64_t id) {
    std::shared_ptr<XeThreadState> state;
    { std::lock_guard lock(xeThreadMutex); auto found = xeThreads.find(id); if (found == xeThreads.end()) return -1; state = found->second; xeThreads.erase(found); }
    if (state->worker.joinable()) state->worker.join();
    return state->result;
}
inline std::string readFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) throw std::runtime_error("cannot open file for reading: " + path);
    std::ostringstream contents; contents << file.rdbuf();
    if (!file.good() && !file.eof()) throw std::runtime_error("failed while reading file: " + path);
    return contents.str();
}
inline void writeFile(const std::string& path, const std::string& contents) {
    std::ofstream file(path, std::ios::binary);
    if (!file) throw std::runtime_error("cannot open file for writing: " + path);
    file.write(contents.data(), static_cast<std::streamsize>(contents.size()));
    if (!file) throw std::runtime_error("failed while writing file: " + path);
}
inline std::int64_t stringLength(const std::string& value) { return static_cast<std::int64_t>(value.size()); }
inline std::string stringBuilder(const std::string& initial = {}) {
    std::string result;
    result.reserve(std::max<std::size_t>(256, initial.size()));
    result.append(initial);
    return result;
}
inline std::string& stringAppend(std::string& value, const std::string& suffix) {
    value.append(suffix);
    return value;
}
inline std::int64_t stringChar(const std::string& value, std::int64_t index) {
    if (index < 0 || static_cast<std::uint64_t>(index) >= value.size()) throw std::out_of_range("string index is out of range");
    return static_cast<unsigned char>(value[static_cast<std::size_t>(index)]);
}
inline std::string stringSlice(const std::string& value, std::int64_t first, std::int64_t last) {
    if (first < 0 || last < first || static_cast<std::uint64_t>(last) > value.size()) throw std::out_of_range("string slice bounds are out of range");
    return value.substr(static_cast<std::size_t>(first), static_cast<std::size_t>(last - first));
}
inline std::int64_t stringFind(const std::string& value, const std::string& needle) {
    const auto found = value.find(needle);
    return found == std::string::npos ? -1 : static_cast<std::int64_t>(found);
}
inline std::string stringNumber(std::int64_t value) { return std::to_string(value); }
inline std::string stringFromChar(std::int64_t value) {
    if (value < 0 || value > 255) throw std::out_of_range("character value must be between 0 and 255");
    return std::string(1, static_cast<char>(value));
}

template<class T> RawPointer::operator T*() const {
    if constexpr (!std::is_void_v<T>) {
        if (size < sizeof(T)) throw std::length_error("allocation is too small for the requested pointer type");
        bool regionOwned = false;
        for (auto& region : regions) if (region->owns(value)) {
            region->constructObject(static_cast<T*>(value));
            regionOwned = true;
            break;
        }
        if (!regionOwned) {
            std::lock_guard lock(heapAllocationMutex);
            if (!heapAllocations.contains(value)) throw std::runtime_error("pointer conversion requires a live allocation");
            auto existing = heapObjects.find(value);
            if (existing != heapObjects.end()) {
                if (existing->second.type != typeid(T)) throw std::runtime_error("allocation was already converted to a different pointer type");
            } else {
                ::new (value) T{};
                heapObjects.emplace(value, ObjectRecord{typeid(T), [](void* address) { std::destroy_at(static_cast<T*>(address)); }});
            }
        }
    }
    return static_cast<T*>(value);
}

class GcScopeGuard {
public:
    GcScopeGuard() { regions.push_back(std::make_shared<RegionArena>()); }
    GcScopeGuard(const GcScopeGuard&) = delete;
    GcScopeGuard& operator=(const GcScopeGuard&) = delete;
    ~GcScopeGuard() { if (!regions.empty()) regions.pop_back(); }
};
class GcContextScope {
public:
    explicit GcContextScope(const std::vector<std::shared_ptr<RegionArena>>& inherited)
        : previous(std::move(regions)) { regions = inherited; }
    GcContextScope(const GcContextScope&) = delete;
    GcContextScope& operator=(const GcContextScope&) = delete;
    ~GcContextScope() { regions = std::move(previous); }
private:
    std::vector<std::shared_ptr<RegionArena>> previous;
};
template<class F> class Defer {
public:
    explicit Defer(F action) : action(std::move(action)) {}
    Defer(const Defer&) = delete;
    Defer& operator=(const Defer&) = delete;
    Defer(Defer&& other) noexcept : action(std::move(other.action)), active(std::exchange(other.active, false)) {}
    ~Defer() noexcept { if (active) { try { action(); } catch (...) {} } }
private:
    F action;
    bool active = true;
};
template<class F> Defer(F) -> Defer<F>;
inline RawPointer alloc(std::size_t size) {
    if (!regions.empty()) return {regions.back()->allocate(size), size};
    auto allocation = std::make_unique<std::byte[]>(size);
    void* address = allocation.get();
    { std::lock_guard lock(heapAllocationMutex); heapAllocations.insert(address); }
    allocation.release();
    return {address, size};
}
inline RawPointer allocAligned(std::size_t size, std::size_t alignment) {
    if (alignment < alignof(void*) || (alignment & (alignment - 1)) != 0)
        throw std::invalid_argument("alignment must be a power of two at least alignof(void*)");
    if (!regions.empty()) return {regions.back()->allocate(size, alignment), size};
    void* address = ::operator new(size == 0 ? 1 : size, std::align_val_t(alignment));
    {
        std::lock_guard lock(heapAllocationMutex);
        heapAllocations.insert(address);
        heapAllocationAlignments[address] = alignment;
    }
    return {address, size};
}
inline void freeRaw(void* raw) {
    if (!raw) return;
    std::size_t alignment = 0;
    {
        std::lock_guard lock(heapAllocationMutex);
        if (!heapAllocations.erase(raw)) throw std::runtime_error("free requires a live allocation returned by alloc");
        if (auto found = heapAllocationAlignments.find(raw); found != heapAllocationAlignments.end()) {
            alignment = found->second;
            heapAllocationAlignments.erase(found);
        }
        if (auto object = heapObjects.find(raw); object != heapObjects.end()) {
            object->second.destroy(raw);
            heapObjects.erase(object);
        }
    }
    if (alignment) ::operator delete(raw, std::align_val_t(alignment));
    else delete[] static_cast<std::byte*>(raw);
}
template<class T, std::enable_if_t<std::is_integral_v<T>, int> = 0> inline RawPointer alloc(T size) {
    if constexpr (std::is_signed_v<T>) if (size < 0) throw std::runtime_error("allocation size cannot be negative");
    if (static_cast<std::uintmax_t>(size) > std::numeric_limits<std::size_t>::max()) throw std::length_error("allocation size is out of range");
    return alloc(static_cast<std::size_t>(size));
}
template<class T> void free(T*& item) {
    if (!item) return;
    void* raw = static_cast<void*>(item);
    for (auto& region : regions) if (region->owns(raw)) { region->destroyObject(raw); item = nullptr; return; }
    freeRaw(raw);
    item = nullptr;
}
template<class T> volatile std::int64_t& hardware(T address) {
    std::uintptr_t raw;
    if constexpr (std::is_pointer_v<T>) raw = reinterpret_cast<std::uintptr_t>(address);
    else raw = static_cast<std::uintptr_t>(address);
    return *reinterpret_cast<volatile std::int64_t*>(raw);
}
inline volatile std::int64_t& hardware(RawPointer address) { return *reinterpret_cast<volatile std::int64_t*>(address.value); }
inline std::int64_t floatToInt(double value) {
    if (!std::isfinite(value) || value < static_cast<double>(std::numeric_limits<std::int64_t>::min()) || value >= 9223372036854775808.0)
        return std::numeric_limits<std::int64_t>::min();
    return static_cast<std::int64_t>(std::trunc(value));
}
inline std::string floatFormat(double value, std::int64_t decimals) {
    if (decimals < 0 || decimals > 18) throw std::out_of_range("float format decimals must be between 0 and 18");
    std::ostringstream result;
    result << std::fixed << std::setprecision(static_cast<int>(decimals)) << value;
    return result.str();
}
inline std::mutex ioMutex;
inline void writeValue(const double& value) {
    auto precision = std::cout.precision(); std::cout << std::setprecision(std::numeric_limits<double>::max_digits10) << value; std::cout.precision(precision);
}
template<class T> void writeValue(const T& value) { std::cout << value; }
inline void write() {}
template<class T> void write(const T& value) { std::lock_guard lock(ioMutex); writeValue(value); }
template<class First, class... Rest> void write(const First& first, const Rest&... rest) { std::lock_guard lock(ioMutex); writeValue(first); (writeValue(rest), ...); }
template<class... Args> void writeln(const Args&... args) { std::lock_guard lock(ioMutex); (writeValue(args), ...); std::cout << '\n'; }
template<class T> void read(T& value) {
    std::lock_guard lock(ioMutex);
    if (!(std::cin >> value)) throw std::runtime_error("input read failed");
    const int next = std::cin.peek();
    if (next != std::char_traits<char>::eof() && std::isspace(static_cast<unsigned char>(next))) std::cin.get();
}
inline void readln(std::string& value) {
    std::lock_guard lock(ioMutex);
    if (!std::getline(std::cin, value)) throw std::runtime_error("input read failed");
}
template<class T> std::vector<T> slice(const std::vector<T>& values, std::size_t first, std::size_t last) {
    if (first > last || last > values.size()) throw std::out_of_range("array slice out of bounds");
    return std::vector<T>(values.begin() + static_cast<std::ptrdiff_t>(first), values.begin() + static_cast<std::ptrdiff_t>(last));
}
template<class T, class I> T& at(std::vector<T>& values, I index) {
    if constexpr (std::is_signed_v<I>) if (index < 0) throw std::out_of_range("negative array index");
    auto position = static_cast<std::size_t>(index); if (position >= values.size()) throw std::out_of_range("array index out of bounds"); return values[position];
}
template<class T, class I> const T& at(const std::vector<T>& values, I index) {
    if constexpr (std::is_signed_v<I>) if (index < 0) throw std::out_of_range("negative array index");
    auto position = static_cast<std::size_t>(index); if (position >= values.size()) throw std::out_of_range("array index out of bounds"); return values[position];
}
}
