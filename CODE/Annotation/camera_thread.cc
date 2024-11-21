#include "camera_thread.h"

#include <iostream>
#include <thread>
#include <utility>

namespace sample {

// CameraThread 생성자
// - 객체 생성 시 새로운 스레드를 시작하고 run_impl() 메서드를 실행
CameraThread::CameraThread() {
  thread_ = std::thread([this](){
    run_impl();
  });
}

// CameraThread 소멸자
// - 객체가 소멸될 때 스레드를 종료(join)
CameraThread::~CameraThread() {
  join();
}

// 카메라 실행 메서드
// - 주어진 카메라 인덱스를 사용하여 카메라를 실행
// - 카메라 상태를 확인하고 pause를 해제하여 실행 시작
bool CameraThread::run(int camera_index) {
  if (camera_index == camera_index_) { // 이미 같은 카메라를 사용 중인 경우
    resume(); // 실행 상태로 전환
  }

  auto lck = pause_wait(); // pause 상태로 진입 및 대기
  camera_index_ = camera_index; // 새로운 카메라 인덱스 설정
  if (!check_status()) // 카메라 상태 확인
    return false; // 카메라 열기에 실패하면 false 반환
  lck.unlock(); // 잠금 해제

  pause_ = false; // pause 상태 해제
  cv_.notify_all(); // 대기 중인 스레드 깨우기

  return true;
}

// 카메라 일시정지 메서드
// - pause 상태로 전환하고 대기 중인 스레드에게 알림
void CameraThread::pause() {
  pause_ = true;
  cv_.notify_all();
}

// pause 상태 대기 메서드
// - pause 상태로 전환 후 mutex를 잠그고 대기
std::unique_lock<std::mutex> CameraThread::pause_wait() {
  pause_ = true; // pause 상태로 설정
  cv_.notify_all(); // 대기 중인 스레드 깨우기
  std::unique_lock<std::mutex> lck(mutex_); // mutex 잠금
  return lck; // 잠금 반환
}

// 카메라 재개 메서드
// - pause 상태를 해제하고 대기 중인 스레드 깨우기
void CameraThread::resume() {
  pause_ = false; // pause 상태 해제
  cv_.notify_all(); // 대기 중인 스레드 깨우기
}

// 스레드 실행 메서드 (실제 작업 수행)
// - pause 상태에서는 대기하고, 카메라 프레임을 읽어 이벤트(on_frame_)로 전달
void CameraThread::run_impl() {
  std::unique_lock<std::mutex> lck(mutex_); // mutex 잠금

  while (true) {
    // pause 상태가 해제되거나 stop 상태가 될 때까지 대기
    cv_.wait(lck, [this]() -> bool {
      return !pause_ || stop_;
    });

    if (stop_) // stop 상태면 루프 종료
      break;

    video_ >> frame_; // 카메라에서 프레임 읽기
    on_frame_(std::move(frame_)); // 프레임 이벤트 전달
  }
}

// 스레드 종료 대기 메서드
// - stop 상태로 설정하고 스레드가 종료될 때까지 대기
void CameraThread::join() {
  stop_.store(true, std::memory_order_release); // stop 상태 설정
  cv_.notify_all(); // 대기 중인 스레드 깨우기

  if (thread_.joinable()) // 스레드가 실행 중이면
    thread_.join(); // 스레드 종료 대기
}

// 카메라 상태 확인 메서드
// - 카메라를 열고 프레임을 성공적으로 가져올 수 있는지 확인
bool CameraThread::check_status() {
  video_.open(camera_index_); // 주어진 인덱스로 카메라 열기
  if (!video_.isOpened()) { // 카메라 열기에 실패한 경우
    std::cerr << "Failed to open camera\n";
    return false;
  } else if ((video_ >> frame_, frame_.empty())) { // 프레임을 가져오지 못한 경우
    std::cerr << "Camera is opened, but failed to get a frame. Try changing the camera_index\n";
    return false;
  }

  return true; // 성공적으로 열림
}

} // namespace sample
