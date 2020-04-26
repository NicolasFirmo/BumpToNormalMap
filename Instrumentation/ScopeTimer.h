#pragma once
#include <iostream>
#include <chrono>
#include <mutex>
#include "Macros.h"

namespace nic {

	std::mutex s_TimerMutex;

	using nanoseconds = std::chrono::nanoseconds;
	using microseconds = std::chrono::microseconds;
	using milliseconds = std::chrono::milliseconds;
	using seconds = std::chrono::seconds;
	using minutes = std::chrono::minutes;
	using hours = std::chrono::hours;

	template<typename D = microseconds>
	class ScopeTimerOStream
	{
	public:
		constexpr ScopeTimerOStream(const char* scopeName = "Scope", std::ostream& os = std::cout) noexcept;
		~ScopeTimerOStream();
	private:
		const char* m_ScopeName;
		struct TimeUnity;
		std::ostream& m_Os;
		std::chrono::time_point<std::chrono::steady_clock> m_StartTimePoint;
	};

	template<typename T, typename D = microseconds>
	class ScopeTimerReference
	{
	public:
		constexpr ScopeTimerReference(T& number) noexcept;
		~ScopeTimerReference();
	private:
		T& m_Number;
		std::chrono::time_point<std::chrono::steady_clock> m_StartTimePoint;
	};

	template<>struct ScopeTimerOStream<nanoseconds>::TimeUnity { static constexpr const char* value = "ns\n"; };
	template<>struct ScopeTimerOStream<microseconds>::TimeUnity { static constexpr const char* value = "us\n"; };
	template<>struct ScopeTimerOStream<milliseconds>::TimeUnity { static constexpr const char* value = "ms\n"; };
	template<>struct ScopeTimerOStream<seconds>::TimeUnity { static constexpr const char* value = "s\n"; };
	template<>struct ScopeTimerOStream<minutes>::TimeUnity { static constexpr const char* value = "m\n"; };
	template<>struct ScopeTimerOStream<hours>::TimeUnity { static constexpr const char* value = "h\n"; };

	template<typename D>
	constexpr ScopeTimerOStream<D>::ScopeTimerOStream(const char* scopeName, std::ostream& os) noexcept
		: m_ScopeName(scopeName), m_Os(os), m_StartTimePoint(std::chrono::steady_clock::now())
	{
	}
	template<typename D>
	ScopeTimerOStream<D>::~ScopeTimerOStream()
	{
		auto endTimePoint = std::chrono::steady_clock::now();
		auto duration = std::chrono::duration_cast<D>(endTimePoint - m_StartTimePoint).count();
		s_TimerMutex.lock();
		m_Os << m_ScopeName << " took:\t" << duration << TimeUnity::value;
		s_TimerMutex.unlock();
	}

	template<typename T, typename D>
	constexpr ScopeTimerReference<T, D>::ScopeTimerReference(T& number) noexcept
		: m_Number(number), m_StartTimePoint(std::chrono::steady_clock::now())
	{
	}
	template<typename T, typename D>
	ScopeTimerReference<T, D>::~ScopeTimerReference()
	{
		auto endTimePoint = std::chrono::steady_clock::now();
		auto duration = std::chrono::duration_cast<D>(endTimePoint - m_StartTimePoint).count();
		static_assert(std::is_same<decltype(duration), T>::value, "Must be int64_t, otherwise, if on Windows and D is above seconds, it must be int");
		m_Number = duration;
	}

} // namespace nic

#define NIC_TIMER 0
#if NIC_TIMER
#define NIC_TIME(x) { nic::ScopeTimerOStream<> timer(#x, std::clog); x; }
#define NIC_FUNCTION_TIMER_STATUS(duration,...) ::nic::ScopeTimerStatus<duration> timer##__LINE__(NIC_FUNC_SIG,__VA_ARGS__)
#define NIC_FUNCTION_TIMER_OSTREAM(duration,...) ::nic::ScopeTimerOStream<duration> timer##__LINE__(NIC_FUNC_SIG,__VA_ARGS__)
#else
#define NIC_TIME(x) x
#define NIC_FUNCTION_TIMER_STATUS(duration,...) 
#define NIC_FUNCTION_TIMER_OSTREAM(duration,...)
#endif // NIC_TIMER