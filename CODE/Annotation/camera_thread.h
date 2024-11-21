#ifndef EYEDID_CPP_SAMPLE_CAMERA_THREAD_H_
#define EYEDID_CPP_SAMPLE_CAMERA_THREAD_H_

#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include "opencv2/opencv.hpp"
#include "simple_signal.h"

namespace sample {

/**
* CameraThread Ŭ����:
* ī�޶� ������ �����忡�� �����ϱ� ���� Ŭ���� 
* on_frame_�� ���� ������ �����ʸ� �߰��ϰų� ���� ����
*/
class CameraThread {
 public:
  CameraThread(); // �⺻ ������
  ~CameraThread(); // �Ҹ���

  //ī�޶� ���� �޼���
  bool run(int camera_index = 0);

  void resume(); // �Ͻ������� ī�޶� �ٽ� ����
  void pause(); // ī�޶� �Ͻ�����

  void join(); // ������ ���� ���

  // ���ο� �������� �����ϸ� ����Ǵ� �ñ׳�
  signal<void(cv::Mat frame)> on_frame_;

 private:
  void run_impl(); // ���� ������ ���� ����
  bool check_status(); // ���� Ȯ��
  std::unique_lock<std::mutex> pause_wait(); // �Ͻ����� ���� ���

  int camera_index_ = 0; // ����� ī�޶� �ε���
  cv::VideoCapture video_; // OpenCV ���� ĸó ��ü
  cv::Mat frame_; // ���� ������ ����

  std::thread thread_; // ī�޶� ������ ���� ������
  std::atomic_bool pause_{ true }; // �Ͻ����� ���¸� ��Ÿ���� ����
  std::mutex mutex_; // ����ȭ�� ���� mutex
  std::condition_variable cv_; // ���� ���� ���

  std::atomic_bool stop_{ false }; // ���� ���� ���θ� ��Ÿ���� ����
};

} // namespace sample

#endif // EYEDID_CPP_SAMPLE_CAMERA_THREAD_H_

