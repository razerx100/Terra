#ifndef SHARED_PTR_VECTOR_HPP_
#define SHARED_PTR_VECTOR_HPP_
#include <vector>
#include <memory>
#include <type_traits>

class SharedPtrAllocator
{
public:
    SharedPtrAllocator() : m_buffer{}, m_size{ 0u } {};

    void* Allocate(size_t size)
    {
        m_buffer = std::shared_ptr<std::uint8_t>{ new std::uint8_t[size] };
        m_size   = size;

        return m_buffer.get();
    }

    void Deallocate(void*, size_t)
    {
        // Should be just fine to deallocate.
        Reset();
    }

    void Reset() noexcept
    {
        m_size = 0u;
        m_buffer.reset();
    }

    [[nodiscard]]
    size_t Size() const noexcept { return m_size; }

    [[nodiscard]]
    std::shared_ptr<std::uint8_t> Get() const noexcept { return m_buffer; }

private:
    std::shared_ptr<std::uint8_t> m_buffer;
    size_t                        m_size;

public:
    // This kinda copying isn't ideal but should be fine, since I won't actually make
    // duplicate vectors.
    SharedPtrAllocator(const SharedPtrAllocator& other) noexcept
        : m_buffer{ other.m_buffer }, m_size{ other.m_size }
    {}
    SharedPtrAllocator& operator=(const SharedPtrAllocator& other) noexcept
    {
        m_buffer = other.m_buffer;
        m_size   = other.m_size;

        return *this;
    }

    SharedPtrAllocator(SharedPtrAllocator&& other) noexcept
        : m_buffer{ std::move(other.m_buffer) }, m_size{ other.m_size }
    {}
    SharedPtrAllocator& operator=(SharedPtrAllocator&& other) noexcept
    {
        m_buffer = std::move(other.m_buffer);
        m_size   = other.m_size;

        return *this;
    }
};

template<typename T>
class AllocatorSharedVec
{
    template<typename U>
    friend class AllocatorSharedVec;

public:
    typedef T value_type;
    typedef size_t size_type;
    typedef std::ptrdiff_t  difference_type;
    typedef T* pointer;
    typedef const T* const_pointer;
    typedef T& reference;
    typedef const T& const_reference;

    template<typename U>
    struct rebind { typedef AllocatorSharedVec<U> other; };

    AllocatorSharedVec() : m_allocator{} {}

    AllocatorSharedVec(const AllocatorSharedVec& alloc) noexcept : m_allocator{ alloc.m_allocator } {}
    AllocatorSharedVec(AllocatorSharedVec&& alloc) noexcept : m_allocator{ std::move(alloc.m_allocator) } {}

    template<typename U>
    AllocatorSharedVec(const AllocatorSharedVec<U>& alloc) noexcept : m_allocator{ alloc.m_allocator } {}
    template<typename U>
    AllocatorSharedVec(AllocatorSharedVec<U>&& alloc) noexcept : m_allocator{ std::move(alloc.m_allocator) } {}

    AllocatorSharedVec& operator=(AllocatorSharedVec&& alloc) noexcept
    {
        m_allocator = std::move(alloc.m_allocator);

        return *this;
    }

    AllocatorSharedVec& operator=(const AllocatorSharedVec& alloc) noexcept
    {
        m_allocator = alloc.m_allocator;

        return *this;
    }

    template<typename U>
    friend bool operator==(const AllocatorSharedVec<T>& lhs, const AllocatorSharedVec<U>& rhs) noexcept
    {
        return lhs.m_allocator.Get() == rhs.m_allocator.Get();
    }

    template<typename U>
    friend bool operator!=(const AllocatorSharedVec<T>& lhs, const AllocatorSharedVec<U>& rhs) noexcept
    {
        return lhs.m_allocator.Get() != rhs.m_allocator.Get();
    }

    pointer allocate(size_type size)
    {
        return static_cast<pointer>(m_allocator.Allocate(size * sizeof(T)));
    }

    void deallocate(pointer ptr, size_type size)
    {
        m_allocator.Deallocate(ptr, size * sizeof(T));
    }

    template<typename X, typename... Args>
    void construct(X* ptr, Args&&... args)
        noexcept(std::is_nothrow_constructible<X, Args...>::value)
    {
        ::new(ptr) X(std::forward<Args>(args)...);
    }

    template<typename X>
    void destroy(X* ptr) noexcept(std::is_nothrow_destructible<X>::value) { ptr->~X(); }

    size_type max_size() const noexcept
    {
        return m_allocator.Size();
    }

    [[nodiscard]]
    std::shared_ptr<std::uint8_t> GetSharedPtr() const noexcept { return m_allocator.Get(); }

    void Reset() noexcept { m_allocator.Reset(); }

private:
    SharedPtrAllocator m_allocator;
};

template<typename T>
class SharedPtrVector
{
public:
    SharedPtrVector() : m_allocator{}, m_vector{ m_allocator } {}

    auto& GetVector() noexcept { return m_vector; }
    const auto& GetVector() const noexcept { return m_vector; }

    [[nodiscard]]
    std::shared_ptr<std::uint8_t> GetSharedPtr() const noexcept { return m_allocator.GetSharedPtr(); }

    void Reset() noexcept
    {
        m_allocator.Reset();
        m_vector = std::vector<T, AllocatorSharedVec<T>>{ m_allocator };
    }

    [[nodiscard]]
    size_t size() const noexcept { return std::size(m_vector); }

private:
    AllocatorSharedVec<T>                 m_allocator;
    std::vector<T, AllocatorSharedVec<T>> m_vector;

public:
    SharedPtrVector(const SharedPtrVector&) = delete;
    SharedPtrVector& operator=(const SharedPtrVector&) = delete;

    SharedPtrVector(SharedPtrVector&& other) noexcept
        : m_allocator{ std::move(other.m_allocator) }, m_vector{ std::move(other.m_vector) }
    {}
    SharedPtrVector& operator=(SharedPtrVector&& other) noexcept
    {
        m_allocator = std::move(other.m_allocator);
        m_vector    = std::move(other.m_vector);

        return *this;
    }
};

template<typename T>
std::shared_ptr<std::uint8_t> CopyVectorToSharedPtr(const std::vector<T>& container) noexcept
{
	const auto bufferSize = std::size(container) * sizeof(T);

	auto dataBuffer = std::shared_ptr<std::uint8_t>{ new std::uint8_t[bufferSize] };
	memcpy(dataBuffer.get(), std::data(container), bufferSize);

	return dataBuffer;
}
#endif
