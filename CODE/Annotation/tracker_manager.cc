#include "tracker_manager.h"

#include <iostream> // 디버깅 출력을 위한 I/O 라이브러리
#include <utility>  // std::move 등 유틸리티 함수 포함
#include <vector>   // 벡터 자료구조 사용

#include "eyedid/util/display.h" // 디스플레이 정보를 가져오기 위한 라이브러리

namespace sample {

/**
 * 창의 크기와 패딩을 기반으로 영역(Rect)을 반환
 * @param window_name 창 이름
 * @param padding 패딩 값 (기본값: 30)
 * @return 패딩이 적용된 창의 영역 좌표 (벡터 형태로 반환: {x1, y1, x2, y2})
 */
static std::vector<float> getWindowRectWithPadding(const char* window_name, int padding = 30) {
  const auto window_rect = eyedid::getWindowRect(window_name); // 창의 현재 위치와 크기 가져오기
  return {
    static_cast<float>(window_rect.x + padding), // 좌측 상단 x 좌표 + 패딩
    static_cast<float>(window_rect.y + padding), // 좌측 상단 y 좌표 + 패딩
    static_cast<float>(window_rect.x + window_rect.width - padding), // 우측 하단 x 좌표 - 패딩
    static_cast<float>(window_rect.y + window_rect.height - padding) // 우측 하단 y 좌표 - 패딩
  };
}

/**
 * TrackerManager 클래스의 OnMetrics 메서드:
 * 다양한 추적 데이터를 처리하여 개별 데이터 처리 메서드로 전달
 */
void TrackerManager::OnMetrics(uint64_t timestamp,
                              const EyedidGazeData &gaze_data,
                              const EyedidFaceData &face_data,
                              const EyedidBlinkData &blink_data,
                              const EyedidUserStatusData &user_status_data) {
  // 각 데이터를 해당 처리 메서드로 전달
  this->OnGaze(timestamp, gaze_data.x, gaze_data.y, gaze_data.fixation_x, gaze_data.fixation_y,
               gaze_data.tracking_state, gaze_data.movement_state);
  this->OnFace(timestamp, face_data.score, face_data.left, face_data.top, face_data.right,
               face_data.bottom, face_data.pitch, face_data.yaw, face_data.roll,
               face_data.center_x, face_data.center_y, face_data.center_z);
  this->OnBlink(timestamp, blink_data.is_blink_left, blink_data.is_blink_right, blink_data.is_blink,
                blink_data.left_openness, blink_data.right_openness);
  this->OnAttention(user_status_data.attention_score);
  this->OnDrowsiness(timestamp, user_status_data.is_drowsy, user_status_data.drowsiness_intensity);
}

/**
 * 시선 데이터를 처리하는 메서드
 * @param timestamp 타임스탬프
 * @param x 시선의 x 좌표
 * @param y 시선의 y 좌표
 * @param fixation_x 고정된 시선의 x 좌표
 * @param fixation_y 고정된 시선의 y 좌표
 * @param tracking_state 추적 상태
 * @param eye_movement_state 눈의 움직임 상태
 */
void TrackerManager::OnGaze(uint64_t timestamp,
                            float x, float y,
                            float fixation_x, float fixation_y,
                            EyedidTrackingState tracking_state,
                            EyedidEyeMovementState eye_movement_state) {
  if (tracking_state != kEyedidTrackingSuccess) {
    // 추적 실패 시 초기화된 값으로 콜백 호출
    on_gaze_(0, 0, false);
    return;
  }

  // 창의 시작 좌표를 가져와서 시선 좌표를 보정
  auto winPos = eyedid::getWindowPosition(window_name_);
  x -= static_cast<float>(winPos.x);
  y -= static_cast<float>(winPos.y);

  // 보정된 좌표를 정수로 변환하여 콜백 호출
  on_gaze_(static_cast<int>(x), static_cast<int>(y), true);
}

/**
 * 얼굴 데이터를 처리하는 메서드
 * @param timestamp 타임스탬프
 * @param score 얼굴 탐지 점수
 * @param left 얼굴 좌측 경계
 * @param top 얼굴 상단 경계
 * @param right 얼굴 우측 경계
 * @param bottom 얼굴 하단 경계
 * @param pitch 얼굴의 피치 각도
 * @param yaw 얼굴의 요 각도
 * @param roll 얼굴의 롤 각도
 * @param center_x 얼굴 중심 x 좌표
 * @param center_y 얼굴 중심 y 좌표
 * @param center_z 얼굴 중심 z 좌표
 */
void TrackerManager::OnFace(uint64_t timestamp,
                            float score,
                            float left,
                            float top,
                            float right,
                            float bottom,
                            float pitch,
                            float yaw,
                            float roll,
                            float center_x,
                            float center_y,
                            float center_z) {
  std::cout << "Face Score: " << timestamp << ", " << score << '\n'; // 얼굴 점수 출력
}

/**
 * 주의(attention) 점수를 처리하는 메서드
 * @param score 주의 점수
 */
void TrackerManager::OnAttention(float score) {
  std::cout << "Attention: " << score << '\n'; // 주의 점수 출력
}

/**
 * 눈 깜박임 데이터를 처리하는 메서드
 * @param timestamp 타임스탬프
 * @param isBlinkLeft 왼쪽 눈 깜박임 여부
 * @param isBlinkRight 오른쪽 눈 깜박임 여부
 * @param isBlink 양쪽 눈 깜박임 여부
 * @param leftOpenness 왼쪽 눈 열림 정도
 * @param rightOpenness 오른쪽 눈 열림 정도
 */
void TrackerManager::OnBlink(uint64_t timestamp, bool isBlinkLeft, bool isBlinkRight, bool isBlink,
                             float leftOpenness, float rightOpenness) {
  std::cout << "Blink: " << leftOpenness << ", " << rightOpenness << ", " 
            << isBlinkLeft <<  ", " << isBlinkRight  << '\n'; // 눈 깜박임 데이터 출력
}

/**
 * 졸음 데이터를 처리하는 메서드
 * @param timestamp 타임스탬프
 * @param isDrowsiness 졸음 여부
 * @param intensity 졸음 강도
 */
void TrackerManager::OnDrowsiness(uint64_t timestamp, bool isDrowsiness, float intensity) {
  std::cout << "Drowsiness: " << isDrowsiness << '\n'; // 졸음 상태 출력
}

/**
 * 캘리브레이션 진행 상황을 처리하는 메서드
 * @param progress 진행률 (0.0 ~ 1.0)
 */
void TrackerManager::OnCalibrationProgress(float progress) {
  on_calib_progress_(progress); // 진행률 콜백 호출
}

/**
 * 캘리브레이션 다음 포인트를 설정하는 메서드
 * @param next_point_x 다음 포인트의 x 좌표
 * @param next_point_y 다음 포인트의 y 좌표
 */
void TrackerManager::OnCalibrationNextPoint(float next_point_x, float next_point_y) {
  const auto winPos = eyedid::getWindowPosition(window_name_);
  const auto x = static_cast<int>(next_point_x - static_cast<float>(winPos.x));
  const auto y = static_cast<int>(next_point_y - static_cast<float>(winPos.y));
  on_calib_next_point_(x, y); // 다음 포인트 콜백 호출
  gaze_tracker_.startCollectSamples(); // 샘플 수집 시작
}

/**
 * 캘리브레이션 완료를 처리하는 메서드
 * @param calib_data 캘리브레이션 데이터
 */
void TrackerManager::OnCalibrationFinish(const std::vector<float> &calib_data) {
  on_calib_finish_(calib_data); // 완료 콜백 호출
  calibrating_.store(false, std::memory_order_release); // 캘리브레이션 상태 초기화
}

/**
 * Gaze Tracker 초기화
 * @param license_key 인증 키
 * @param options 트래커 옵션
 * @return 초기화 성공 여부
 */
bool TrackerManager::initialize(const std::string &license_key, const EyedidTrackerOptions& options) {
  const auto code = gaze_tracker_.initialize(license_key, options); // 트래커 초기화
  if (code != 0) {
    std::cerr << "Failed to authenticate (code: " << code << " )\n"; // 오류 출력
    return false;
  }

  gaze_tracker_.setFaceDistance(60); // 얼굴과 카메라 간 거리 설정
  gaze_tracker_.setTrackingCallback(this); // 추적 콜백 연결
  gaze_tracker_.setCalibrationCallback(this); // 캘리브레이션 콜백 연결

  return true;
}

// ... 나머지 메서드도 동일한 방식으로 주석 작성 ...
} // namespace sample
