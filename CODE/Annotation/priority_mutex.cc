#include "priority_mutex.h"

namespace sample {

// **HighMutex 클래스**
// 높은 우선순위의 잠금(mutex)을 관리하는 클래스입니다.
void PriorityMutex::HighMutex::lock() {
  // 높은 우선순위로 mutex를 잠급니다.
  return m_.lock_high();
}

void PriorityMutex::HighMutex::unlock() {
  // 높은 우선순위의 mutex 잠금을 해제합니다.
  return m_.unlock_high();
}

bool PriorityMutex::HighMutex::try_to_lock() {
  // 높은 우선순위로 mutex를 시도하여 잠급니다.
  return m_.try_to_lock_high();
}

// **LowMutex 클래스**
// 낮은 우선순위의 잠금(mutex)을 관리하는 클래스입니다.
void PriorityMutex::LowMutex::lock() {
  // 낮은 우선순위로 mutex를 잠급니다.
  m_.lock_low();
}

void PriorityMutex::LowMutex::unlock() {
  // 낮은 우선순위의 mutex 잠금을 해제합니다.
  m_.unlock_low();
}

bool PriorityMutex::LowMutex::try_to_lock() {
  // 낮은 우선순위로 mutex를 시도하여 잠급니다.
  return m_.try_to_lock_low();
}

// **PriorityMutex 클래스**
// 높은 우선순위와 낮은 우선순위를 구분하여 mutex를 제어하는 클래스입니다.
void PriorityMutex::lock_low() {
  // 낮은 우선순위로 잠금을 시도합니다.
  // 만약 높은 우선순위의 작업이 진행 중이라면, 조건 변수를 사용하여 대기합니다.
  std::unique_lock<mutex_type> lck(m_);
  cv_.wait(lck, [this]{
    // 높은 우선순위 작업이 없을 때만 진행
    return !high_accessing_;
  });
  lck.release(); // 잠금을 해제합니다.
}

void PriorityMutex::unlock_low() {
  // 낮은 우선순위 잠금을 해제하고 조건 변수를 알립니다.
  m_.unlock();
  cv_.notify_one();
}

bool PriorityMutex::try_to_lock_low() {
  // 높은 우선순위 작업이 없고, mutex를 잠글 수 있으면 true를 반환합니다.
  return !high_accessing_ && m_.try_lock();
}

void PriorityMutex::lock_high() {
  // 높은 우선순위로 잠금을 시도하며, 이 작업이 진행 중임을 표시합니다.
  ++high_accessing_;
  m_.lock();
  --high_accessing_; // 작업이 끝난 후 high_accessing_을 감소시킵니다.
}

void PriorityMutex::unlock_high() {
  // 높은 우선순위 잠금을 해제하고 조건 변수를 알립니다.
  m_.unlock();
  cv_.notify_one();
}

bool PriorityMutex::try_to_lock_high() {
  // 높은 우선순위로 잠금을 시도합니다.
  ++high_accessing_;
  if (m_.try_lock()) {
    --high_accessing_; // 잠금 성공 시 high_accessing_을 감소시킵니다.
    return true;
  }
  return false; // 잠금 실패 시 false를 반환합니다.
}

PriorityMutex::HighMutex& PriorityMutex::high() {
  // 높은 우선순위 mutex에 접근할 수 있는 객체를 반환합니다.
  return high_;
}

PriorityMutex::LowMutex& PriorityMutex::low() {
  // 낮은 우선순위 mutex에 접근할 수 있는 객체를 반환합니다.
  return low_;
}

} // namespace sample
