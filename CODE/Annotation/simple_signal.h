#ifndef EYEDID_CPP_SAMPLE_SIMPLE_SIGNAL_H_
#define EYEDID_CPP_SAMPLE_SIMPLE_SIGNAL_H_

#include <list>
#include <memory>
#include <mutex>
#include <utility>

namespace sample {

/**
 * signal 클래스:
 * - boost::signal의 간단한 모방
 * - 함수(슬롯)를 연결하고 호출하는 기능 제공
 */
template<typename F>
class signal;

// 이벤트 슬롯의 기본 클래스
class slot_base {
 public:
  ~slot_base() = default;
  virtual void expire() = 0;
};

// 이벤트 슬롯을 정의하는 클래스 템플릿
template<typename R, typename ...Args>
class slot : public slot_base {
 public:
  template<typename F2>
  explicit slot(const F2& func) : func_(func) {} // 슬롯에 함수 등록

  // 슬롯이 호출될 때 실행될 함수
  template<typename ...Ts>
  R operator()(Ts&&... args) const {
    return func_(std::forward<Ts>(args)...); // 등록된 함수 실행
  }

  void expire() override {
    expired_ = true; // 슬롯을 만료 상태로 설정
  }

  bool expired() const {
    return expired_; // 슬롯이 만료되었는지 확인
  }

 private:
  std::atomic_bool expired_{false}; // 슬롯 만료 여부를 나타냄
  std::function<R(Args...)> func_; // 슬롯에 연결된 함수
};

/**
 * connection 클래스: 특정 슬롯과 연결을 관리
 * - 연결을 끊을 수 있는 기능 제공
 */
class connection {
 public:
  connection() = default;

  // 연결을 끊는 메서드
  void disconnect() {
    auto lock_ptr = slot_ptr_.lock(); // 약한 포인터를 잠금
    if (lock_ptr) {
      lock_ptr->expire(); // 슬롯을 만료 상태로 설정
    }
  }

 private:
  template<typename F> friend class signal;

  // 슬롯 포인터를 초기화하는 생성자
  template<typename ...T>
  explicit connection(std::shared_ptr<slot<T...>> ptr) : slot_ptr_(std::move(ptr)) {}

  template<typename ...T>
  connection& operator=(std::shared_ptr<slot<T...>> ptr) {
    slot_ptr_ = std::move(ptr); // 슬롯 포인터를 교체
    return *this;
  }

  std::weak_ptr<slot_base> slot_ptr_; // 슬롯의 약한 포인터
};

/**
 * raii_connection 클래스: 자동으로 연결을 끊는 RAII 기반 클래스
 * - 소멸 시 연결을 자동으로 해제
 */
class raii_connection {
 public:
  raii_connection() = default;
  ~raii_connection() { conn_.disconnect(); } // 소멸 시 연결 해제

  raii_connection(const raii_connection&) = delete;
  raii_connection& operator=(const raii_connection&) = delete;

  raii_connection(raii_connection&&) = default;
  raii_connection& operator=(raii_connection&&) = default;

  explicit raii_connection(connection&& conn) : conn_(std::move(conn)) {}
  raii_connection& operator=(connection&& conn) {
    conn_.disconnect(); // 기존 연결 해제
    conn_ = std::move(conn); // 새 연결 설정
    return *this;
  }
 private:
  connection conn_; // 연결 객체
};

/**
 * signal 클래스: 이벤트 연결과 알림을 처리
 * - 등록된 함수(슬롯)를 호출할 수 있는 이벤트 시스템
 *
 * @tparam R    함수 반환 타입
 * @tparam Args 함수 인자 타입
 */
template<typename R, typename ...Args>
class signal<R(Args...)> {
 public:
  using function_type = std::function<R(Args...)>; // 함수 타입 정의
  using slot_type = slot<R, Args...>; // 슬롯 타입 정의
  using slot_list = std::list<std::shared_ptr<slot_type>>; // 슬롯 리스트

  /**
   * 함수 연결
   * - 주어진 함수를 슬롯에 연결
   * @param func 연결할 함수
   * @return 연결 객체(connection)
   */
  connection connect(function_type func) {
    auto new_slot = std::make_shared<slot_type>(std::move(func));
    {
      std::lock_guard<std::mutex> lck(connect_mutex_); // 리스트 보호
      slot_list_.emplace_back(new_slot); // 슬롯 추가
    }

    return connection(new_slot); // 연결 객체 반환
  }

  /**
   * 함수와 추적 객체를 연결
   * - 추적 객체의 수명이 끝나면 함수가 호출되지 않음
   *
   * @tparam T    추적 객체 타입
   * @param func  연결할 함수
   * @param track 추적할 객체
   * @return 연결 객체(connection)
   */
  template<typename T>
  connection connect(function_type func, std::shared_ptr<T> track) {
    std::weak_ptr<void> weak_ptr = std::move(track); // 추적할 객체 저장
    return connect([=](Args&&... args) {
      auto lck = weak_ptr.lock(); // 추적 객체 잠금
      if (lck) {
        func(std::forward<Args>(args)...); // 추적 객체가 유효하면 함수 실행
      }
    });
  }

  /**
   * 연결된 함수 호출
   * - 연결된 슬롯 리스트를 순회하며 함수 호출
   *
   * @tparam Args2 호출에 전달할 인자 타입
   * @param args   호출에 전달할 인자
   */
  template<typename ...Args2>
  void operator()(Args2&&... args) {
    std::unique_lock<std::mutex> lck(connect_mutex_); // 리스트 보호
    auto it = slot_list_.begin();

    while (it != slot_list_.end()) {
      if ((*it)->expired()) { // 만료된 슬롯 제거
        it = slot_list_.erase(it);
        continue;
      }

      lck.unlock(); // 리스트 보호 해제
      (**it)(args...); // 슬롯 실행
      lck.lock(); // 다시 보호
      ++it;
    }
  }

 private:
  mutable std::mutex connect_mutex_; // 연결 리스트 보호를 위한 mutex
  slot_list slot_list_; // 연결된 슬롯 리스트
};

} // namespace sample

#endif // EYEDID_CPP_SAMPLE_SIMPLE_SIGNAL_H_