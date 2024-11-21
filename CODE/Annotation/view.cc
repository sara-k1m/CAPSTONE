#include "view.h"

#include <cstring> // memset 사용을 위해 포함
#include <algorithm> // std::move 등 알고리즘 관련 기능을 위해 포함
#include <utility> // std::move와 같은 유틸리티 기능 사용을 위해 포함

namespace sample {

// View 클래스 생성자
// 주어진 너비와 높이로 배경 이미지를 초기화하고, 윈도우 이름을 설정한 뒤, OpenCV 창을 생성하고 초기 요소를 설정함
View::View(int width, int height, std::string windowName)
: background_(height, width, CV_8UC3, {0, 0, 0}), // 배경 이미지를 검정색으로 초기화
  window_name_(std::move(windowName)) { // 윈도우 이름을 설정 (std::move로 효율적으로 전달)
  cv::namedWindow(window_name_); // OpenCV 윈도우 생성
  initElements(); // 화면에 표시할 기본 요소 초기화
}

// 사용자의 시선을 나타내는 점의 좌표를 설정
void View::setPoint(int x, int y) {
  gaze_point_.center = {x, y}; // 중심 좌표 설정
}

// 화면에 표시할 프레임을 설정
void View::setFrame(const cv::Mat& frame) {
  frame_.buffer = frame; // 프레임 데이터를 내부 버퍼에 복사
}

// 화면을 그리는 메서드
int View::draw(int wait_ms) {
  clearBackground(); // 배경 초기화 (검정색으로 지움)
  drawElements(); // 요소들을 화면에 그림
  return drawWindow(wait_ms); // 화면 출력 및 키 입력 대기
}

// 윈도우를 닫는 메서드
void View::closeWindow() {
  cv::destroyWindow(window_name_); // OpenCV 윈도우 닫기
}

// 윈도우 이름 반환 (const 참조로 반환하여 불필요한 복사를 방지)
const std::string& View::getWindowName() const {
  return window_name_;
}

// 화면에 그릴 요소들을 초기화하는 메서드
void View::initElements() {
  // 시선 표시점 설정
  gaze_point_.color = {0, 220, 220}; // 청록색으로 설정

  // 캘리브레이션 점 초기화
  calibration_point_.visible = false; // 기본적으로 보이지 않음
  calibration_point_.color = {0, 0, 255}; // 빨간색으로 설정
  calibration_point_.radius = 50; // 반지름 설정

  // 캘리브레이션 설명 초기화
  calibration_desc_.text = "Stare at the red circle until it disappears or moves to other place.";
  calibration_desc_.org = {background_.cols / 2, background_.rows / 2}; // 중앙에 위치
  calibration_desc_.visible = false; // 기본적으로 보이지 않음

  // 프레임 기본 크기 설정
  frame_.size = {480, 320};

  // 화면 하단 설명 초기화
  desc_.resize(2); // 설명 텍스트 2개를 저장
  desc_[0].text = "Press ESC to exit program, Press 'C' to start calibration"; // 첫 번째 설명
  desc_[1].text = "Do not resize the window manually after created"; // 두 번째 설명
  desc_[1].color = {0, 0, 220}; // 파란색으로 표시

  // 설명 텍스트 위치와 글자 크기 설정
  for (int i = 0; i < desc_.size(); ++i) {
    desc_[desc_.size() - 1 - i].fontScale = 1.5; // 글자 크기
    desc_[desc_.size() - 1 - i].org = {50, background_.rows - 50 * (i + 1)}; // 위치
  }
}

// 배경 초기화 메서드
void View::clearBackground() {
  std::memset(background_.data, 0, background_.rows * background_.cols * background_.channels());
  // OpenCV의 메모리 데이터 직접 초기화하여 모든 픽셀을 검정색으로 설정
}

// 화면에 그릴 요소들을 순서대로 그리는 메서드
void View::drawElements() {
  read_lock_guard lock(read_mutex()); // 멀티스레드 환경에서 안전한 읽기 보호

  // 각 요소를 배경에 그리기
  drawables::draw_if(frame_, &background_); // 프레임 그리기
  drawables::draw_if(gaze_point_, &background_); // 시선 점 그리기
  drawables::draw_if(calibration_point_, &background_); // 캘리브레이션 점 그리기
  drawables::draw_if(calibration_desc_, &background_); // 캘리브레이션 설명 그리기
  for (const auto& desc : desc_)
    drawables::draw_if(desc, &background_); // 설명 텍스트 그리기
}

// 화면을 출력하고 키 입력을 기다리는 메서드
int View::drawWindow(int wait_ms) {
  cv::imshow(window_name_, background_); // 배경 이미지를 윈도우에 표시
  int key = cv::waitKey(wait_ms); // 주어진 시간(ms) 동안 키 입력 대기
  return key; // 입력된 키 값을 반환
}

} // namespace sample
