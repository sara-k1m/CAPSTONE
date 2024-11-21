#ifndef EYEDID_CPP_SAMPLE_SIMPLE_SIGNAL_H_
#define EYEDID_CPP_SAMPLE_SIMPLE_SIGNAL_H_

#include <list>
#include <memory>
#include <mutex>
#include <utility>

namespace sample {

/**
 * signal Ŭ����:
 * - boost::signal�� ������ ���
 * - �Լ�(����)�� �����ϰ� ȣ���ϴ� ��� ����
 */
template<typename F>
class signal;

// �̺�Ʈ ������ �⺻ Ŭ����
class slot_base {
 public:
  ~slot_base() = default;
  virtual void expire() = 0;
};

// �̺�Ʈ ������ �����ϴ� Ŭ���� ���ø�
template<typename R, typename ...Args>
class slot : public slot_base {
 public:
  template<typename F2>
  explicit slot(const F2& func) : func_(func) {} // ���Կ� �Լ� ���

  // ������ ȣ��� �� ����� �Լ�
  template<typename ...Ts>
  R operator()(Ts&&... args) const {
    return func_(std::forward<Ts>(args)...); // ��ϵ� �Լ� ����
  }

  void expire() override {
    expired_ = true; // ������ ���� ���·� ����
  }

  bool expired() const {
    return expired_; // ������ ����Ǿ����� Ȯ��
  }

 private:
  std::atomic_bool expired_{false}; // ���� ���� ���θ� ��Ÿ��
  std::function<R(Args...)> func_; // ���Կ� ����� �Լ�
};

/**
 * connection Ŭ����: Ư�� ���԰� ������ ����
 * - ������ ���� �� �ִ� ��� ����
 */
class connection {
 public:
  connection() = default;

  // ������ ���� �޼���
  void disconnect() {
    auto lock_ptr = slot_ptr_.lock(); // ���� �����͸� ���
    if (lock_ptr) {
      lock_ptr->expire(); // ������ ���� ���·� ����
    }
  }

 private:
  template<typename F> friend class signal;

  // ���� �����͸� �ʱ�ȭ�ϴ� ������
  template<typename ...T>
  explicit connection(std::shared_ptr<slot<T...>> ptr) : slot_ptr_(std::move(ptr)) {}

  template<typename ...T>
  connection& operator=(std::shared_ptr<slot<T...>> ptr) {
    slot_ptr_ = std::move(ptr); // ���� �����͸� ��ü
    return *this;
  }

  std::weak_ptr<slot_base> slot_ptr_; // ������ ���� ������
};

/**
 * raii_connection Ŭ����: �ڵ����� ������ ���� RAII ��� Ŭ����
 * - �Ҹ� �� ������ �ڵ����� ����
 */
class raii_connection {
 public:
  raii_connection() = default;
  ~raii_connection() { conn_.disconnect(); } // �Ҹ� �� ���� ����

  raii_connection(const raii_connection&) = delete;
  raii_connection& operator=(const raii_connection&) = delete;

  raii_connection(raii_connection&&) = default;
  raii_connection& operator=(raii_connection&&) = default;

  explicit raii_connection(connection&& conn) : conn_(std::move(conn)) {}
  raii_connection& operator=(connection&& conn) {
    conn_.disconnect(); // ���� ���� ����
    conn_ = std::move(conn); // �� ���� ����
    return *this;
  }
 private:
  connection conn_; // ���� ��ü
};

/**
 * signal Ŭ����: �̺�Ʈ ����� �˸��� ó��
 * - ��ϵ� �Լ�(����)�� ȣ���� �� �ִ� �̺�Ʈ �ý���
 *
 * @tparam R    �Լ� ��ȯ Ÿ��
 * @tparam Args �Լ� ���� Ÿ��
 */
template<typename R, typename ...Args>
class signal<R(Args...)> {
 public:
  using function_type = std::function<R(Args...)>; // �Լ� Ÿ�� ����
  using slot_type = slot<R, Args...>; // ���� Ÿ�� ����
  using slot_list = std::list<std::shared_ptr<slot_type>>; // ���� ����Ʈ

  /**
   * �Լ� ����
   * - �־��� �Լ��� ���Կ� ����
   * @param func ������ �Լ�
   * @return ���� ��ü(connection)
   */
  connection connect(function_type func) {
    auto new_slot = std::make_shared<slot_type>(std::move(func));
    {
      std::lock_guard<std::mutex> lck(connect_mutex_); // ����Ʈ ��ȣ
      slot_list_.emplace_back(new_slot); // ���� �߰�
    }

    return connection(new_slot); // ���� ��ü ��ȯ
  }

  /**
   * �Լ��� ���� ��ü�� ����
   * - ���� ��ü�� ������ ������ �Լ��� ȣ����� ����
   *
   * @tparam T    ���� ��ü Ÿ��
   * @param func  ������ �Լ�
   * @param track ������ ��ü
   * @return ���� ��ü(connection)
   */
  template<typename T>
  connection connect(function_type func, std::shared_ptr<T> track) {
    std::weak_ptr<void> weak_ptr = std::move(track); // ������ ��ü ����
    return connect([=](Args&&... args) {
      auto lck = weak_ptr.lock(); // ���� ��ü ���
      if (lck) {
        func(std::forward<Args>(args)...); // ���� ��ü�� ��ȿ�ϸ� �Լ� ����
      }
    });
  }

  /**
   * ����� �Լ� ȣ��
   * - ����� ���� ����Ʈ�� ��ȸ�ϸ� �Լ� ȣ��
   *
   * @tparam Args2 ȣ�⿡ ������ ���� Ÿ��
   * @param args   ȣ�⿡ ������ ����
   */
  template<typename ...Args2>
  void operator()(Args2&&... args) {
    std::unique_lock<std::mutex> lck(connect_mutex_); // ����Ʈ ��ȣ
    auto it = slot_list_.begin();

    while (it != slot_list_.end()) {
      if ((*it)->expired()) { // ����� ���� ����
        it = slot_list_.erase(it);
        continue;
      }

      lck.unlock(); // ����Ʈ ��ȣ ����
      (**it)(args...); // ���� ����
      lck.lock(); // �ٽ� ��ȣ
      ++it;
    }
  }

 private:
  mutable std::mutex connect_mutex_; // ���� ����Ʈ ��ȣ�� ���� mutex
  slot_list slot_list_; // ����� ���� ����Ʈ
};

} // namespace sample

#endif // EYEDID_CPP_SAMPLE_SIMPLE_SIGNAL_H_