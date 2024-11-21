#ifndef EYEDID_CPP_SAMPLE_DRAWABLES_H_
#define EYEDID_CPP_SAMPLE_DRAWABLES_H_

#include <algorithm>
#include <mutex>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>

#include "opencv2/opencv.hpp"

/**
 * OpenCV를 사용해 UI 요소를 그리는 클래스
 *
 * - 각 요소(Circle, Text, Image)는 draw 메서드를 통해 화면에 그려짐
 * - `draw` 메서드는 요소가 보이는지(visibility)를 확인하지 않음
 * - 가시성을 확인하려면 `draw_if`를 사용
 */

namespace sample {
namespace drawables {
// 모든 그리기 요소의 기본 클래스
struct DrawableBase {
bool visible = true; // 요소의 가시 여부를 나타냄 (기본값: 보임)
};

// 원을 그리기 위한 구조체
struct Circle : protected DrawableBase {
  using DrawableBase::visible; // 부모 클래스의 visible 변수 사용

  // 원을 화면에 그리는 함수
  void draw(cv::Mat* dst) const {
    cv::circle(*dst, center, radius, color, thickness, line_type, shift);
  }

  cv::Point center; // 원의 중심 좌표
  int radius = 10; // 원의 반지름 (기본값: 10)
  cv::Scalar color; // 원의 색상 (BGR 형식)
  int thickness = -1; // 선 두께 (-1이면 내부를 채움)
  int line_type = cv::LINE_8; // 선 유형 (기본값: LINE_8)
  int shift = 0; // 소수점 좌표 정밀도
};

// 이미지를 그리기 위한 구조체
struct Image : protected DrawableBase {
  using DrawableBase::visible; // 부모 클래스의 visible 변수 사용

  // 이미지를 화면에 그리는 함수
  void draw(cv::Mat* dst) const {
    if (buffer.empty()) return; // 이미지 데이터가 없으면 그리지 않음

    cv::resize(buffer, resized_, size); // 이미지를 원하는 크기로 조정
    const auto img_w = std::min(resized_.cols, dst->cols - tl.x); // 화면 너비에 맞춤
    const auto img_h = std::min(resized_.rows, dst->rows - tl.y); // 화면 높이에 맞춤
    resized_(cv::Rect(0, 0, img_w, img_h)).copyTo((*dst)(cv::Rect(tl.x, tl.y, img_w, img_h))); // 이미지를 복사
  }
  cv::Point tl; // 이미지를 그릴 위치 (좌측 상단)
  cv::Size size = { 100, 100 }; // 이미지 크기 (기본값: 100x100)
  cv::Mat buffer; // 이미지 데이터

  private:
    mutable cv::Mat resized_; // 크기 조정된 이미지를 저장 (mutable: const 메서드에서도 변경 가능)
};

// 텍스트를 그리기 위한 구조체
struct Text : protected DrawableBase {
  using DrawableBase::visible; // 부모 클래스의 visible 변수 사용

  // 텍스트를 화면에 그리는 함수
  void draw(cv::Mat* dst) const {
    cv::putText(*dst, text, org, font_face, fontScale, color, thickness, line_type, bottom_left_origin);
  }

  cv::Point org; // 텍스트 시작 좌표
  std::string text; // 표시할 텍스트
  int font_face = cv::FONT_HERSHEY_PLAIN; // 폰트 유형
  double fontScale = 1; // 폰트 크기 배율
  cv::Scalar color = { 255, 255, 255 }; // 텍스트 색상 (기본값: 흰색)
  int thickness = 1; // 선 두께
  int line_type = cv::LINE_8; // 선 유형
  bool bottom_left_origin = false; // 좌표 기준점 (false: 좌측 상단 기준)
};

// 특정 객체가 Drawable인지 확인하기 위한 템플릿
template<typename...> using void_t = void;

template<typename T, typename = void>
struct is_drawable : std::false_type {}; // 기본적으로 Drawable이 아님

template<typename T>
struct is_drawable<
  T,
  void_t<decltype(std::declval<typename std::add_const<T>::type>().draw(static_cast<cv::Mat*>(nullptr)))>
> : std::true_type {}; // draw 메서드가 있으면 Drawable로 간주

// 요소가 가시 상태인 경우에만 그리는 함수
template<typename Drawable>
typename std::enable_if<is_drawable<Drawable>::value>::type
draw_if(const Drawable& drawable, cv::Mat* dst) {
    if (drawable.visible) // 요소가 보이는 상태인지 확인
        drawable.draw(dst); // 요소 그리기
}

// No-own erasure
// Drawable 클래스: 모든 UI 요소를 추상화
class Drawable {
  struct DrawableInterface {
    virtual void draw(cv::Mat* dst) const = 0; // 요소를 그리는 인터페이스
  };

  // 구체적인 Drawable 객체를 포장하는 클래스
  template<typename T>
  struct DrawableConcrete : DrawableInterface {
    explicit DrawableConcrete(T* t) : object_(t) {}

    void draw(cv::Mat* dst) const override {
      if (object_->visible) // 가시 상태인 경우
        object_->draw(dst); // 요소 그리기
    }

    T* object_; // 원래 객체 포인터
  };

  std::shared_ptr<DrawableInterface> ptr_; // Drawable 객체 포인터

  template<typename T>
  T* get_ptr() {
    return dynamic_cast<DrawableConcrete<T>*>(ptr_.get())->object_; // 원래 객체로 변환
  }

 public:
  Drawable() = default;

  // Drawable 객체 생성자
  template<typename T, typename std::enable_if<is_drawable<T>::value, int>::type = 0>
  explicit Drawable(T* object) : ptr_(std::make_shared<DrawableConcrete<T>>(object)) {}

  // 원래 객체를 반환
  template<typename T> T* get_as() { return get_ptr<T>(); }
  template<typename T> const T* get_as() const { return get_ptr<T>(); }

  // 요소를 화면에 그리기
  void draw(cv::Mat* dst) const {
    ptr_->draw(dst);
  }
};

} // namespace drawables
} // namespace sample

#endif // EYEDID_CPP_SAMPLE_DRAWABLES_H_
