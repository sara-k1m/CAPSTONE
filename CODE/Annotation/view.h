/*
 *
 * 이 클래스는 OpenCV를 사용하여 창을 생성 및 제거하고,
 * 카메라 프레임과 시선 추적 점(가상 시선)을 그리기 위한 기능을 제공합니다.
 */

#ifndef EYEDID_CPP_SAMPLE_VIEW_H_
#define EYEDID_CPP_SAMPLE_VIEW_H_

#include <string> // 문자열 처리를 위한 헤더
#include <vector> // 텍스트 설명과 같은 요소들을 저장할 벡터 자료구조 포함

#include "opencv2/opencv.hpp" // OpenCV 라이브러리 사용
#include "drawables.h" // 화면에 그릴 도형 및 텍스트 요소에 대한 정의 포함
#include "priority_mutex.h" // 동기화를 위한 사용자 정의 뮤텍스 정의 포함

namespace sample {

/**
 * 읽기 및 쓰기 락을 정의:
 * - 읽기 락은 우선순위가 높은 mutex를 사용
 * - 쓰기 락은 우선순위가 낮은 mutex를 사용
 */
using read_lock_guard = std::lock_guard<typename PriorityMutex::high_mutex_type>;
using read_unique_lock = std::unique_lock<typename PriorityMutex::high_mutex_type>;
using write_lock_guard = std::lock_guard<typename PriorityMutex::low_mutex_type>;
using write_unique_lock = std::unique_lock<typename PriorityMutex::low_mutex_type>;

/**
 * View 클래스 정의:
 * - OpenCV를 기반으로 창을 생성하고 화면 요소를 표시합니다.
 * - 시선 추적 및 캘리브레이션 관련 화면 요소도 처리합니다.
 */
class View {
 public:
  /**
   * 생성자
   * @param width 창의 너비
   * @param height 창의 높이
   * @param windowName 창 이름
   */
  View(int width, int height, std::string windowName);

  /**
   * 시선 좌표를 설정하는 함수
   * @param x 시선의 x좌표
   * @param y 시선의 y좌표
   */
  void setPoint(int x, int y);

  /**
   * 카메라에서 받은 프레임을 설정하는 함수
   * @param frame OpenCV에서 캡처한 프레임(cv::Mat 형식)
   */
  void setFrame(const cv::Mat& frame);

  /**
   * 창을 그리는 함수
   * @param wait_ms 키 입력 대기 시간 (기본값 10ms)
   * @return 눌린 키 값
   */
  int draw(int wait_ms = 10);

  /**
   * 생성된 창을 닫는 함수
   */
  void closeWindow();

  /**
   * 창 이름을 반환하는 함수
   * @return 윈도우 이름(const 참조)
   */
  const std::string& getWindowName() const;

  /**
   * 공용 멤버:
   * - gaze_point_: 시선을 나타내는 원
   * - calibration_point_: 캘리브레이션을 위한 빨간색 원
   * - calibration_desc_: 캘리브레이션 시 보여주는 텍스트
   * - frame_: 카메라로 받은 프레임 이미지
   * - desc_: 화면 하단에 표시할 설명 텍스트 목록
   */
  drawables::Circle gaze_point_;
  drawables::Circle calibration_point_;
  drawables::Text calibration_desc_;
  drawables::Image frame_;
  std::vector<drawables::Text> desc_;

  /**
   * 쓰기 mutex에 대한 참조를 반환
   * @return 우선순위가 낮은 mutex 참조
   */
  PriorityMutex::low_mutex_type& write_mutex() { return mutex_.low(); }

 private:
  /**
   * 내부 메서드:
   * - 초기화, 그리기, 화면 초기화 등을 담당
   */

  // 화면 요소를 초기화하는 메서드
  void initElements();

  // 배경 이미지를 초기화하는 메서드 (검정색으로 설정)
  void clearBackground();

  // 등록된 화면 요소를 그리는 메서드
  void drawElements();

  /**
   * 창을 표시하고 키 입력을 대기하는 메서드
   * @param wait_ms 키 입력 대기 시간(ms)
   * @return 눌린 키 값
   */
  int drawWindow(int wait_ms);

  /**
   * 읽기 mutex에 대한 참조를 반환
   * @return 우선순위가 높은 mutex 참조
   */
  PriorityMutex::high_mutex_type& read_mutex() { return mutex_.high(); }

  /**
   * 비공용 멤버:
   * - window_name_: 창 이름
   * - background_: 화면 배경(cv::Mat 형식)
   * - mutex_: 동기화를 위한 우선순위 뮤텍스
   */
  std::string window_name_; // 창 이름
  cv::Mat background_; // 화면 배경 이미지
  mutable PriorityMutex mutex_; // 동기화를 위한 mutable mutex
};

} // namespace sample

#endif // EYEDID_CPP_SAMPLE_VIEW_H_
