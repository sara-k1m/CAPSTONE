#ifndef EYEDID_CPP_SAMPLE_PRIORITY_MUTEX_H_
#define EYEDID_CPP_SAMPLE_PRIORITY_MUTEX_H_

#include <atomic>
#include <condition_variable>
#include <mutex>

namespace sample {

// PriorityMutex 클래스
// 높은 우선순위와 낮은 우선순위를 구분하여 mutex 잠금을 관리합니다.
class PriorityMutex {
  // HighMutex 클래스
  // 높은 우선순위의 mutex 잠금을 관리하는 내부 클래스입니다.
  class HighMutex {
   public:
    // 생성자
    // PriorityMutex 객체를 참조로 받아 초기화합니다.
    explicit HighMutex(PriorityMutex& m) noexcept : m_(m) {}

    void lock();        // 높은 우선순위로 잠금을 수행합니다.
    void unlock();      // 높은 우선순위의 잠금을 해제합니다.
    bool try_to_lock(); // 높은 우선순위로 잠금을 시도합니다.

    // 복사 및 이동 연산 금지
    HighMutex(HighMutex const&) = delete;
    HighMutex(HighMutex &&) = delete;
    HighMutex& operator=(HighMutex const&) = delete;
    HighMutex& operator=(HighMutex &&) = delete;

   private:
    PriorityMutex& m_; // PriorityMutex 객체 참조
  };

  // LowMutex 클래스
  // 낮은 우선순위의 mutex 잠금을 관리하는 내부 클래스입니다.
  class LowMutex {
   public:
    // 생성자
    // PriorityMutex 객체를 참조로 받아 초기화합니다.
    explicit LowMutex(PriorityMutex& m) noexcept : m_(m) {}

    void lock();        // 낮은 우선순위로 잠금을 수행합니다.
    void unlock();      // 낮은 우선순위의 잠금을 해제합니다.
    bool try_to_lock(); // 낮은 우선순위로 잠금을 시도합니다.

    // 복사 및 이동 연산 금지
    LowMutex(LowMutex const&) = delete;
    LowMutex(LowMutex &&) = delete;
    LowMutex& operator=(LowMutex const&) = delete;
    LowMutex& operator=(LowMutex &&) = delete;

   private:
    PriorityMutex& m_; // PriorityMutex 객체 참조
  };

 public:
  // 타입 정의
  using mutex_type = std::mutex;          // 일반 mutex 타입
  using low_mutex_type = LowMutex;        // 낮은 우선순위 mutex 타입
  using high_mutex_type = HighMutex;      // 높은 우선순위 mutex 타입

  // 낮은 우선순위 mutex 관련 메서드
  void lock_low();        // 낮은 우선순위로 잠금을 수행합니다.
  void unlock_low();      // 낮은 우선순위의 잠금을 해제합니다.
  bool try_to_lock_low(); // 낮은 우선순위로 잠금을 시도합니다.
  LowMutex& low();        // LowMutex 객체를 반환합니다.

  // 높은 우선순위 mutex 관련 메서드
  void lock_high();        // 높은 우선순위로 잠금을 수행합니다.
  void unlock_high();      // 높은 우선순위의 잠금을 해제합니다.
  bool try_to_lock_high(); // 높은 우선순위로 잠금을 시도합니다.
  HighMutex& high();       // HighMutex 객체를 반환합니다.

 private:
  mutex_type m_;                  // mutex 객체
  std::condition_variable cv_;    // 조건 변수 (낮은 우선순위 대기 관리)
  std::atomic_int high_accessing_{0}; // 높은 우선순위 작업 수 (원자적 접근)

  low_mutex_type low_{*this};   // LowMutex 객체
  high_mutex_type high_{*this}; // HighMutex 객체
};

} // namespace sample

#endif // EYEDID_CPP_SAMPLE_PRIORITY_MUTEX_H_
