#include <iostream>
#include <thread>
#include <stdexcept>
#include <memory>

#include <opencv2/opencv.hpp> // OpenCV 라이브러리 포함

#include "eyedid//gaze_tracker.h" // Eyedid SDK의 Gaze Tracker 헤더 파일 포함
#include "eyedid/util/display.h"  // Eyedid 디스플레이 유틸리티 포함

#include "tracker_manager.h" // 추적 관리자 관련 클래스
#include "view.h"            // GUI를 그리기 위한 클래스
#include "camera_thread.h"   // 카메라 스레드 구현 관련 클래스

#ifdef EYEDID_TEST_KEY
#  define EYEDID_STRINGFY_IMPL(x) #x
#  define EYEDID_STRINGFY(x) EYEDID_STRINGFY_IMPL(x)
const char* license_key = EYEDID_STRINGFY(EYEDID_TEST_KEY);
#else
const char* license_key = "YOUR LICENSE KEY HERE"; // 라이센스 키 설정
#endif

// Eyedid SDK를 활용한 GUI 구현 예제
// Eyedid SDK의 자세한 내용은 https://docs.eyedid.ai/ 를 참조

void printDisplays(const std::vector<eyedid::DisplayInfo>& displays);

int main() {
  // Eyedid 라이브러리 초기화
  try {
    eyedid::global_init();
  } catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
    return EXIT_FAILURE; // 초기화 실패 시 프로그램 종료
  }

  // 디스플레이 정보 가져오기
  const auto displays = eyedid::getDisplayLists();
  if (displays.empty()) {
    std::cerr << "Cannot find displays\n";
    return EXIT_FAILURE; // 디스플레이를 찾을 수 없으면 프로그램 종료
  }
  printDisplays(displays); // 디스플레이 정보 출력

  // Gaze Tracker 관리자 생성
  auto tracker_manager = std::make_shared<sample::TrackerManager>();
  auto tracker_manager_ptr = tracker_manager.get();

  // 추가 기능(사용자 상태 및 깜박임 감지) 옵션 설정
  EyedidTrackerOptions options;
  options.use_blink = kEyedidTrue;         // 깜박임 감지 활성화
  options.use_user_status = kEyedidTrue;  // 사용자 상태 감지 활성화

  // Gaze Tracker 초기화 및 인증
  auto code = tracker_manager->initialize(license_key, options);
  if (!code)
    return EXIT_FAILURE; // 초기화 실패 시 프로그램 종료

  // 카메라 좌표계를 디스플레이 픽셀 단위로 변환
  const auto& main_display = displays[0]; // 메인 디스플레이 선택
  tracker_manager->setDefaultCameraToDisplayConverter(main_display);

  // 전체 화면을 사용자의 관심 영역(ROI)으로 설정
  if (options.use_user_status) {
    tracker_manager->setWholeScreenToAttentionRegion(main_display);
  }

  // 카메라를 별도의 스레드에서 실행
  int camera_index = 0;
  sample::CameraThread camera_thread;
  if (!camera_thread.run(camera_index))
    return EXIT_FAILURE; // 카메라 실행 실패 시 프로그램 종료

  // GUI를 그릴 창 생성
  const char* window_name = "eyedid-sample";
  auto view = std::make_shared<sample::View>(
      main_display.widthPx * 2 / 3, main_display.heightPx * 2 / 3, window_name);
  auto view_ptr = view.get();
  tracker_manager->window_name_ = window_name;

  /// 이벤트 리스너 추가
  // 1. 사용자의 시선 위치 표시
  tracker_manager->on_gaze_.connect([=](int x, int y, bool valid) {
    sample::write_lock_guard lock(view_ptr->write_mutex());
    if (valid) {
      view_ptr->gaze_point_.center = {x, y};
      view_ptr->gaze_point_.color = {0, 220, 220}; // 유효한 시선: 청록색
    } else {
      view_ptr->gaze_point_.color = {0, 0, 220};   // 유효하지 않은 시선: 빨간색
    }
    view_ptr->gaze_point_.visible = true;
  }, view);

  // 2. 캘리브레이션 중 UI 상태 변경
  tracker_manager->on_calib_start_.connect([=]() {
    sample::write_lock_guard lock(view_ptr->write_mutex());
    view_ptr->calibration_desc_.visible = true; // 캘리브레이션 설명 표시
    for (auto& desc : view_ptr->desc_)
      desc.visible = false; // 다른 설명 숨기기
    view_ptr->frame_.visible = false; // 카메라 프레임 숨기기
  }, view);

  tracker_manager->on_calib_finish_.connect([=](const std::vector<float>& data) {
    sample::write_lock_guard lock(view_ptr->write_mutex());
    view_ptr->calibration_desc_.visible = false;
    view_ptr->calibration_point_.visible = false;
    for (auto& desc : view_ptr->desc_)
      desc.visible = true; // 설명 다시 표시
    view_ptr->frame_.visible = true; // 카메라 프레임 다시 표시
  }, view);

  // 3. 캘리브레이션 다음 지점 표시
  tracker_manager->on_calib_next_point_.connect([=](int x, int y) {
    sample::write_lock_guard lock(view_ptr->write_mutex());
    view_ptr->calibration_point_.center = {x, y};
    view_ptr->calibration_point_.visible = true;
    view_ptr->calibration_desc_.visible = false;
  }, view);

  tracker_manager->on_calib_progress_.connect([=](float progress) {
    std::cout << '\r' << progress * 100 << '%'; // 진행률 표시
  }, view);

  /// 카메라 프레임 리스너 추가
  // 1. 프레임을 GUI에 그리기
  camera_thread.on_frame_.connect([=](const cv::Mat& frame) {
    sample::write_lock_guard lock(view_ptr->write_mutex());
    cv::resize(frame, view_ptr->frame_.buffer, {640, 480});
  }, view);

  // 2. Eyedid SDK에 프레임 전달
  camera_thread.on_frame_.connect([=](const cv::Mat& frame) {
    static auto cvt = new cv::Mat();
    static const auto current_time = [] {
      using clock = std::chrono::steady_clock;
      return std::chrono::duration_cast<std::chrono::milliseconds>(clock::now().time_since_epoch()).count();
    };
    cv::cvtColor(frame, *cvt, cv::COLOR_BGR2RGB); // 프레임을 RGB로 변환
    tracker_manager_ptr->addFrame(current_time(), *cvt); // SDK에 전달
  }, tracker_manager);

  // ESC 키 또는 'C' 키를 눌러 프로그램 제어
  while (true) {
    int key = view->draw(10); // 10ms 간격으로 화면 갱신
    if (key == 27 /* ESC */) {
      break; // ESC 키로 종료
    } else if (key == 'c' || key == 'C') {
      tracker_manager->startFullWindowCalibration(
          kEyedidCalibrationPointFive,
          kEyedidCalibrationAccuracyDefault); // 캘리브레이션 시작
    }
  }
  view->closeWindow(); // 창 닫기

  return EXIT_SUCCESS;
}

// 디스플레이 정보를 출력하는 함수
void printDisplays(const std::vector<eyedid::DisplayInfo>& displays) {
  for (const auto& display : displays) {
    std::cout << "\nDisplay Name    : " << display.displayName
              << "\nDisplay String  : " << display.displayString
              << "\nDisplayFlags    : " << display.displayStateFlag
              << "\nDisplayId       : " << display.displayId
              << "\nDisplayKey      : " << display.displayKey
              << "\nSize(mm)        : " << display.widthMm << "x" << display.heightMm
              << "\nSize(px)        : " << display.widthPx << "x" << display.heightPx
              << "\n";
  }
}
