#include "camera_thread.h"

#include <iostream>
#include <thread>
#include <utility>

namespace sample {

// CameraThread ������
// - ��ü ���� �� ���ο� �����带 �����ϰ� run_impl() �޼��带 ����
CameraThread::CameraThread() {
  thread_ = std::thread([this](){
    run_impl();
  });
}

// CameraThread �Ҹ���
// - ��ü�� �Ҹ�� �� �����带 ����(join)
CameraThread::~CameraThread() {
  join();
}

// ī�޶� ���� �޼���
// - �־��� ī�޶� �ε����� ����Ͽ� ī�޶� ����
// - ī�޶� ���¸� Ȯ���ϰ� pause�� �����Ͽ� ���� ����
bool CameraThread::run(int camera_index) {
  if (camera_index == camera_index_) { // �̹� ���� ī�޶� ��� ���� ���
    resume(); // ���� ���·� ��ȯ
  }

  auto lck = pause_wait(); // pause ���·� ���� �� ���
  camera_index_ = camera_index; // ���ο� ī�޶� �ε��� ����
  if (!check_status()) // ī�޶� ���� Ȯ��
    return false; // ī�޶� ���⿡ �����ϸ� false ��ȯ
  lck.unlock(); // ��� ����

  pause_ = false; // pause ���� ����
  cv_.notify_all(); // ��� ���� ������ �����

  return true;
}

// ī�޶� �Ͻ����� �޼���
// - pause ���·� ��ȯ�ϰ� ��� ���� �����忡�� �˸�
void CameraThread::pause() {
  pause_ = true;
  cv_.notify_all();
}

// pause ���� ��� �޼���
// - pause ���·� ��ȯ �� mutex�� ��װ� ���
std::unique_lock<std::mutex> CameraThread::pause_wait() {
  pause_ = true; // pause ���·� ����
  cv_.notify_all(); // ��� ���� ������ �����
  std::unique_lock<std::mutex> lck(mutex_); // mutex ���
  return lck; // ��� ��ȯ
}

// ī�޶� �簳 �޼���
// - pause ���¸� �����ϰ� ��� ���� ������ �����
void CameraThread::resume() {
  pause_ = false; // pause ���� ����
  cv_.notify_all(); // ��� ���� ������ �����
}

// ������ ���� �޼��� (���� �۾� ����)
// - pause ���¿����� ����ϰ�, ī�޶� �������� �о� �̺�Ʈ(on_frame_)�� ����
void CameraThread::run_impl() {
  std::unique_lock<std::mutex> lck(mutex_); // mutex ���

  while (true) {
    // pause ���°� �����ǰų� stop ���°� �� ������ ���
    cv_.wait(lck, [this]() -> bool {
      return !pause_ || stop_;
    });

    if (stop_) // stop ���¸� ���� ����
      break;

    video_ >> frame_; // ī�޶󿡼� ������ �б�
    on_frame_(std::move(frame_)); // ������ �̺�Ʈ ����
  }
}

// ������ ���� ��� �޼���
// - stop ���·� �����ϰ� �����尡 ����� ������ ���
void CameraThread::join() {
  stop_.store(true, std::memory_order_release); // stop ���� ����
  cv_.notify_all(); // ��� ���� ������ �����

  if (thread_.joinable()) // �����尡 ���� ���̸�
    thread_.join(); // ������ ���� ���
}

// ī�޶� ���� Ȯ�� �޼���
// - ī�޶� ���� �������� ���������� ������ �� �ִ��� Ȯ��
bool CameraThread::check_status() {
  video_.open(camera_index_); // �־��� �ε����� ī�޶� ����
  if (!video_.isOpened()) { // ī�޶� ���⿡ ������ ���
    std::cerr << "Failed to open camera\n";
    return false;
  } else if ((video_ >> frame_, frame_.empty())) { // �������� �������� ���� ���
    std::cerr << "Camera is opened, but failed to get a frame. Try changing the camera_index\n";
    return false;
  }

  return true; // ���������� ����
}

} // namespace sample
