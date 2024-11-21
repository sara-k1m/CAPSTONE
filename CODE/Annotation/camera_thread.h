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
* CameraThread 클래스:
* 카메라를 별도의 스레드에서 실행하기 위한 클래스 
* on_frame_을 통해 프레임 리스너를 추가하거나 제거 가능
*/
class CameraThread {
 public:
  CameraThread(); // 기본 생성자
  ~CameraThread(); // 소멸자

  //카메라 실행 메서드
  bool run(int camera_index = 0);

  void resume(); // 일시정지된 카메라를 다시 실행
  void pause(); // 카메라 일시정지

  void join(); // 스레드 종료 대기

  // 새로운 프레임이 도착하면 실행되는 시그널
  signal<void(cv::Mat frame)> on_frame_;

 private:
  void run_impl(); // 내부 스레드 실행 로직
  bool check_status(); // 상태 확인
  std::unique_lock<std::mutex> pause_wait(); // 일시정지 상태 대기

  int camera_index_ = 0; // 사용할 카메라 인덱스
  cv::VideoCapture video_; // OpenCV 비디오 캡처 객체
  cv::Mat frame_; // 현재 프레임 저장

  std::thread thread_; // 카메라 실행을 위한 스레드
  std::atomic_bool pause_{ true }; // 일시정지 상태를 나타내는 변수
  std::mutex mutex_; // 동기화를 위한 mutex
  std::condition_variable cv_; // 상태 변경 대기

  std::atomic_bool stop_{ false }; // 실행 종료 여부를 나타내는 변수
};

} // namespace sample

#endif // EYEDID_CPP_SAMPLE_CAMERA_THREAD_H_

