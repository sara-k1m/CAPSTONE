#ifndef EYEDID_CPP_SAMPLE_TRACKER_MANAGER_H_
#define EYEDID_CPP_SAMPLE_TRACKER_MANAGER_H_

#include <atomic>    // 캘리브레이션 상태 관리에 사용
#include <future>    // 비동기 작업 처리를 위한 std::future
#include <memory>    // 스마트 포인터 사용
#include <string>    // 문자열 처리
#include <vector>    // 벡터 자료구조

#include "eyedid/gaze_tracker.h"   // GazeTracker 클래스 및 관련 데이터 정의
#include "eyedid/util/display.h"   // 디스플레이 정보 관련 유틸리티
#include "opencv2/opencv.hpp"      // OpenCV 기능 사용
#include "simple_signal.h"         // 신호-슬롯 기반 콜백 구현

namespace sample {

/**
 * @class TrackerManager
 * 
 * GazeTracker(시선 추적기) 및 캘리브레이션 기능을 관리하는 클래스.
 * - ITrackingCallback 및 ICalibrationCallback 인터페이스를 구현하여 추적 데이터와 캘리브레이션 이벤트를 처리합니다.
 * - OpenCV 프레임 추가, 창 좌표 처리, 시선 및 캘리브레이션 이벤트 콜백 기능을 제공합니다.
 */
class TrackerManager :
  public eyedid::ITrackingCallback,          // 시선 추적 콜백 인터페이스
  public eyedid::ICalibrationCallback {      // 캘리브레이션 콜백 인터페이스
 public:
  /**
   * 기본 생성자
   * GazeTracker 초기화 및 이벤트 연결 전에 사용할 수 있습니다.
   */
  TrackerManager() = default;

  /**
   * GazeTracker를 초기화하는 함수
   * @param license_key 라이선스 키
   * @param options GazeTracker 초기화 옵션
   * @return 초기화 성공 여부
   */
  bool initialize(const std::string &license_key, const EyedidTrackerOptions& options);

  /**
   * 기본 카메라-디스플레이 변환기를 설정
   * @param display_info 디스플레이 정보
   */
  void setDefaultCameraToDisplayConverter(const eyedid::DisplayInfo& display_info);

  /**
   * OpenCV 프레임을 추가
   * @param timestamp 프레임 타임스탬프
   * @param frame OpenCV 프레임(cv::Mat)
   * @return 프레임 추가 성공 여부
   */
  bool addFrame(std::int64_t timestamp, const cv::Mat& frame);

  /**
   * 전체 창 캘리브레이션 시작
   * @param target_num 캘리브레이션 포인트 개수
   * @param accuracy 캘리브레이션 정확도
   */
  void startFullWindowCalibration(EyedidCalibrationPointNum target_num, EyedidCalibrationAccuracy accuracy);

  /**
   * 화면 전체를 주의 영역(Attention Region)으로 설정
   * @param display_info 디스플레이 정보
   */
  void setWholeScreenToAttentionRegion(const eyedid::DisplayInfo& display_info);

  // ==== 신호(signal) 정의 ====

  /**
   * 시선 데이터 전달 신호
   * @param x 시선 x 좌표
   * @param y 시선 y 좌표
   * @param is_tracking 시선 추적 여부
   */
  signal<void(int, int, bool)> on_gaze_;

  /**
   * 캘리브레이션 진행률 신호
   * @param progress 캘리브레이션 진행률(0.0 ~ 1.0)
   */
  signal<void(float)> on_calib_progress_;

  /**
   * 캘리브레이션 다음 포인트 신호
   * @param x 다음 포인트 x 좌표
   * @param y 다음 포인트 y 좌표
   */
  signal<void(int, int)> on_calib_next_point_;

  /**
   * 캘리브레이션 시작 신호
   */
  signal<void()> on_calib_start_;

  /**
   * 캘리브레이션 완료 신호
   * @param calib_data 캘리브레이션 결과 데이터
   */
  signal<void(const std::vector<float>&)> on_calib_finish_;

  /**
   * 창 이름 (공용 멤버 변수)
   */
  std::string window_name_;

 private:
  // ==== ITrackingCallback 구현 ====

  /**
   * 추적 데이터를 처리하는 콜백 메서드
   */
  void OnMetrics(uint64_t timestamp, const EyedidGazeData& gaze_data, const EyedidFaceData& face_data,
                 const EyedidBlinkData& blink_data, const EyedidUserStatusData& user_status_data) override;

  /**
   * 시선 데이터를 처리하는 메서드
   */
  void OnGaze(uint64_t timestamp, float x, float y, float fixation_x, float fixation_y,
              EyedidTrackingState tracking_state, EyedidEyeMovementState eye_movement_state);

  /**
   * 얼굴 데이터를 처리하는 메서드
   */
  void OnFace(uint64_t timestamp, float score, float left, float top, float right, float bottom,
              float pitch, float yaw, float roll, float center_x, float center_y, float center_z);

  /**
   * 주의 점수를 처리하는 메서드
   */
  void OnAttention(float score);

  /**
   * 눈 깜박임 데이터를 처리하는 메서드
   */
  void OnBlink(uint64_t timestamp, bool isBlinkLeft, bool isBlinkRight, bool isBlink,
               float leftOpenness, float rightOpenness);

  /**
   * 졸음 데이터를 처리하는 메서드
   */
  void OnDrowsiness(uint64_t timestamp, bool isDrowsiness, float intensity);

  // ==== ICalibrationCallback 구현 ====

  /**
   * 캘리브레이션 진행 상황을 처리하는 메서드
   */
  void OnCalibrationProgress(float progress) override;

  /**
   * 캘리브레이션 다음 포인트를 처리하는 메서드
   */
  void OnCalibrationNextPoint(float next_point_x, float next_point_y) override;

  /**
   * 캘리브레이션 완료를 처리하는 메서드
   */
  void OnCalibrationFinish(const std::vector<float>& calib_data) override;

  // ==== 내부 멤버 변수 ====

  /**
   * GazeTracker 객체
   */
  eyedid::GazeTracker gaze_tracker_;

  /**
   * 비동기 캘리브레이션 처리 작업
   */
  std::future<void> delayed_calibration_;

  /**
   * 현재 캘리브레이션 상태를 나타내는 플래그
   * true일 경우 캘리브레이션 중
   */
  std::atomic_bool calibrating_{false};
};

} // namespace sample

#endif // EYEDID_CPP_SAMPLE_TRACKER_MANAGER_H_
